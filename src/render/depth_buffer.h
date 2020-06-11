#pragma once

#include <vulkan/vulkan.h>

typedef struct {
  VkFormat format;

  VkImage image;
  VkDeviceMemory mem;
  VkImageView view;
} depth_buffer_t;
