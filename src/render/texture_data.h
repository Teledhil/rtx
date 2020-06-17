#pragma once

#include <vulkan/vulkan.h>

typedef struct {
  VkDescriptorImageInfo image_info;
} texture_data_t;

typedef struct {
  VkSampler sampler;

  VkImage image;
  VkImageLayout image_layout;

  bool needs_staging;
  VkBuffer buffer;
  VkDeviceSize buffer_size;

  VkDeviceMemory image_memory;
  VkDeviceMemory buffer_memory;
  VkImageView view;
  int32_t texture_width;
  int32_t texture_height;
} texture_object_t;
