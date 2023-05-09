#pragma once

#include <string.h>

#include <iostream>

#include <vulkan/vulkan.h>

namespace rtx {
class memory {
 public:
  memory() = default;
  memory(VkDevice device,
         const VkPhysicalDeviceMemoryProperties memory_properties,
         const VkAllocationCallbacks *allocation_callbacks)
      : device_(device),
        memory_properties_(memory_properties),
        allocation_callbacks_(allocation_callbacks) {}

  memory &operator=(memory &&) = default;

  bool allocate_memory(const VkMemoryRequirements &memory_requirements,
                       const VkMemoryPropertyFlags &properties,
                       VkDeviceMemory &memory) {
    std::cout << "allocating memory (" << memory_requirements.size << " bytes)"
              << std::endl;
    // Allocate the memory.
    //
    VkMemoryAllocateInfo memory_allocate_info{};
    memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memory_allocate_info.allocationSize = memory_requirements.size;

    if (!memory_type_from_properties(memory_requirements.memoryTypeBits,
                                     properties,
                                     &memory_allocate_info.memoryTypeIndex)) {
      std::cerr << "Failed to get memory type properties." << std::endl;
      return false;
    }

    VkResult res = vkAllocateMemory(device_, &memory_allocate_info,
                                    allocation_callbacks_, &memory);
    if (VK_SUCCESS != res) {
      std::cerr << "Failed to allocate memory: " << res << std::endl;
      return false;
    }

    return true;
  }

  bool create_buffer(VkDeviceSize size, VkBufferUsageFlags usage,
                     VkMemoryPropertyFlags properties, VkBuffer &buffer,
                     VkDeviceMemory &buffer_memory) {
    VkBufferCreateInfo buffer_create_info{};
    buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_create_info.pNext = nullptr;
    buffer_create_info.size = size;
    buffer_create_info.usage = usage;
    buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkResult res = vkCreateBuffer(device_, &buffer_create_info,
                                  allocation_callbacks_, &buffer);
    if (VK_SUCCESS != res) {
      std::cerr << "Failed to create buffer: " << res << std::endl;
      return false;
    }

    VkMemoryRequirements memory_requirements;
    vkGetBufferMemoryRequirements(device_, buffer, &memory_requirements);

    if (!allocate_memory(memory_requirements, properties, buffer_memory)) {
      std::cerr << "Failed to allocate buffer." << std::endl;
      return false;
    }

    VkDeviceSize memory_offset = 0;
    res = vkBindBufferMemory(device_, buffer, buffer_memory, memory_offset);
    if (VK_SUCCESS != res) {
      std::cerr << "Failed to bind buffer: " << res << std::endl;
      return false;
    }

    return true;
  }

  bool copy_to_buffer(VkDeviceMemory buffer_memory, VkDeviceSize size,
                      const void *data) {
    void *mapped_data;
    VkDeviceSize offset = 0;
    VkMemoryMapFlags map_flags = 0;

    VkResult res = vkMapMemory(device_, buffer_memory, offset, size, map_flags,
                               &mapped_data);
    if (VK_SUCCESS != res) {
      std::cerr << "Failed to map buffer to CPU memory: " << res << std::endl;
      return false;
    }

    memcpy(mapped_data, data, static_cast<size_t>(size));

    vkUnmapMemory(device_, buffer_memory);

    return true;
  }

  bool create_buffer_and_copy(VkDeviceSize size, VkBufferUsageFlags usage,
                              VkMemoryPropertyFlags properties,
                              VkBuffer &buffer, VkDeviceMemory &buffer_memory,
                              const void *data) {
    if (!create_buffer(size, usage, properties, buffer, buffer_memory)) {
      return false;
    }

    if (!copy_to_buffer(buffer_memory, size, data)) {
      return false;
    }

    return true;
  }

  VkDevice get_device() const { return device_; }
  const VkAllocationCallbacks *get_allocation_callbacks() const {
    return allocation_callbacks_;
  }

 private:
  VkDevice device_;
  VkPhysicalDeviceMemoryProperties memory_properties_;
  const VkAllocationCallbacks *allocation_callbacks_;

  bool memory_type_from_properties(uint32_t type_bits,
                                   VkFlags requirements_mask,
                                   uint32_t *type_index) {
    for (uint32_t i = 0; i < memory_properties_.memoryTypeCount; ++i) {
      if ((type_bits & 1) == 1) {
        if ((memory_properties_.memoryTypes[i].propertyFlags &
             requirements_mask) == requirements_mask) {
          *type_index = i;
          return true;
        }
      }
      type_bits >>= 1;
    }

    return false;
  }
};
}  // namespace rtx
