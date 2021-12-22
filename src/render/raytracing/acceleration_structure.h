#pragma once

#include <vector>

#include <vulkan/vulkan.h>

#include "glm.h"
#include "raytracing/bottom_level_acceleration_structure.h"
#include "raytracing/top_level_acceleration_structure.h"

namespace rtx {
// TODO: Rename to ray_tracer? create a separate ray_tracer?
class acceleration_structure {
 public:
  // Each call to this method will create a separate BLAS. All objects passed
  // will go into the new BLAS.
  bool add_objects(const std::vector<object_model_t> &objects) {
    // Create a new BLAS.
    blas_.emplace_back();
    bottom_level_acceleration_structure &blas = blas_.back();

    // Add objects to the BLAS.
    for (auto &object : objects) {
      if (!blas.add_object(object)) {
        std::cerr << "Failed to add object to BLAS." << std::endl;
        return false;
      }
      for (auto &transform : object.transforms) {
        blas.add_transform(transform);
      }
    }

    return true;
  }

  bool add_object(const object_model_t &object) {
    // Create a new BLAS.
    blas_.emplace_back();
    bottom_level_acceleration_structure &blas = blas_.back();

    // Add object to the BLAS.
    if (!blas.add_object(object)) {
      std::cerr << "Failed to add object to BLAS." << std::endl;
      return false;
    }
    for (auto &transform : object.transforms) {
      blas.add_transform(transform);
    }

    return true;
  }

  bool generate(VkDevice device, memory &mem, VkCommandBuffer command_buffer,
                bool update_only) {
    // Create the BLAS descriptors and compute the buffer sizes for each BLAS.
    // Obtain the maximun scratch buufer size needed so only one scratch buffer
    // will be created for all BLASs.
    VkDeviceSize max_scratch_size = 0;
    for (auto &blas : blas_) {
      if (!blas.create(device, mem.get_allocation_callbacks(), update_only)) {
        std::cerr << "Failed to create BLAS descriptor." << std::endl;
        return false;
      }

      VkDeviceSize scratch_size = 0;
      if (!blas.compute_buffer_sizes(device, mem, scratch_size)) {
        std::cerr << "Failed to compute BLAS scratch size." << std::endl;
        return false;
      }
      std::cout << "Partial scratch buffer size: " << scratch_size << " bytes."
                << std::endl;
      max_scratch_size = std::max(max_scratch_size, scratch_size);
    }
    std::cout << "Final scratch buffer size: " << max_scratch_size << " bytes."
              << std::endl;

    // Create scratch buffer.
    VkBuffer scratch_buffer;
    VkDeviceMemory scratch_buffer_memory;
    if (!create_scratch_buffer(mem, scratch_buffer, scratch_buffer_memory,
                               max_scratch_size)) {
      return false;
    }

    // Create the actual BLASs.
    VkDeviceSize scratch_offset = 0;
    for (auto &blas : blas_) {
      if (!blas.generate(device, command_buffer, scratch_buffer, scratch_offset,
                         update_only)) {
        std::cerr << "Failed to generate BLAS." << std::endl;
        return false;
      }
    }

    // Create the TLAS.
    //

    // Add instances to TLAS.
    uint32_t hit_group_id = 0;
    for (int i = 0; i < blas_.size(); ++i) {
      for (auto &transform : blas_[i].get_transforms()) {
        if (!add_instance(i, hit_group_id, transform)) {
          return false;
        }
      }
    }

    // Create TLAS descriptor.
    if (!tlas_.create(device, mem.get_allocation_callbacks(), update_only)) {
      std::cerr << "Failed to create TLAS descriptor." << std::endl;
      return false;
    }

    // Compute TLAS scratch buffer size. If possible, reuse scratch buffer used
    // for BLASes.
    VkDeviceSize tlas_scratch_size = 0;
    if (!tlas_.compute_buffer_sizes(device, mem, tlas_scratch_size)) {
      std::cerr << "Failed to compute TLAS scratch buffer size." << std::endl;
      return false;
    }
    if (tlas_scratch_size > max_scratch_size) {
      // BLAS scratch buffer is not big enough. Delete it and recreate it.
      std::cout << "TLAS scratch buffer re-created." << std::endl;
      destroy_scratch_buffer(mem, scratch_buffer, scratch_buffer_memory);
      if (!create_scratch_buffer(mem, scratch_buffer, scratch_buffer_memory,
                                 tlas_scratch_size)) {
        std::cerr << "Failed to re-create scratch buffer." << std::endl;
        return false;
      }
    }

    // Generate the TLAS.
    scratch_offset = 0;
    if (!tlas_.generate(device, mem, command_buffer, scratch_buffer,
                        scratch_offset, update_only)) {
      std::cerr << "Failed to generate TLAS." << std::endl;
      return false;
    }

    destroy_scratch_buffer(mem, scratch_buffer, scratch_buffer_memory);

    return true;
  }

  void destroy(memory &mem) {
    // Destroy TLAS.
    tlas_.destroy(mem.get_device(), mem.get_allocation_callbacks());

    // Destroy BLASses.
    for (auto &blas : blas_) {
      blas.destroy(mem.get_device(), mem.get_allocation_callbacks());
    }
    blas_.clear();
  }

  const VkAccelerationStructureNV &get_tlas() const {
    return tlas_.get_acceleration_structure();
  }

 private:
  std::vector<bottom_level_acceleration_structure> blas_;  // TODO: blases_.
  top_level_acceleration_structure tlas_;

  bool add_instance(uint32_t blas_id, uint32_t hit_group_id,
                    const glm::mat4 &transform) {
    // This method has to be called after the BLAS is created with its
    // generate() method.

    if (!tlas_.add_instance(blas_[blas_id], transform, tlas_.num_instances(),
                            hit_group_id)) {
      std::cerr << "Failed to add instance." << std::endl;
      return false;
    }
    return true;
  }

  bool create_scratch_buffer(memory &mem, VkBuffer &buffer,
                             VkDeviceMemory &buffer_memory, VkDeviceSize size) {
    VkBufferUsageFlags usage = VK_BUFFER_USAGE_RAY_TRACING_BIT_NV;
    VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    if (!mem.create_buffer(size, usage, properties, buffer, buffer_memory)) {
      std::cerr << "Failed to create scratch buffer." << std::endl;
      return false;
    }

    return true;
  }

  void destroy_scratch_buffer(memory &mem, VkBuffer &scratch_buffer,
                              VkDeviceMemory &scratch_buffer_memory) {
    vkDestroyBuffer(mem.get_device(), scratch_buffer,
                    mem.get_allocation_callbacks());
    scratch_buffer = VK_NULL_HANDLE;

    vkFreeMemory(mem.get_device(), scratch_buffer_memory,
                 mem.get_allocation_callbacks());
    scratch_buffer_memory = VK_NULL_HANDLE;
  }
};
}  // namespace rtx
