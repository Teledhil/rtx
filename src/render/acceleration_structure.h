#pragma once

#include <vulkan/vulkan.h>

#include "glm.h"

namespace rtx {

struct acceleration_structure_t {
  VkDeviceMemory mem;
  VkAccelerationStructureNV as;
  // uint64_t handle;
};

struct blas_t {
  acceleration_structure_t as;
  VkAccelerationStructureInfoNV as_info;
  // VkGeometryNV geometry;
};

struct tlas_t {
  acceleration_structure_t as;
  VkAccelerationStructureInfoNV as_info;
  VkBuffer buffer;
  VkDeviceMemory mem;
};

struct blas_instance_t {
  uint32_t blas_id;       // index of the blas on Engine::blas_.
  uint32_t instance_id;   // Instance index, gl_InstanceID.
  uint32_t hit_group_id;  // Hit group index on the SBT.
  uint32_t mask = 0xff;   // Visibility mask, AND-ed with ray mask.
  VkGeometryInstanceFlagsNV flags =
      VK_GEOMETRY_INSTANCE_TRIANGLE_CULL_DISABLE_BIT_NV;
  glm::mat4 transform = glm::mat4(1.0f);
};

// TODO: Remove after tutorial.
struct VkGeometryInstanceNV {
  /// Transform matrix, containing only the top 3 rows
  glm::mat3x4 transform;
  /// Instance index
  uint32_t instance_id : 24;
  /// Visibility mask
  uint32_t mask : 8;
  /// Index of the hit group which will be invoked when a ray hits the instance
  uint32_t hit_group_id : 24;
  /// Instance flags, such as culling
  uint32_t flags : 8;
  /// Opaque handle of the bottom-level acceleration structure
  uint64_t acceleration_structure_handle;
};

struct storage_image_t {
  VkDeviceMemory mem;
  VkImage image;
  VkImageView view;
  VkFormat format;
};

struct ray_tracing_constants_t {
  glm::vec4 clear_color;
  glm::vec3 light_position;
  float light_intensity;
  int light_type;
  int frame;
  int samples;
  int max_iterations;
  bool temperature;
};

struct shader_binding_table_t {
  VkBuffer buffer;
  VkDeviceMemory mem;
};
}  // namespace rtx
