#pragma once

#include <vulkan/vulkan.h>

typedef struct {
  VkBuffer buf;
        VkDeviceMemory mem;
        VkDescriptorBufferInfo buffer_info;
        size_t size;
} uniform_data_t;
