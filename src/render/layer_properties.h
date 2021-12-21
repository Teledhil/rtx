#pragma once

#include <vector>

#include <vulkan/vulkan.h>

/*
 * A layer can expose extensions, keep track of those
 * extensions here.
 */
typedef struct {
  VkLayerProperties properties;
  std::vector<VkExtensionProperties> instance_extensions;
  std::vector<VkExtensionProperties> device_extensions;
} layer_properties_t;
