#pragma once

#include <iostream>
#include <vector>

#include <vulkan/vulkan.h>

#include "memory.h"

namespace rtx {

class descriptor_sets {
 public:
  bool init_pool(const memory& mem,
                 const std::vector<VkDescriptorPoolSize>& pool_sizes) {
    // static constexpr uint32_t pool_descriptor_count = 1;
    // VkDescriptorPoolSize descriptor_pool_size[] = {
    //    {VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV, pool_descriptor_count},
    //    {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, pool_descriptor_count},
    //    {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, pool_descriptor_count},
    //    {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2}};

    uint32_t max_sets = 0;
    for (auto& pool : pool_sizes) {
      max_sets += pool.descriptorCount;
    }

    VkDescriptorPoolCreateInfo descriptor_pool_create_info = {};
    descriptor_pool_create_info.sType =
        VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptor_pool_create_info.pNext = nullptr;
    descriptor_pool_create_info.maxSets = max_sets;
    descriptor_pool_create_info.poolSizeCount =
        std::static_cast<uint32_t>(pool_sizes.size());
    descriptor_pool_create_info.pPoolSizes = pool_sizes.data();

    VkResult res = vkCreateDescriptorPool(
        mem.get_device(), &descriptor_pool_create_info,
        mem.get_allocation_callbacks(), &descriptor_pool_);
    if (VK_SUCCESS != res) {
      std::cerr << "Failed to create ray tracing descriptor pool." << std::endl;
      return false;
    }

    return true;
  }

  void fini_pool(const memory& mem) {
    vkDestroyDescriptorPool(mem.get_device(), descriptor_pool_,
                            mem.get_allocation_callbacks());
    descriptor_pool_ = VK_NULL_HANDLE;
  }

 private:
  VkDescriptorPool descriptor_pool_;
};

}  // namespace rtx
