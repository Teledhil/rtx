#pragma once

#include <iostream>

#include <vulkan/vulkan.h>

#include "memory.h"
#include "raytracing/acceleration_structure_instance.h"
#include "raytracing/bottom_level_acceleration_structure.h"

namespace rtx {

class top_level_acceleration_structure {
 public:
  top_level_acceleration_structure() = default;

  bool add_instance(const bottom_level_acceleration_structure &blas,
                    const glm::mat4 &transform, uint32_t instance_id,
                    uint32_t hit_group_id) {
    instances_.emplace_back(blas.get_acceleration_structure(), instance_id,
                            hit_group_id, transform);
    return true;
  }

  // Create the handle required to build the acceleration structure.
  //
  // It requires a flag to indicate whether the acceleration structure will
  // support dynamic updates, so that the builder can later optimize the
  // structure for that usage.
  //
  // It is required to know the number of instances inserted in advance, that
  // is why this method must be called after all the instances have been added
  // with add_instance().
  bool create(VkDevice device,
              const VkAllocationCallbacks *allocation_callbacks,
              bool allow_update) {
    // The generated acceleration structure can support iterative updates. This
    // updates may change the final size of the acceleration structure and then
    // the memory requirements. This flag must be set before the acceleration
    // structure is built.
    flags_ =
        allow_update ? VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_NV : 0;

    // Setup the descriptor of the acceleration structure which contains the
    // number of geometries it will contain.
    VkAccelerationStructureInfoNV as_info = descriptor();

    VkAccelerationStructureCreateInfoNV as_create_info{};
    as_create_info.sType =
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_NV;
    as_create_info.info = as_info;

    VkResult res = vkCreateAccelerationStructureNV(device, &as_create_info,
                                                   allocation_callbacks,
                                                   &acceleration_structure_);
    if (VK_SUCCESS != res) {
      std::cerr << "Failed to create TLAS." << std::endl;
      return false;
    }

    return true;
  }

  bool compute_buffer_sizes(VkDevice device, memory &mem,
                            VkDeviceSize &scratch_size) {
    // Create the descriptor for the memory requirements.
    //
    VkAccelerationStructureMemoryRequirementsInfoNV memory_requirements_info{};
    memory_requirements_info.sType =
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV;
    memory_requirements_info.type =
        VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_OBJECT_NV;
    memory_requirements_info.accelerationStructure = acceleration_structure_;

    // Compute the size of the built acceleration structure and store it for
    // future build/updates.
    //
    VkMemoryRequirements2 memory_requirements{};
    memory_requirements.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;
    vkGetAccelerationStructureMemoryRequirementsNV(
        device, &memory_requirements_info, &memory_requirements);
    structure_size_ = memory_requirements.memoryRequirements.size;

    // Allocate the GPU memory that will contain the acceleration structure.
    VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    if (!mem.allocate_memory(memory_requirements.memoryRequirements, properties,
                             acceleration_structure_memory_)) {
      std::cerr << "Failed to allocate BLAS memory." << std::endl;
      return false;
    }

    // Compute the scratch buffer size needed to build the acceleration
    // structure.
    //
    memory_requirements_info.type =
        VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_BUILD_SCRATCH_NV;
    vkGetAccelerationStructureMemoryRequirementsNV(
        device, &memory_requirements_info, &memory_requirements);
    scratch_size_ = memory_requirements.memoryRequirements.size;

    // Compute the scratch buffer size needed to update the acceleration
    // structure.
    //
    memory_requirements_info.type =
        VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_UPDATE_SCRATCH_NV;
    vkGetAccelerationStructureMemoryRequirementsNV(
        device, &memory_requirements_info, &memory_requirements);
    scratch_size_ =
        std::max(scratch_size_, memory_requirements.memoryRequirements.size);

    scratch_size = scratch_size_;

    // Size of the instance descriptors.
    instance_descriptors_size_ =
        instances_.size() * sizeof(geometry_instance_t);

    return true;
  }

  bool generate(VkDevice device, memory &mem, VkCommandBuffer command_buffer,
                VkBuffer scratch_buffer, VkDeviceSize scratch_offset,
                bool update_only) {
    // Sanity checks for update option.
    if (update_only) {
      if (!(flags_ & VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_NV)) {
        std::cerr
            << "Cannot update TLAS originally built without update support."
            << std::endl;
        return false;
      }
      if (VK_NULL_HANDLE == acceleration_structure_) {
        std::cerr << "Cannot update TLAS not built." << std::endl;
        return false;
      }
    }

    // Sanity checks for buffer sizes.
    if (0 == scratch_size_ || 0 == instance_descriptors_size_ ||
        0 == structure_size_) {
      std::cerr << "TLAS: compute_buffer_sizes() must be run before generate()."
                << std::endl;
      return false;
    }

    std::vector<geometry_instance_t> geometry_instances;
    for (auto &instance : instances_) {
      geometry_instances.emplace_back();
      if (!convert_instance_to_instance_descriptor(device, instance,
                                                   geometry_instances.back())) {
        std::cerr << "TLAS: Failed to convert instance to geometry instance."
                  << std::endl;
        return false;
      }
    }

    // Copy the instance descriptors into the instance buffer.
    //
    VkBufferUsageFlags usage = VK_BUFFER_USAGE_RAY_TRACING_BIT_NV;

    VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                       VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    if (!mem.create_buffer_and_copy(
            instance_descriptors_size_, usage, properties, instance_buffer_,
            instance_buffer_memory_, geometry_instances.data())) {
      std::cerr << "Failed to create tlas instance buffer." << std::endl;
      return false;
    }

    // Bind the acceleration structure descriptor to the memory that will
    // contain it.
    //
    VkBindAccelerationStructureMemoryInfoNV bind{};
    bind.sType = VK_STRUCTURE_TYPE_BIND_ACCELERATION_STRUCTURE_MEMORY_INFO_NV;
    bind.accelerationStructure = acceleration_structure_;
    bind.memory =
        acceleration_structure_memory_;  // TODO: Receive VkDeviceMemory +
                                         // offset after all BLASs.
    bind.memoryOffset = 0;
    bind.deviceIndexCount = 0;
    bind.pDeviceIndices = nullptr;

    static constexpr uint32_t bind_info_count = 1;
    VkResult res =
        vkBindAccelerationStructureMemoryNV(device, bind_info_count, &bind);
    if (VK_SUCCESS != res) {
      std::cerr << "Bind of TLAS failed." << std::endl;
      return false;
    }

    // Make sure the copy of the instance buffer is copied before triggering
    // the acceleration structure build.
    VkMemoryBarrier memory_barrier{};
    memory_barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    memory_barrier.dstAccessMask =
        VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV;

    VkPipelineStageFlags src_stage_mask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    VkPipelineStageFlags dst_stage_mask =
        VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV;
    VkDependencyFlags dependency_flags = 0;
    uint32_t memory_barrier_count = 1;
    uint32_t buffer_memory_barrier_count = 0;
    const VkBufferMemoryBarrier *buffer_memory_barriers = nullptr;
    uint32_t image_memory_barrier_count = 0;
    const VkImageMemoryBarrier *image_memory_barriers = nullptr;
    vkCmdPipelineBarrier(command_buffer, src_stage_mask, dst_stage_mask,
                         dependency_flags, memory_barrier_count,
                         &memory_barrier, buffer_memory_barrier_count,
                         buffer_memory_barriers, image_memory_barrier_count,
                         image_memory_barriers);

    VkAccelerationStructureInfoNV as_info = descriptor();

    VkDeviceSize instance_offset = 0;
    VkAccelerationStructureNV as_src =
        update_only ? acceleration_structure_ : VK_NULL_HANDLE;
    vkCmdBuildAccelerationStructureNV(command_buffer, &as_info,
                                      instance_buffer_, instance_offset,
                                      update_only, acceleration_structure_,
                                      as_src, scratch_buffer, scratch_offset);

    return true;
  }

  void destroy(VkDevice device,
               const VkAllocationCallbacks *allocation_callbacks) {
    vkDestroyAccelerationStructureNV(device, acceleration_structure_,
                                     allocation_callbacks);
    acceleration_structure_ = VK_NULL_HANDLE;

    vkFreeMemory(device, acceleration_structure_memory_, allocation_callbacks);
    acceleration_structure_memory_ = VK_NULL_HANDLE;

    vkDestroyBuffer(device, instance_buffer_, allocation_callbacks);
    instance_buffer_ = VK_NULL_HANDLE;

    vkFreeMemory(device, instance_buffer_memory_, allocation_callbacks);
    instance_buffer_memory_ = VK_NULL_HANDLE;

    instances_.clear();
  }

  size_t num_instances() const { return instances_.size(); }

  const VkAccelerationStructureNV &get_acceleration_structure() const {
    return acceleration_structure_;
  }

 private:
  // Attributes
  //

  // The acceleration structure.
  VkAccelerationStructureNV acceleration_structure_;

  // The memory containing the acceleration structure.
  VkDeviceMemory acceleration_structure_memory_;

  // The buffer containing the instance descriptors.
  VkBuffer instance_buffer_;

  // The memory where the instance buffer is stored.
  VkDeviceMemory instance_buffer_memory_;

  // Construction flags, used to indicate whether the AS allows updates.
  VkBuildAccelerationStructureFlagsNV flags_;

  // Size needed for the temporary memory used to build the TLAS.
  VkDeviceSize scratch_size_;

  // Size of the buffer containing the instance descriptors.
  VkDeviceSize instance_descriptors_size_;

  // Size of the buffer containing the TLAS.
  VkDeviceSize structure_size_;

  // List of BLAS instances.
  std::vector<new_blas_instance_t> instances_;

  // Methods
  //

  VkAccelerationStructureInfoNV descriptor() const {
    VkAccelerationStructureInfoNV build_info{};
    build_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
    build_info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_NV;
    build_info.instanceCount = static_cast<uint32_t>(instances_.size());
    build_info.geometryCount = 0;  // Always zero with top-level.
    build_info.pGeometries = VK_NULL_HANDLE;
    build_info.flags = flags_;
    return build_info;
  }

  bool convert_instance_to_instance_descriptor(
      VkDevice device, const new_blas_instance_t &instance,
      geometry_instance_t &geometry_instance) {
    // For each BLAS, fetch the acceleration structure handle that will allow
    // the builder to access it from the device;
    uint64_t as_handle;
    VkResult res = vkGetAccelerationStructureHandleNV(
        device, instance.blas, sizeof(as_handle), &as_handle);
    if (VK_SUCCESS != res) {
      std::cerr << "Failed to get acceleration structure handle." << std::endl;
      return false;
    }

    geometry_instance = {};
    geometry_instance.transform =
        glm::mat3x4(glm::transpose(instance.transform));
    geometry_instance.instance_id = instance.instance_id;
    geometry_instance.mask = instance.mask;
    geometry_instance.hit_group_id = instance.hit_group_id;
    geometry_instance.flags = instance.flags;
    geometry_instance.acceleration_structure_handle = as_handle;

    return true;
  }
};

}  // namespace rtx
