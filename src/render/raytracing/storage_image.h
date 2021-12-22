#pragma once

#include <iostream>
#include <vector>

#include <vulkan/vulkan.h>

#include "helpers.h"
#include "memory.h"

namespace rtx {

// Storage image to where the ray tracing shaders will write to.
//
class storage_image {
 public:
  storage_image() = default;

  // A suitable image format must be found with storage_image::find_format().
  bool init(memory &mem, VkCommandBuffer command_buffer, VkExtent2D window_size,
            VkFormat format) {
    format_ = format;

    if (!helpers::create_image(
            mem, window_size.width, window_size.height, format,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                VK_IMAGE_USAGE_STORAGE_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image_, mem_)) {
      std::cerr << "Failed to create ray tracing storage image." << std::endl;
      return false;
    }

    if (!helpers::create_image_view(mem, image_, format,
                                    VK_IMAGE_ASPECT_COLOR_BIT, view_)) {
      std::cerr << "Failed to create ray tracing storage image view."
                << std::endl;
      return false;
    }

    if (!helpers::transition_image_layout(command_buffer, image_, format,
                                          VK_IMAGE_LAYOUT_UNDEFINED,
                                          image_layout_)) {
      std::cerr << "Failed to transit ray tracing storage image layout."
                << std::endl;
      return false;
    }

    return true;
  }

  void fini(memory &mem) {
    vkDestroyImageView(mem.get_device(), view_, mem.get_allocation_callbacks());
    view_ = VK_NULL_HANDLE;

    vkDestroyImage(mem.get_device(), image_, mem.get_allocation_callbacks());
    image_ = VK_NULL_HANDLE;

    vkFreeMemory(mem.get_device(), mem_, mem.get_allocation_callbacks());
    mem_ = VK_NULL_HANDLE;
  }

  const VkImage &image() const { return image_; }
  const VkImageView &view() const { return view_; }
  const VkFormat &format() const { return format_; }
  const VkImageLayout &image_layout() const { return image_layout_; }

  static bool find_format(VkPhysicalDevice gpu, VkFormat &format) {
    static const std::vector<VkFormat> candidates{
        VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM,
        VK_FORMAT_R8G8B8A8_SRGB, VK_FORMAT_B8G8R8A8_UNORM};
    static const VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
    static const VkFormatFeatureFlags features =
        VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT;

    // format = VK_FORMAT_B8G8R8A8_UNORM;
    return helpers::find_supported_format(gpu, candidates, tiling, features,
                                          format);
  }

 private:
  VkDeviceMemory mem_;
  VkImage image_;
  VkImageView view_;
  VkFormat format_;

  static const VkImageLayout image_layout_ = VK_IMAGE_LAYOUT_GENERAL;
};

}  // namespace rtx
