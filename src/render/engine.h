#pragma once

#include <string>
#include <vector>

#include <vulkan/vulkan.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include <glm/glm.hpp>
// perspective, translate, rotate.
#include <glm/gtc/matrix_transform.hpp>

#include "depth_buffer.h"
#include "layer_properties.h"
#include "platform.h"
#include "swap_chain_buffer.h"
#include "uniform_data.h"

namespace rtx {

static VKAPI_ATTR VkBool32 VKAPI_CALL
debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
              VkDebugUtilsMessageTypeFlagsEXT messageType,
              const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
              void *pUserData) {
  std::cerr << "Validation layer: " << pCallbackData->pMessage << std::endl;

  return VK_FALSE;
}

class render_engine {
 public:
  render_engine()
      : platform_(),
        instance_(),
        gpus_(),
        instance_layer_properties_(),
        instance_extension_names_(),
        device_extension_names_(),
        instance_layer_names_(),
        allocation_callbacks_(nullptr) {
    std::cout << "Engine: Hello World." << std::endl;
  }

  bool init(const std::string &application_name, uint32_t application_version,
            int width, int height, const std::string &title) {
    application_name_ = application_name;
    application_version_ = application_version;

    if (!init_glfw(width, height, title)) {
      std::cerr << "init_glfw() failed" << std::endl;
      return false;
    }

    if (!init_global_layer_properties()) {
      std::cerr << "init_global_layer_properties() failed:" << std::endl;
      return false;
    }

    if (!init_instance_extension_names()) {
      std::cerr << "init_instance_extension_names() failed." << std::endl;
      return false;
    }

    if (!init_device_extension_names()) {
      std::cerr << "init_device_extension_names() failed." << std::endl;
      return false;
    }

    if (!init_instance()) {
      std::cerr << "init_instance() failed." << std::endl;
      return false;
    }

    if (!init_debug_callback()) {
      std::cerr << "init_debug_callback() failed." << std::endl;
      return false;
    }

    if (!init_enumerate_device()) {
      std::cerr << "init_enumerate_device() failed." << std::endl;
      return false;
    }

    if (!platform_.create_window_surface(instance_, allocation_callbacks_,
                                         surface_)) {
      std::cerr << "platform.create_window_surface() failed." << std::endl;
      return false;
    }

    if (!init_swapchain_extension()) {
      std::cerr << "init_swapchain_extension() failed." << std::endl;
      return false;
    }

    if (!init_device()) {
      std::cerr << "init_device() failed." << std::endl;
      return false;
    }

    if (!init_command_pool()) {
      std::cerr << "init_command_pool() failed." << std::endl;
      return false;
    }

    if (!init_command_buffer()) {
      std::cerr << "init_command_buffer() failed." << std::endl;
      return false;
    }

    if (!platform_.init_framebuffer()) {
      std::cerr << "platfom.init_framebuffer() failed." << std::endl;
      return false;
    }

    if (!execute_begin_command_buffer()) {
      std::cerr << "execute_begin_command_buffer() failed." << std::endl;
      return false;
    }

    if (!init_device_queue()) {
      std::cerr << "init_device_queue() failed." << std::endl;
      return false;
    }

    if (!init_swap_chain()) {
      std::cerr << "init_swap_chain() failed." << std::endl;
      return false;
    }

    if (!init_depth_buffer()) {
      std::cerr << "init_depth_buffer() failed." << std::endl;
      return false;
    }

    if (!init_model_view_projection()) {
      std::cerr << "init_model_view_projection() failed." << std::endl;
      return false;
    }

    if (!init_uniform_buffer()) {
      std::cerr << "init_uniform_buffer() failed." << std::endl;
      return false;
    }

    return true;
  }

  bool init_glfw(int width, int height, const std::string &title) {
    return platform_.init(width, height, title);
  }

  bool init_global_layer_properties() {
    uint32_t instance_layer_count;
    VkLayerProperties *vk_props = nullptr;
    VkResult res;

    do {
      res = vkEnumerateInstanceLayerProperties(&instance_layer_count, nullptr);
      if (res != VK_SUCCESS) {
        std::cerr << "VkEnumerateInstanceLayerProperties failed." << std::endl;
        return false;
      }

      if (0 == instance_layer_count) {
        return true;
      }

      vk_props = (VkLayerProperties *)realloc(
          vk_props, instance_layer_count * sizeof(VkLayerProperties));
      res = vkEnumerateInstanceLayerProperties(&instance_layer_count, vk_props);
    } while (res == VK_INCOMPLETE);

    for (uint32_t i = 0; i < instance_layer_count; ++i) {
      layer_properties_t layer_properties;
      layer_properties.properties = vk_props[i];

      if (!init_global_extension_properties(layer_properties)) {
        return false;
      }
      instance_layer_properties_.push_back(layer_properties);
    }
    free(vk_props);

    return true;
  }

  bool init_instance_extension_names() {
    // Get GLFW extensions
    uint32_t platform_extensions_count = 0;
    const char **platform_extensions = nullptr;
    if (!platform_.get_vulkan_extensions(platform_extensions_count,
                                         platform_extensions)) {
      std::cerr << "Getting platform required extensions failed." << std::endl;
      return false;
    }
    for (uint32_t i = 0; i < platform_extensions_count; ++i) {
      instance_extension_names_.push_back(platform_extensions[i]);
    }

    // Validation layer
    instance_extension_names_.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    std::cout << "Required instance extensions:" << std::endl;
    for (auto &e : instance_extension_names_) {
      std::cout << "- " << e << std::endl;
    }

      return true;
    }

    bool init_device_extension_names() {
      device_extension_names_.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

      std::cout << "Required device extensions:" << std::endl;
      for (auto &e : device_extension_names_) {
        std::cout << "- " << e << std::endl;
      }
      return true;
    }

    bool init_instance() {
      VkApplicationInfo application_info = {};
      application_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
      application_info.pNext = nullptr;
      application_info.pApplicationName = application_name_.c_str();
      application_info.applicationVersion = application_version_;
      application_info.pEngineName = engine_name_.c_str();
      application_info.engineVersion = engine_version_;
      application_info.apiVersion = VK_API_VERSION_1_1;

      VkInstanceCreateInfo instance_create_info = {};
      instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
      instance_create_info.pNext = nullptr;
      instance_create_info.flags = 0;
      instance_create_info.pApplicationInfo = &application_info;
      instance_create_info.enabledLayerCount = instance_layer_names_.size();
      instance_create_info.ppEnabledLayerNames =
          instance_layer_names_.size() ? instance_layer_names_.data() : nullptr;
      instance_create_info.enabledExtensionCount =
          instance_extension_names_.size();
      instance_create_info.ppEnabledExtensionNames =
          instance_extension_names_.data();

      VkResult res = vkCreateInstance(&instance_create_info,
                                      allocation_callbacks_, &instance_);
      if (VK_SUCCESS != res) {
        std::cerr << "Failed to create vulkan instance." << std::endl;
        return false;
      }

      return true;
    }

    bool init_debug_callback() {
      VkDebugUtilsMessengerCreateInfoEXT createInfo{};

      createInfo.sType =
          VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;

      createInfo.messageSeverity =
          VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
          VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
          VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

      createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

      createInfo.pfnUserCallback = debugCallback;

      createInfo.pUserData = nullptr;

      PFN_vkCreateDebugUtilsMessengerEXT f =
          (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
              instance_, "vkCreateDebugUtilsMessengerEXT");
      f(instance_, &createInfo, nullptr, &debugMessenger_);

      return true;
    }

    bool init_enumerate_device() {
      uint32_t gpu_count;
      VkResult res = vkEnumeratePhysicalDevices(instance_, &gpu_count, nullptr);

      if (0 == gpu_count) {
        std::cerr << "No GPUs available." << std::endl;
        return false;
      }

      gpus_.resize(gpu_count);

      res = vkEnumeratePhysicalDevices(instance_, &gpu_count, gpus_.data());
      if (VK_SUCCESS != res || 0 == gpu_count) {
        std::cerr << "Failed to enumerate GPUs." << std::endl;
        return false;
      }

      vkGetPhysicalDeviceQueueFamilyProperties(gpus_[0], &queue_family_count_,
                                               nullptr);
      if (0 == queue_family_count_) {
        std::cerr << "No queue families." << std::endl;
        return false;
      }
      queue_props_.resize(queue_family_count_);
      vkGetPhysicalDeviceQueueFamilyProperties(gpus_[0], &queue_family_count_,
                                               queue_props_.data());
      if (0 == queue_family_count_) {
        std::cerr << "Failed to get queue families." << std::endl;
        return false;
      }

      vkGetPhysicalDeviceMemoryProperties(gpus_[0], &memory_properties_);
      vkGetPhysicalDeviceProperties(gpus_[0], &gpu_properties_);
      for (auto &layer_properties : instance_layer_properties_) {
        if (!init_device_extension_properties(layer_properties)) {
          std::cerr << "Failed to init device extension property "
                    << layer_properties.properties.layerName << "."
                    << std::endl;
          return false;
        }
      }
      return true;
    }

    bool init_device_extension_properties(
        layer_properties_t &layer_properties) {
      char *layer_name = layer_properties.properties.layerName;
      uint32_t device_extension_count;
      VkResult res;

      do {
        res = vkEnumerateDeviceExtensionProperties(
            gpus_[0], layer_name, &device_extension_count, nullptr);
        if (res) {
          return false;
        }

        if (0 == device_extension_count) {
          return true;
        }

        layer_properties.device_extensions.resize(device_extension_count);
        res = vkEnumerateDeviceExtensionProperties(
            gpus_[0], layer_name, &device_extension_count,
            layer_properties.device_extensions.data());

      } while (res == VK_INCOMPLETE);

      std::cout << "Layer property " << layer_name
                << " requires extension properties:" << std::endl;
      for (auto &e : layer_properties.device_extensions) {
        std::cout << "- " << e.extensionName << std::endl;
      }

      return true;
    }

    bool init_swapchain_extension() {
      // Find queues that support present.
      VkBool32 *pSupportsPresent =
          (VkBool32 *)malloc(queue_family_count_ * sizeof(VkBool32));
      if (!pSupportsPresent) {
        std::cerr << "Failed to reserve memory for queue families that support "
                     "present mode."
                  << std::endl;
        return false;
      }
      for (uint32_t i = 0; i < queue_family_count_; i++) {
        vkGetPhysicalDeviceSurfaceSupportKHR(gpus_[0], i, surface_,
                                             &pSupportsPresent[i]);
      }

      // Find a graphics and a present queue.
      // Try to find a queue that supports both.
      graphics_queue_family_index_ = UINT32_MAX;
      present_queue_family_index_ = UINT32_MAX;
      for (uint32_t i = 0; i < queue_family_count_; ++i) {
        if ((queue_props_[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0) {
          if (graphics_queue_family_index_ == UINT32_MAX) {
            graphics_queue_family_index_ = i;
          }

          if (pSupportsPresent[i] == VK_TRUE) {
            graphics_queue_family_index_ = i;
            present_queue_family_index_ = i;
            break;
          }
        }
      }

      if (present_queue_family_index_ == UINT32_MAX) {
        // If there is no queue that supports both graphics and present, find a
        // queue that supports present
        for (uint32_t i = 0; i < queue_family_count_; ++i) {
          if (pSupportsPresent[i] == VK_TRUE) {
            present_queue_family_index_ = i;
            break;
          }
        }
      }

      free(pSupportsPresent);

      if (graphics_queue_family_index_ == UINT32_MAX ||
          present_queue_family_index_ == UINT32_MAX) {
        std::cerr << "No queues for graphics and present." << std::endl;
        return false;
      }

      // TODO: Move somewhere else?
      uint32_t format_count;
      VkResult res = vkGetPhysicalDeviceSurfaceFormatsKHR(
          gpus_[0], surface_, &format_count, nullptr);
      if (res != VK_SUCCESS) {
        std::cerr << "Failed to get count of surface formats." << std::endl;
        return false;
      }

      VkSurfaceFormatKHR *surface_formats = (VkSurfaceFormatKHR *)malloc(
          format_count * sizeof(VkSurfaceFormatKHR));
      if (!surface_formats) {
        std::cerr << "Failed to reserve memory for surface formats."
                  << std::endl;
        return false;
      }
      res = vkGetPhysicalDeviceSurfaceFormatsKHR(
          gpus_[0], surface_, &format_count, surface_formats);
      if (res != VK_SUCCESS) {
        std::cerr << "Failed to get surface formats." << std::endl;
        free(surface_formats);
        return false;
      }
      if (1 == format_count &&
          VK_FORMAT_UNDEFINED == surface_formats[0].format) {
        format_ = VK_FORMAT_B8G8R8A8_UNORM;
      } else {
        if (0 == format_count) {
          std::cerr << "No surface formats supported." << std::endl;
          free(surface_formats);
          return false;
        }
        format_ = surface_formats[0].format;
      }

      free(surface_formats);

      return true;
    }

    bool init_device() {
      VkDeviceQueueCreateInfo device_queue_create_info = {};
      float queue_priorities[1] = {0.0};
      device_queue_create_info.sType =
          VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
      device_queue_create_info.pNext = nullptr;
      device_queue_create_info.queueCount = 1;
      device_queue_create_info.pQueuePriorities = queue_priorities;
      device_queue_create_info.queueFamilyIndex = graphics_queue_family_index_;

      VkDeviceCreateInfo device_create_info = {};
      device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
      device_create_info.pNext = nullptr;
      device_create_info.queueCreateInfoCount = 1;
      device_create_info.pQueueCreateInfos = &device_queue_create_info;
      device_create_info.enabledExtensionCount = device_extension_names_.size();
      device_create_info.ppEnabledExtensionNames =
          device_create_info.enabledExtensionCount
              ? device_extension_names_.data()
              : nullptr;
      device_create_info.pEnabledFeatures = nullptr;

      VkResult res = vkCreateDevice(gpus_[0], &device_create_info,
                                    allocation_callbacks_, &device_);
      if (res != VK_SUCCESS) {
        std::cerr << "Failed to create device." << std::endl;
        return false;
      }

      return true;
    }

    bool init_command_pool() {
      VkCommandPoolCreateInfo command_pool_create_info = {};
      command_pool_create_info.sType =
          VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
      command_pool_create_info.pNext = nullptr;
      command_pool_create_info.queueFamilyIndex = graphics_queue_family_index_;
      command_pool_create_info.flags =
          VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

      VkResult res = vkCreateCommandPool(device_, &command_pool_create_info,
                                         allocation_callbacks_, &command_pool_);
      if (VK_SUCCESS != res) {
        std::cerr << "Failed to create command pool." << std::endl;
        return false;
      }

      return true;
    }

    bool init_command_buffer() {
      VkCommandBufferAllocateInfo command_buffer_allocate_info = {};
      command_buffer_allocate_info.sType =
          VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
      command_buffer_allocate_info.pNext = nullptr;
      command_buffer_allocate_info.commandPool = command_pool_;
      command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
      command_buffer_allocate_info.commandBufferCount = 1;

      VkResult res = vkAllocateCommandBuffers(
          device_, &command_buffer_allocate_info, &command_buffer_);
      if (VK_SUCCESS != res) {
        std::cerr << "Failed to create command buffer." << std::endl;
        return false;
      }

      return true;
    }

    bool execute_begin_command_buffer() {
      VkCommandBufferBeginInfo command_buffer_begin_info = {};
      command_buffer_begin_info.sType =
          VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
      command_buffer_begin_info.pNext = nullptr;
      command_buffer_begin_info.flags = 0;
      command_buffer_begin_info.pInheritanceInfo = nullptr;

      VkResult res =
          vkBeginCommandBuffer(command_buffer_, &command_buffer_begin_info);
      if (VK_SUCCESS != res) {
        std::cerr << "Failed to begin command buffer." << std::endl;
        return false;
      }

      return true;
    }

    bool init_device_queue() {
      uint32_t queue_index = 0;
      vkGetDeviceQueue(device_, graphics_queue_family_index_, queue_index,
                       &graphics_queue_);

      if (graphics_queue_family_index_ == present_queue_family_index_) {
        present_queue_ = graphics_queue_;
      } else {
        vkGetDeviceQueue(device_, present_queue_family_index_, queue_index,
                         &present_queue_);
      }
      return true;
    }

    bool init_swap_chain() {

      VkSurfaceCapabilitiesKHR surface_capabilities;
      VkResult res = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
          gpus_[0], surface_, &surface_capabilities);
      if (VK_SUCCESS != res) {
        std::cerr << "Failed to get surface capabilities." << std::endl;
        return false;
      }

      uint32_t present_mode_count;
      res = vkGetPhysicalDeviceSurfacePresentModesKHR(
          gpus_[0], surface_, &present_mode_count, nullptr);
      if (VK_SUCCESS != res) {
        std::cerr << "Failed to get surface present modes count." << std::endl;
        return false;
      }
      VkPresentModeKHR *present_modes = (VkPresentModeKHR *)malloc(
          present_mode_count * sizeof(VkPresentModeKHR));
      if (!present_modes) {
        std::cerr << "Failed to reserve memory fro present modes."
                  << std ::endl;
        return false;
      }
      res = vkGetPhysicalDeviceSurfacePresentModesKHR(
          gpus_[0], surface_, &present_mode_count, present_modes);
      if (VK_SUCCESS != res) {
        std::cerr << "Failed to get surface present modes." << std::endl;
        free(present_modes);
        return false;
      }

      VkExtent2D swap_chain_extent;
      // width and height are either both 0xFFFFFFFF, or both not 0xFFFFFFFF.
      if (surface_capabilities.currentExtent.width == 0xFFFFFFFF) {
        // The surface size is undefined, set the same size of the frame.
        swap_chain_extent = platform_.window_size();

        swap_chain_extent.width = std::max(
            swap_chain_extent.width, surface_capabilities.minImageExtent.width);
        swap_chain_extent.width = std::min(
            swap_chain_extent.width, surface_capabilities.maxImageExtent.width);

        swap_chain_extent.height =
            std::max(swap_chain_extent.height,
                     surface_capabilities.minImageExtent.height);
        swap_chain_extent.height =
            std::min(swap_chain_extent.height,
                     surface_capabilities.maxImageExtent.height);
      } else {
        // The surface size is defined.
        swap_chain_extent = surface_capabilities.currentExtent;
      }

      VkPresentModeKHR swap_chain_present_mode = VK_PRESENT_MODE_FIFO_KHR;
      // Try to use the less laggy mailbox.
      for (uint32_t i = 0; i < present_mode_count; ++i) {
        if (present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
          std::cout << "Present mode: mailbox." << std::endl;
          swap_chain_present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
          break;
        }
      }
      free(present_modes);

      // Try to use triple buffering
      uint32_t triple_buffering = 3;
      uint32_t desired_number_of_swap_chain_images =
          std::max(surface_capabilities.minImageCount, triple_buffering);
      desired_number_of_swap_chain_images =
          std::min(desired_number_of_swap_chain_images,
                   surface_capabilities.maxImageCount);
      std::cout << "Buffering images: " << desired_number_of_swap_chain_images
                << "." << std::endl;

      // Try to not rotate the image.
      VkSurfaceTransformFlagBitsKHR pre_transform;
      if (surface_capabilities.supportedTransforms &
          VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
        pre_transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
      } else {
        pre_transform = surface_capabilities.currentTransform;
      }

      // Find a supported composite mode.
      VkCompositeAlphaFlagBitsKHR composite_alpha =
          VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
      VkCompositeAlphaFlagBitsKHR composite_alpha_flags[4] = {
          VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
          VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
          VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
          VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
      };
      for (uint32_t i = 0; i < sizeof(composite_alpha_flags); ++i) {
        if (surface_capabilities.supportedCompositeAlpha &
            composite_alpha_flags[i]) {
          composite_alpha = composite_alpha_flags[i];
          break;
        }
      }

      VkSwapchainCreateInfoKHR swap_chain_create_info = {};
      swap_chain_create_info.sType =
          VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
      swap_chain_create_info.pNext = nullptr;
      swap_chain_create_info.surface = surface_;
      swap_chain_create_info.minImageCount =
          desired_number_of_swap_chain_images;
      swap_chain_create_info.imageFormat = format_;
      swap_chain_create_info.imageExtent.width = swap_chain_extent.width;
      swap_chain_create_info.imageExtent.height = swap_chain_extent.height;
      swap_chain_create_info.preTransform = pre_transform;
      swap_chain_create_info.compositeAlpha = composite_alpha;
      swap_chain_create_info.imageArrayLayers = 1;  // VR would be a 2.
      swap_chain_create_info.presentMode = swap_chain_present_mode;
      swap_chain_create_info.oldSwapchain = VK_NULL_HANDLE;
      swap_chain_create_info.clipped = VK_FALSE;
      swap_chain_create_info.imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
      swap_chain_create_info.imageUsage =
          VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

      swap_chain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
      swap_chain_create_info.queueFamilyIndexCount = 0;
      swap_chain_create_info.pQueueFamilyIndices = nullptr;
      uint32_t queue_family_indices[2] = {graphics_queue_family_index_,
                                          present_queue_family_index_};
      if (graphics_queue_family_index_ != present_queue_family_index_) {
        std::cout << "swap chain with sharing mode." << std::endl;
        swap_chain_create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swap_chain_create_info.queueFamilyIndexCount = 2;
        swap_chain_create_info.pQueueFamilyIndices = queue_family_indices;
      }

      res = vkCreateSwapchainKHR(device_, &swap_chain_create_info, nullptr,
                                 &swap_chain_);
      if (VK_SUCCESS != res) {
        std::cerr << "Failed to create swap chain." << std::endl;
        return false;
      }

      res = vkGetSwapchainImagesKHR(device_, swap_chain_,
                                    &swap_chain_image_count_, nullptr);
      if (VK_SUCCESS != res) {
        std::cerr << "Failed to get swap chain image count." << std::endl;
        return false;
      }

      VkImage *swap_chain_images =
          (VkImage *)malloc(swap_chain_image_count_ * sizeof(VkImage));
      if (!swap_chain_images) {
        std::cerr << "Failed to reserve memory for swap chain images."
                  << std::endl;
        return false;
      }
      res = vkGetSwapchainImagesKHR(
          device_, swap_chain_, &swap_chain_image_count_, swap_chain_images);
      if (VK_SUCCESS != res) {
        std::cerr << "Failed to get swap chain images." << std::endl;
        return false;
      }

      for (uint32_t i = 0; i < swap_chain_image_count_; ++i) {
        swap_chain_buffer_t swap_chain_buffer;

        VkImageViewCreateInfo color_image_view_create_info = {};
        color_image_view_create_info.sType =
            VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        color_image_view_create_info.pNext = nullptr;
        color_image_view_create_info.format = format_;
        color_image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_R;
        color_image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_G;
        color_image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_B;
        color_image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_A;
        color_image_view_create_info.subresourceRange.aspectMask =
            VK_IMAGE_ASPECT_COLOR_BIT;
        color_image_view_create_info.subresourceRange.baseMipLevel = 0;
        color_image_view_create_info.subresourceRange.levelCount = 1;
        color_image_view_create_info.subresourceRange.baseArrayLayer = 0;
        color_image_view_create_info.subresourceRange.layerCount = 1;
        color_image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        color_image_view_create_info.flags = 0;

        swap_chain_buffer.image = swap_chain_images[i];

        color_image_view_create_info.image = swap_chain_buffer.image;

        res = vkCreateImageView(device_, &color_image_view_create_info, nullptr,
                                &swap_chain_buffer.view);
        if (VK_SUCCESS != res) {
          std::cerr << "Failed to create image view." << std::endl;
          free(swap_chain_images);
          return false;
        }

        buffers_.push_back(swap_chain_buffer);
      }
      free(swap_chain_images);
      current_buffer_ = 0;

      return true;
    }

    bool init_depth_buffer() {

      VkImageCreateInfo image_create_info = {};

      const VkFormat depth_format = VK_FORMAT_D16_UNORM;

      VkFormatProperties format_properties;
      vkGetPhysicalDeviceFormatProperties(gpus_[0], depth_format,
                                          &format_properties);
      if (format_properties.linearTilingFeatures &
          VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
        image_create_info.tiling = VK_IMAGE_TILING_LINEAR;
      } else if (format_properties.optimalTilingFeatures &
                 VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
        image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
      } else {
        std::cerr << "Depth format " << depth_format << " unsupported."
                  << std::endl;
        return false;
      }

      image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
      image_create_info.pNext = nullptr;
      image_create_info.imageType = VK_IMAGE_TYPE_2D;
      image_create_info.format = depth_format;
      VkExtent2D window_size = platform_.window_size();
      image_create_info.extent.width = window_size.width;
      image_create_info.extent.height = window_size.height;
      std::cout << "Depth buffer image size (" << image_create_info.extent.width
                << "x" << image_create_info.extent.height << ")." << std::endl;
      image_create_info.extent.depth = 1;
      image_create_info.mipLevels = 1;
      image_create_info.arrayLayers = 1;
      image_create_info.samples = NUM_SAMPLES;
      image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      image_create_info.queueFamilyIndexCount = 0;
      image_create_info.pQueueFamilyIndices = nullptr;
      image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
      image_create_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
      image_create_info.flags = 0;

      VkMemoryAllocateInfo memory_allocate_info = {};
      memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
      memory_allocate_info.pNext = nullptr;
      memory_allocate_info.allocationSize = 0;
      memory_allocate_info.memoryTypeIndex = 0;

      VkImageViewCreateInfo image_view_create_info = {};
      image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
      image_view_create_info.pNext = nullptr;
      image_view_create_info.image = VK_NULL_HANDLE;
      image_view_create_info.format = depth_format;
      image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_R;
      image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_G;
      image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_B;
      image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_A;
      image_view_create_info.subresourceRange.aspectMask =
          VK_IMAGE_ASPECT_DEPTH_BIT;
      image_view_create_info.subresourceRange.baseMipLevel = 0;
      image_view_create_info.subresourceRange.levelCount = 1;
      image_view_create_info.subresourceRange.baseArrayLayer = 0;
      image_view_create_info.subresourceRange.layerCount = 1;
      image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
      image_view_create_info.flags = 0;

      if (depth_format == VK_FORMAT_D16_UNORM_S8_UINT ||
          depth_format == VK_FORMAT_D24_UNORM_S8_UINT ||
          depth_format == VK_FORMAT_D32_SFLOAT_S8_UINT) {
        image_view_create_info.subresourceRange.aspectMask |=
            VK_IMAGE_ASPECT_STENCIL_BIT;
      }

      depth_buffer_.format = depth_format;

      VkResult res = vkCreateImage(device_, &image_create_info,
                                   allocation_callbacks_, &depth_buffer_.image);
      if (VK_SUCCESS != res) {
        std::cerr << "Failed to create depth buffer image." << std::endl;
        return false;
      }

      VkMemoryRequirements memory_requirements;
      vkGetImageMemoryRequirements(device_, depth_buffer_.image,
                                   &memory_requirements);

      memory_allocate_info.allocationSize = memory_requirements.size;

      if (!memory_type_from_properties(memory_requirements.memoryTypeBits,
                                       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                       &memory_allocate_info.memoryTypeIndex)) {
        std::cerr
            << "Failed to get memory type properties of depth buffer image."
            << std::endl;
        return false;
      }

      // Allocate memory of depth buffer image.
      res = vkAllocateMemory(device_, &memory_allocate_info,
                             allocation_callbacks_, &depth_buffer_.mem);
      if (VK_SUCCESS != res) {
        std::cerr << "Failed to allocate memory for depth buffer image."
                  << std::endl;
        return false;
      }

      //  Bind memory allocated of depth buffer image to the image.
      VkDeviceSize memory_offset = 0;
      res = vkBindImageMemory(device_, depth_buffer_.image, depth_buffer_.mem,
                              memory_offset);
      if (VK_SUCCESS != res) {
        std::cerr << "Failed to bind memory for depth buffer image."
                  << std::endl;
        if (VK_ERROR_OUT_OF_HOST_MEMORY == res) {
          std::cerr << "VK_ERROR_OUT_OF_HOST_MEMORY" << std::endl;
        }
        if (VK_ERROR_OUT_OF_DEVICE_MEMORY == res) {
          std::cerr << "VK_ERROR_OUT_OF_DEVICE_MEMORY" << std::endl;
        }
        std::cerr << res << std::endl;
        return false;
      }

      // Create image view.
      image_view_create_info.image = depth_buffer_.image;

      res = vkCreateImageView(device_, &image_view_create_info,
                              allocation_callbacks_, &depth_buffer_.view);
      if (VK_SUCCESS != res) {
        std::cerr << "Failed to create depth buffer image view." << std::endl;
        return false;
      }

      return true;
    }

    bool init_model_view_projection() {
      // fov_ = glm::radians(45.0f);
      fov_ = 45.0f;

      VkExtent2D window = platform_.window_size();
      // if (window.width > window.height) {
      //  fov_ *= static_cast<float>(window.height) /
      //          static_cast<float>(window.width);
      //}

      // Projection
      float aspect_ratio =
          static_cast<float>(window.width) / static_cast<float>(window.height);
      float near_distance = 0.001f;
      float far_distance = 100.0f;
      projection_ = glm::perspective(glm::radians(fov_), aspect_ratio,
                                     near_distance, far_distance);

      // Look At
      auto eye = glm::vec3(-5.0, 3.0, -10.0);
      auto center = glm::vec3(0.0, 0.0, 0.0);
      auto up = glm::vec3(0.0, -1.0, 0.0);
      view_ = glm::lookAt(eye, center, up);

      // Model
      model_ = glm::mat4(1.0f);

      // Clip
      // Vulkan clip space has inverted Y and half Z.
      clip_ = glm::mat4(1.0f, 0.0f, 0.0f, 0.0f,   //
                        0.0f, -1.0f, 0.0f, 0.0f,  //
                        0.0f, 0.0f, 0.5f, 0.0f,   //
                        0.0f, 0.0f, 0.5f, 1.0f    //
      );

      mvp_ = clip_ * projection_ * view_ * model_;

      return true;
    }

    bool init_uniform_buffer() {
      VkBufferCreateInfo buffer_create_info = {};
      buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
      buffer_create_info.pNext = nullptr;
      buffer_create_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
      buffer_create_info.size = sizeof(mvp_);
      buffer_create_info.queueFamilyIndexCount = 0;
      buffer_create_info.pQueueFamilyIndices = nullptr;
      buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
      buffer_create_info.flags = 0;

      VkResult res = vkCreateBuffer(device_, &buffer_create_info,
                                    allocation_callbacks_, &uniform_data_.buf);
      if (VK_SUCCESS != res) {
        std::cerr << "Failed to create uniform buffer." << std::endl;
        return false;
      }

      VkMemoryRequirements memory_requirements;
      vkGetBufferMemoryRequirements(device_, uniform_data_.buf,
                                    &memory_requirements);

      VkMemoryAllocateInfo memory_allocate_info = {};
      memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
      memory_allocate_info.pNext = nullptr;
      memory_allocate_info.memoryTypeIndex = 0;
      memory_allocate_info.allocationSize = memory_requirements.size;

      // From vulkan tutorial:
      // https://vulkan.lunarg.com/doc/sdk/1.2.141.2/linux/tutorial/html/07-init_uniform_buffer.html
      //
      // The VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT communicates that the memory
      // should be mapped so that the CPU (host) can access it.
      //
      // The VK_MEMORY_PROPERTY_HOST_COHERENT_BIT requests that the writes to
      // the memory by the host are visible to the device (and vice-versa)
      // without the need to flush memory caches. This just makes it a bit
      // simpler to program, since it isn't necessary to call
      // vkFlushMappedMemoryRanges and vkInvalidateMappedMemoryRanges to make
      // sure that the data is visible to the GPU.
      if (!memory_type_from_properties(memory_requirements.memoryTypeBits,
                                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                           VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                       &memory_allocate_info.memoryTypeIndex)) {
        std::cerr << "Failed to get memory type properties of uniform buffer."
                  << std::endl;
        return false;
      }

      res = vkAllocateMemory(device_, &memory_allocate_info,
                             allocation_callbacks_, &uniform_data_.mem);
      if (VK_SUCCESS != res) {
        std::cerr << "Failed to allocate memory for uniform buffer."
                  << std::endl;
        return false;
      }

      uint8_t *pointer_uniform_data;
      VkDeviceSize offset = 0;
      VkMemoryMapFlags flags = 0;
      res = vkMapMemory(device_, uniform_data_.mem, offset,
                        memory_requirements.size, flags,
                        (void **)&pointer_uniform_data);
      if (VK_SUCCESS != res) {
        std::cerr << "Failed to map uniform buffer to CPU memory." << std::endl;
        return false;
      }

      memcpy(pointer_uniform_data, &mvp_, sizeof(mvp_));

      vkUnmapMemory(device_, uniform_data_.mem);

      VkDeviceSize memory_offset = 0;
      res = vkBindBufferMemory(device_, uniform_data_.buf, uniform_data_.mem,
                               memory_offset);
      if (VK_SUCCESS != res) {
        std::cerr << "Failed to bind uniform buffer memory." << std::endl;
        return false;
      }

      uniform_data_.buffer_info.buffer = uniform_data_.buf;
      uniform_data_.buffer_info.offset = 0;
      uniform_data_.buffer_info.range = sizeof(mvp_);

      return true;
    }

   private:
    platform platform_;
    VkInstance instance_;
    VkDebugUtilsMessengerEXT debugMessenger_;
    std::vector<VkPhysicalDevice> gpus_;
    std::vector<VkQueueFamilyProperties> queue_props_;
    VkPhysicalDeviceMemoryProperties memory_properties_;
    VkDevice device_;
    VkQueue graphics_queue_;
    VkQueue present_queue_;
    uint32_t graphics_queue_family_index_;
    uint32_t present_queue_family_index_;
    VkPhysicalDeviceProperties gpu_properties_;

    VkFormat format_;

    uint32_t swap_chain_image_count_;
    VkSwapchainKHR swap_chain_;
    std::vector<swap_chain_buffer_t> buffers_;

    VkCommandPool command_pool_;
    VkCommandBuffer command_buffer_;

    depth_buffer_t depth_buffer_;

    uniform_data_t uniform_data_;

    glm::mat4 projection_;
    glm::mat4 view_;
    glm::mat4 model_;
    glm::mat4 clip_;
    glm::mat4 mvp_;
    float fov_;

    std::vector<layer_properties_t> instance_layer_properties_;
    std::vector<const char *> instance_extension_names_;
    std::vector<const char *> device_extension_names_;
    std::vector<const char *> instance_layer_names_;

    VkSurfaceKHR surface_;
    ImGui_ImplVulkanH_Window imgui_window_;

    uint32_t current_buffer_;
    uint32_t queue_family_count_;

    const VkAllocationCallbacks *allocation_callbacks_;

    std::string application_name_;
    uint32_t application_version_;
    const std::string engine_name_ = "rtx_engine";
    static constexpr uint32_t engine_version_ = 1;

    static const VkSampleCountFlagBits NUM_SAMPLES = VK_SAMPLE_COUNT_1_BIT;

    static bool init_global_extension_properties(
        layer_properties_t &layer_properties) {
      char *layer_name = layer_properties.properties.layerName;
      VkResult res;
      uint32_t instance_extension_count;

      do {
        res = vkEnumerateInstanceExtensionProperties(
            layer_name, &instance_extension_count, nullptr);
        if (res != VK_SUCCESS) {
          std::cerr << "vkEnumerateInstanceExtensionProperties of layer "
                    << layer_name << " failed." << std::endl;
          return false;
        }

        if (0 == instance_extension_count) {
          return true;
        }

        layer_properties.instance_extensions.resize(instance_extension_count);
        VkExtensionProperties *instance_extensions =
            layer_properties.instance_extensions.data();

        res = vkEnumerateInstanceExtensionProperties(
            layer_name, &instance_extension_count, instance_extensions);

      } while (res == VK_INCOMPLETE);

      return res == VK_SUCCESS;
    }

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
