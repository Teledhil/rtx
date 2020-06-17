#pragma once

#include <vulkan/vulkan.h>

typedef struct {
  VkBuffer buf;
  VkDeviceMemory mem;
  VkDescriptorBufferInfo buffer_info;
} vertex_buffer_t;
