#pragma once

#include <iostream>

#include <vulkan/vulkan.h>

#include "memory.h"
#include "object.h"

namespace rtx {
class bottom_level_acceleration_structure {
 public:
  bottom_level_acceleration_structure() = default;

  // Add an object vertex and index buffers in GPU memory into the acceleration
  // structure. Index buffer is optional.
  bool add_object(const object_model_t &object) {
    VkGeometryNV geometry{};

    // VK_GEOMETRY_OPAQUE_BIT_NV means the object won't invoke any-hit shaders.
    VkGeometryFlagBitsNV flags = VK_GEOMETRY_OPAQUE_BIT_NV;

    if (!convert_object_to_geometry_nv(object, geometry, flags)) {
      std::cerr << "BLAS: Failed to add geometry." << std::endl;
      return false;
    }
    geometries_.push_back(geometry);

    return true;
  }

  void add_transform(const glm::mat4 &transform) {
    transforms_.emplace_back(transform);
  }

  void clear_transforms() { transforms_.clear(); }

  const std::vector<glm::mat4> &get_transforms() const { return transforms_; }

  // Create the handle required to build the acceleration structure.
  //
  // It requires a flag to indicate whether the acceleration structure will
  // support dynamic updates, so that the builder can later optimize the
  // structure for that usage.
  //
  // It is required to know the number of geometries inserted in advance, that
  // is why this method must be called after all the geometries have been added
  // with add_object().
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
      std::cerr << "Failed to create BLAS." << std::endl;
      return false;
    }

    return true;
  }

  // Compute the size of the scratch buffer required to build the acceleration
  // structure and the size of the structure once built.
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

    return true;
  }

  bool generate(VkDevice device, VkCommandBuffer command_buffer,
                VkBuffer scratch_buffer, VkDeviceSize scratch_offset,
                bool update_only) {
    // Sanity checks for update option.
    if (update_only) {
      if (!(flags_ & VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_NV)) {
        std::cerr
            << "Cannot update BLAS originally built without update support."
            << std::endl;
        return false;
      }
      if (VK_NULL_HANDLE == acceleration_structure_) {
        std::cerr << "Cannot update BLAS not built." << std::endl;
        return false;
      }
    }

    // Sanity checks for buffer sizes.
    if (0 == scratch_size_ || 0 == structure_size_) {
      std::cerr << "BLAS: compute_buffer_sizes() must be run before generate()."
                << std::endl;
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
                                         // offset for all BLASs.
    bind.memoryOffset = 0;
    bind.deviceIndexCount = 0;
    bind.pDeviceIndices = nullptr;

    static constexpr uint32_t bind_info_count = 1;
    VkResult res =
        vkBindAccelerationStructureMemoryNV(device, bind_info_count, &bind);
    if (VK_SUCCESS != res) {
      std::cerr << "Bind of BLAS failed." << std::endl;
      return false;
    }

    // Build the actual acceleration structure.
    //

    // Re-create the acceleration structure descriptor.
    VkAccelerationStructureInfoNV as_info = descriptor();

    VkBuffer instance_data = VK_NULL_HANDLE;
    VkDeviceSize instance_offset = 0;
    vkCmdBuildAccelerationStructureNV(
        command_buffer, &as_info, instance_data, instance_offset, update_only,
        acceleration_structure_,
        update_only ? acceleration_structure_ : VK_NULL_HANDLE, scratch_buffer,
        scratch_offset);

    // Since the scratch buffer is reused for each BLAS, add a barrier to
    // wait from previous build before the next.
    VkMemoryBarrier memory_barrier{};
    memory_barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    memory_barrier.srcAccessMask =
        VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV;
    memory_barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV;

    VkPipelineStageFlags src_stage_mask =
        VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV;
    VkPipelineStageFlags dst_stage_mask =
        VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV;
    VkDependencyFlags dependency_flags = 0;
    uint32_t memory_barrier_count = 1;
    uint32_t buffer_memory_barrier_count = 0;
    const VkBufferMemoryBarrier *buffer_memory_barrier = nullptr;
    uint32_t image_memory_barrier_count = 0;
    const VkImageMemoryBarrier *image_memory_barrier = nullptr;

    vkCmdPipelineBarrier(command_buffer, src_stage_mask, dst_stage_mask,
                         dependency_flags, memory_barrier_count,
                         &memory_barrier, buffer_memory_barrier_count,
                         buffer_memory_barrier, image_memory_barrier_count,
                         image_memory_barrier);

    return true;
  }

  void destroy(VkDevice device,
               const VkAllocationCallbacks *allocation_callbacks) {
    vkDestroyAccelerationStructureNV(device, acceleration_structure_,
                                     allocation_callbacks);
    acceleration_structure_ = VK_NULL_HANDLE;

    vkFreeMemory(device, acceleration_structure_memory_, allocation_callbacks);
    acceleration_structure_memory_ = VK_NULL_HANDLE;
  }

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

  // Construction flags, used to indicate whether the AS allows updates.
  VkBuildAccelerationStructureFlagsNV flags_;

  // List of geometries contained on the BLAS.
  std::vector<VkGeometryNV> geometries_;

  // List of transofmration matrix that applied to all geometries.
  std::vector<glm::mat4> transforms_;

  // Size needed for the temporary memory used to build the BLAS.
  VkDeviceSize scratch_size_;

  // Size of the buffer containing the BLAS.
  VkDeviceSize structure_size_;

  // Methods
  //
  static bool convert_object_to_geometry_nv(const object_model_t &object,
                                            VkGeometryNV &geometry,
                                            VkGeometryFlagBitsNV flags) {
    geometry.sType = VK_STRUCTURE_TYPE_GEOMETRY_NV;
    geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_NV;

    // Vertex buffer.
    geometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_GEOMETRY_TRIANGLES_NV;
    geometry.geometry.triangles.vertexData = object.vertex_buf;
    geometry.geometry.triangles.vertexOffset = object.vertex_offset;
    geometry.geometry.triangles.vertexCount =
        static_cast<uint32_t>(object.vertices.size());
    geometry.geometry.triangles.vertexStride = sizeof(Vertex);
    geometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;

    // Index buffer.
    // This one is optional.
    //
    geometry.geometry.triangles.indexData = object.index_buf;
    geometry.geometry.triangles.indexOffset = object.index_offset;
    geometry.geometry.triangles.indexCount =
        static_cast<uint32_t>(object.indices.size());
    geometry.geometry.triangles.indexType = object.index_buf != VK_NULL_HANDLE
                                                ? VK_INDEX_TYPE_UINT32
                                                : VK_INDEX_TYPE_NONE_NV;

    // Transformation matrix.
    // Not used.
    //
    geometry.geometry.triangles.transformData = VK_NULL_HANDLE;
    geometry.geometry.triangles.transformOffset = 0;

    // axis-aligned bounding box.
    //
    geometry.geometry.aabbs = {};
    geometry.geometry.aabbs.sType = VK_STRUCTURE_TYPE_GEOMETRY_AABB_NV;

    geometry.flags = flags;

    return true;
  }

  VkAccelerationStructureInfoNV descriptor() const {
    VkAccelerationStructureInfoNV build_info{};
    build_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
    build_info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_NV;
    build_info.instanceCount = 0;  // Always zero with bottom-level.
    build_info.geometryCount = static_cast<uint32_t>(geometries_.size());
    build_info.pGeometries = geometries_.data();
    build_info.flags = flags_;
    return build_info;
  }
};
}  // namespace rtx
