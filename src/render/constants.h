#pragma once

#include <vulkan/vulkan.h>

namespace rtx {

class constants {
 public:
  static const VkSampleCountFlagBits NUM_SAMPLES = VK_SAMPLE_COUNT_1_BIT;

  // Number of descriptor sets needs to be the same at alloc, pipeline layout
  // creation, and descriptor set layout creation.
  static constexpr int NUM_DESCRIPTOR_SETS = 1;

  // Number of viewports and number of scissors have to be the same at
  // pipeline creation and in any call to set them dynamically.
  static constexpr int NUM_VIEWPORTS_AND_SCISSORS = 1;

  // Number of frames that can be drawed concurrently.
  static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;
};

}  // namespace rtx
