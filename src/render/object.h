#pragma once

#include <vector>

#include <vulkan/vulkan.h>

#include "glm.h"

#include "vertex.h"

namespace rtx {

struct object_model_t {
  // Vertex
  //
  std::vector<Vertex> vertices;
  VkBuffer vertex_buf;  // Buffer containing the vertex coordinates.
  VkDeviceSize
      vertex_offset;  // Offset of the first vertex inside the vertex buffer.
  VkDeviceMemory vertex_mem;

  // Index
  //
  std::vector<uint32_t> indices;
  VkBuffer index_buf;  // Buffer containing the vertex indices describing the
                       // triangles.
                       // TODO: Merge with vertex buffer.
  VkDeviceSize index_offset;  // Offset of the first index inside the buffer.
  VkDeviceMemory index_mem;

  // Transform
  // VkBuffer transform_buf; // Buffer containing a 4x4 transform matrix in GPU
  //                         // memory, to be applied to the vertices.
  // VkDeviceSize transform_offset; // Offset of the transform matrix in the
  //                                // transform buffer.
  // VkDeviceMemory transform_mem;

  std::vector<glm::mat4> transforms;
};

// TODO: Delete
struct object_instance_t {
  uint32_t index;
  glm::mat4 transform = glm::mat4(1.0f);
};

}  // namespace rtx
