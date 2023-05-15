#pragma once

#include <iostream>

#include <vulkan/vulkan.h>

#include "memory.h"

namespace rtx {

class rt_descriptor_pool {
 public:
  rt_descriptor_pool() : descriptor_pool_() {}

  bool init(memory& mem) {
    static constexpr uint32_t POOL_DESCRIPTOR_COUNT = 1;
    const VkDescriptorPoolSize descriptor_pool_size[] = {
        {VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV, POOL_DESCRIPTOR_COUNT},
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, POOL_DESCRIPTOR_COUNT},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, POOL_DESCRIPTOR_COUNT},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2}};

    VkDescriptorPoolCreateInfo descriptor_pool_create_info = {};
    descriptor_pool_create_info.sType =
        VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptor_pool_create_info.pNext = nullptr;
    descriptor_pool_create_info.maxSets = POOL_DESCRIPTOR_COUNT * 1;
    descriptor_pool_create_info.poolSizeCount = 4;
    descriptor_pool_create_info.pPoolSizes = descriptor_pool_size;

    VkResult res = vkCreateDescriptorPool(
        mem.get_device(), &descriptor_pool_create_info,
        mem.get_allocation_callbacks(), &descriptor_pool_);
    if (VK_SUCCESS != res) {
      std::cerr << "Failed to create ray tracing descriptor pool: " << res
                << std::endl;
      return false;
    }

    return true;
  }

  void fini(memory& mem) {
    vkDestroyDescriptorPool(mem.get_device(), descriptor_pool_,
                            mem.get_allocation_callbacks());
    descriptor_pool_ = VK_NULL_HANDLE;
  }

  const VkDescriptorPool& pool() const { return descriptor_pool_; }

 private:

  VkDescriptorPool descriptor_pool_;
};
}  // namespace rtx
