#pragma once

#include <vulkan/vulkan.h>

#include "glm.h"

namespace rtx {

// Helper structure to hold the instance data.
struct new_blas_instance_t {
  new_blas_instance_t(const VkAccelerationStructureNV &_blas,
                      uint32_t _instance_id, uint32_t _hit_group_id,
                      const glm::mat4 &_transform)
      : blas(_blas),
        instance_id(_instance_id),
        hit_group_id(_hit_group_id),
        transform(_transform) {}

  // Index of the blas on Engine::blas_.
  // uint32_t blas_id;

  // Bottom-Level Acceleration Structure.
  VkAccelerationStructureNV blas;

  // Instance ID used by shaders gl_InstanceID.
  uint32_t instance_id;

  // Hit group index on the SBT.
  uint32_t hit_group_id;

  // Visibility mask, AND-ed with ray mask.
  uint32_t mask = 0xff;

  // instance flags.
  VkGeometryInstanceFlagsNV flags =
      VK_GEOMETRY_INSTANCE_TRIANGLE_CULL_DISABLE_BIT_NV;

  // Transform matrix.
  glm::mat4 transform = glm::mat4(1.0f);
};

// Struct containing the passing instance data required by the
// VK_NV_ray_tracing extension.
// This struct details a 64-byte data layout that for some reason is not in any
// vulkan header.
struct geometry_instance_t {
  // Transform matrix, containing only the top 3 rows.
  glm::mat3x4 transform;

  // Instance index.
  uint32_t instance_id : 24;

  // Visibility mask.
  uint32_t mask : 8;

  // Index of the hit group which will be invoked when a ray hits the instance.
  uint32_t hit_group_id : 24;

  // Instance flags, such as culling.
  uint32_t flags : 8;

  // Opaque handle of the bottom-level acceleration structure.
  uint64_t acceleration_structure_handle;
};

static_assert(sizeof(geometry_instance_t) == 64,
              "geometry_instance_t compiles with wrong size.");
}  // namespace rtx
