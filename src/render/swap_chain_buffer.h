#pragma once

#include <vulkan/vulkan.h>

typedef struct {
  VkImage image;
    VkImageView view;
} swap_chain_buffer_t;
