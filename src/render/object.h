#pragma once

#include <vector>

#include <vulkan/vulkan.h>

#include <glm/glm.hpp>

#include "vertex.h"
#include "vertex_buffer.h"

namespace rtx {

struct object_model_t {
  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;
  vertex_buffer_t vertex_buffer;
};

struct object_instance_t {
  uint32_t index;
  glm::mat4 transform = glm::mat4(1.0f);
};

}  // namespace rtx
