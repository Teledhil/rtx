#pragma once

#include <vulkan/vulkan.h>

#include "glm.h"

typedef struct {
  glm::mat4 mvp;
  glm::mat4 inverse_view;
  glm::mat4 inverse_projection;
} uniform_data_data_t;

typedef struct {
  VkBuffer buf;
  VkDeviceMemory mem;
  VkDescriptorBufferInfo buffer_info;
  size_t size;
  uniform_data_data_t data;
} uniform_data_t;
