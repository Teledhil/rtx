#pragma once

#include <vulkan/vulkan.h>

typedef struct {
  VkBuffer buf;
  VkDeviceMemory mem;
  VkDescriptorBufferInfo buffer_info;
  VkBuffer index_buf;  // TODO: Merge with vertex buffer.
  VkDeviceMemory index_mem;
} vertex_buffer_t;
