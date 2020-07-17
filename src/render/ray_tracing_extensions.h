#pragma once

#include <vulkan/vulkan.h>

namespace rtx {

class ray_tracing_extensions {
 public:
  static bool load(VkDevice device);
};

}  // namespace rtx
