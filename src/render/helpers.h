#pragma once

#include <iostream>
#include <vector>

#include <vulkan/vulkan.h>

#include "constants.h"
#include "memory.h"

namespace rtx {

class helpers {
 public:
  static bool find_supported_format(VkPhysicalDevice gpu,
                                    const std::vector<VkFormat> &candidates,
                                    VkImageTiling tiling,
                                    VkFormatFeatureFlags features,
                                    VkFormat &format) {
    for (VkFormat candidate_format : candidates) {
      VkFormatProperties format_properties;
      vkGetPhysicalDeviceFormatProperties(gpu, candidate_format,
                                          &format_properties);

      if (VK_IMAGE_TILING_LINEAR == tiling &&
          (format_properties.linearTilingFeatures & features) == features) {
        format = candidate_format;
        return true;
      }
      if (VK_IMAGE_TILING_OPTIMAL == tiling &&
          (format_properties.optimalTilingFeatures & features) == features) {
        format = candidate_format;
        return true;
      }
    }

    std::cerr << "Failed to find a supported format." << std::endl;
    return false;
  }

  static bool create_image(memory &mem, uint32_t width, uint32_t height,
                           VkFormat format, VkImageTiling tiling,
                           VkImageUsageFlags usage,
                           VkMemoryPropertyFlags properties, VkImage &image,
                           VkDeviceMemory &image_memory) {
    VkImageCreateInfo image_create_info{};
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.pNext = nullptr;
    image_create_info.imageType = VK_IMAGE_TYPE_2D;
    image_create_info.extent.width = width;
    image_create_info.extent.height = height;
    image_create_info.extent.depth = 1;
    image_create_info.mipLevels = 1;
    image_create_info.arrayLayers = 1;
    image_create_info.format = format;
    image_create_info.tiling = tiling;
    image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_create_info.usage = usage;
    image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_create_info.samples =
        constants::NUM_SAMPLES;  // TODO: Always VK_SAMPLE_COUNT_1_BIT?
    image_create_info.flags = 0;

    VkResult res = vkCreateImage(mem.get_device(), &image_create_info,
                                 mem.get_allocation_callbacks(), &image);
    if (VK_SUCCESS != res) {
      std::cerr << "Failed to create texture image." << std::endl;
      return false;
    }

    VkMemoryRequirements memory_requirements;
    vkGetImageMemoryRequirements(mem.get_device(), image, &memory_requirements);

    if (!mem.allocate_memory(memory_requirements, properties, image_memory)) {
      std::cerr << "Failed to allocate texture image memory." << std::endl;
      return false;
    }

    VkDeviceSize memory_offset = 0;
    vkBindImageMemory(mem.get_device(), image, image_memory, memory_offset);

    return true;
  }

  static bool create_image_view(memory &mem, VkImage image, VkFormat format,
                                VkImageAspectFlags aspect_flags,
                                VkImageView &image_view) {
    VkImageViewCreateInfo image_view_create_info{};
    image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_create_info.pNext = nullptr;
    image_view_create_info.image = image;
    image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    image_view_create_info.format = format;
    image_view_create_info.subresourceRange.aspectMask = aspect_flags;
    image_view_create_info.subresourceRange.baseMipLevel = 0;
    image_view_create_info.subresourceRange.levelCount = 1;
    image_view_create_info.subresourceRange.baseArrayLayer = 0;
    image_view_create_info.subresourceRange.layerCount = 1;
    image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_R;
    image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_G;
    image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_B;
    image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_A;

    VkResult res =
        vkCreateImageView(mem.get_device(), &image_view_create_info,
                          mem.get_allocation_callbacks(), &image_view);
    if (VK_SUCCESS != res) {
      std::cerr << "Failed to create image view." << std::endl;
      return false;
    }

    return true;
  }

  static bool transition_image_layout(VkCommandBuffer command_buffer,
                                      VkImage image, VkFormat format,
                                      VkImageLayout old_layout,
                                      VkImageLayout new_layout) {
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = old_layout;
    barrier.newLayout = new_layout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    barrier.srcAccessMask = access_flags_for_layout(old_layout);
    barrier.dstAccessMask = access_flags_for_layout(new_layout);

    VkPipelineStageFlags src_stage_mask =
        pipeline_stage_flags_for_layout(old_layout);
    VkPipelineStageFlags dst_stage_mask =
        pipeline_stage_flags_for_layout(new_layout);

    VkDependencyFlags dependency_flags = 0;
    static constexpr uint32_t memory_barrier_count = 0;
    const VkMemoryBarrier *memory_barriers = nullptr;
    static constexpr uint32_t buffer_memory_barrier_count = 0;
    const VkBufferMemoryBarrier *buffer_memory_barriers = nullptr;
    static constexpr uint32_t image_memory_barrier_count = 1;
    vkCmdPipelineBarrier(
        command_buffer, src_stage_mask, dst_stage_mask, dependency_flags,
        memory_barrier_count, memory_barriers, buffer_memory_barrier_count,
        buffer_memory_barriers, image_memory_barrier_count, &barrier);

    return true;
  }

 private:
  static VkAccessFlags access_flags_for_layout(VkImageLayout layout) {
    switch (layout) {
      case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        return VK_ACCESS_TRANSFER_WRITE_BIT;
      case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        return VK_ACCESS_TRANSFER_READ_BIT;
      case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        return VK_ACCESS_SHADER_READ_BIT;
      case VK_IMAGE_LAYOUT_PREINITIALIZED:
        return VK_ACCESS_HOST_WRITE_BIT;
      case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        return VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
      case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
        return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
      default:
        return 0;
    }
  }

  static VkPipelineStageFlags pipeline_stage_flags_for_layout(
      VkImageLayout layout) {
    switch (layout) {
      case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        return VK_PIPELINE_STAGE_TRANSFER_BIT;
      case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        return VK_PIPELINE_STAGE_TRANSFER_BIT;
      case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        return VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
      case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL:
        return VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
      case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        return VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
      case VK_IMAGE_LAYOUT_PREINITIALIZED:
        return VK_PIPELINE_STAGE_HOST_BIT;
      case VK_IMAGE_LAYOUT_UNDEFINED:
        return VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
      default:
        return VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    }
  }
};

}  // namespace rtx
