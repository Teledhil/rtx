#pragma once

#include <algorithm>
#include <string>
#include <vector>

#include <vulkan/vulkan.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include <glm/glm.hpp>
// perspective, translate, rotate.
#include <glm/gtc/matrix_transform.hpp>

#include "cube.h"
#include "depth_buffer.h"
#include "layer_properties.h"
#include "platform.h"
#include "swap_chain_buffer.h"
#include "texture_data.h"
#include "uniform_data.h"
#include "vertex_buffer.h"

// shaders
#include "draw_cube.frag.h"
#include "draw_cube.vert.h"

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
  render_engine(bool debug = false)
      : platform_(),
        instance_(),
        gpus_(),
        instance_layer_properties_(),
        instance_extension_names_(),
        device_extension_names_(),
        instance_layer_names_(),
        allocation_callbacks_(nullptr),
        enable_validation_layer_(debug) {
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

    if (enable_validation_layer_) {
      if (!init_debug_callback()) {
        std::cerr << "init_debug_callback() failed." << std::endl;
        return false;
      }
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

    if (!platform_.init_framebuffer()) {
      std::cerr << "platfom.init_framebuffer() failed." << std::endl;
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

    if (!init_descriptor_layout()) {
      std::cerr << "init_descriptor_layout() failed." << std::endl;
      return false;
    }

    if (!init_render_pass()) {
      std::cerr << "init_render_pass() failed." << std::endl;
      return false;
    }

    if (!init_shaders()) {
      std::cerr << "init_shaders() failed." << std::endl;
      return false;
    }

    if (!init_framebuffers()) {
      std::cerr << "init_framebuffers() failed." << std::endl;
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

    if (!init_sync_objects()) {
      std::cerr << "init_sync_objects() failed." << std::endl;
      return false;
    }

    if (!init_vertex_buffer()) {
      std::cerr << "init_vertex_buffer() failed." << std::endl;
      return false;
    }

    // if (!init_texture()) {
    //  std::cerr << "init_texture() failed." << std::endl;
    //  return false;
    //}

    if (!init_descriptor_pool()) {
      std::cerr << "init_descriptor_pool() failed." << std::endl;
      return false;
    }

    if (!init_descriptor_set()) {
      std::cerr << "init_descriptor_set() failed." << std::endl;
      return false;
    }

    if (!init_pipeline_cache()) {
      std::cerr << "init_pipeline_cache() failed." << std::endl;
      return false;
    }

    if (!init_pipeline()) {
      std::cerr << "init_pipeline() failed." << std::endl;
      return false;
    }

    return true;
  }

  bool render_frame(ImDrawData *imgui_draw_data) {
    VkResult res;

    VkClearValue clear_values[2];
    // Black with no alpha.
    clear_values[0].color.float32[0] = 0.0f;
    clear_values[0].color.float32[1] = 0.0f;
    clear_values[0].color.float32[2] = 0.0f;
    clear_values[0].color.float32[3] = 0.0f;
    //
    clear_values[1].depthStencil.depth = 1.0f;
    clear_values[1].depthStencil.stencil = 0;

    static constexpr VkBool32 wait_all = VK_TRUE;
    static constexpr uint64_t timeout = UINT64_MAX;
    res = vkWaitForFences(device_, 1, &in_flight_fences_[current_frame_],
                          wait_all, timeout);
    if (VK_SUCCESS != res) {
      std::cerr << "Failed to wait for in flight fence." << std::endl;
      return false;
    }

    // Get the index of the next available swapchain image.
    //
    res = vkAcquireNextImageKHR(device_, swap_chain_, timeout,
                                image_acquire_semaphores_[current_frame_],
                                VK_NULL_HANDLE, &current_buffer_);
    if (VK_SUCCESS != res) {
      std::cerr << "Failed to acquire next swap chain image. Current buffer = "
                << current_buffer_ << "." << std::endl;
      return false;
    }

    if (images_in_flight_[current_buffer_] != VK_NULL_HANDLE) {
      // Wait for the image to be available.
      res = vkWaitForFences(device_, 1, &images_in_flight_[current_buffer_],
                            wait_all, timeout);
    }
    images_in_flight_[current_buffer_] = in_flight_fences_[current_frame_];

    // Begin render pass.
    //

    if (!execute_begin_command_buffer()) {
      std::cerr << "execute_begin_command_buffer() failed." << std::endl;
      return false;
    }

    VkRenderPassBeginInfo render_pass_begin_info;
    render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_begin_info.pNext = nullptr;
    render_pass_begin_info.renderPass = render_pass_;
    render_pass_begin_info.framebuffer = framebuffers_[current_buffer_];
    render_pass_begin_info.renderArea.offset.x = 0;
    render_pass_begin_info.renderArea.offset.y = 0;
    VkExtent2D window_size = platform_.window_size();
    render_pass_begin_info.renderArea.extent.width = window_size.width;
    render_pass_begin_info.renderArea.extent.height = window_size.height;
    render_pass_begin_info.clearValueCount = 2;
    render_pass_begin_info.pClearValues = clear_values;

    vkCmdBeginRenderPass(command_buffers_[current_buffer_],
                         &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

    // Bind the graphic pipeline.
    //
    vkCmdBindPipeline(command_buffers_[current_buffer_],
                      VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_);
    static constexpr uint32_t first_set = 0;
    static constexpr uint32_t dynamic_offset_count = 0;
    static constexpr uint32_t *dynamic_offsets = nullptr;
    vkCmdBindDescriptorSets(
        command_buffers_[current_buffer_], VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipeline_layout_, first_set, NUM_DESCRIPTOR_SETS,
        descriptor_set_.data(), dynamic_offset_count, dynamic_offsets);

    // Bind the vertex buffer.
    //
    const VkDeviceSize offsets[1] = {0};
    static constexpr uint32_t first_binding = 0;
    static constexpr uint32_t binding_count = 1;

    vkCmdBindVertexBuffers(command_buffers_[current_buffer_], first_binding,
                           binding_count, &vertex_buffer_.buf, offsets);

    // Set the viewport and the scissors rectangle.
    //
    init_viewports();
    init_scissors();

    // Draw the vertices.
    //
    uint32_t vertex_count = 12 * 3;
    uint32_t instance_count = 1;
    uint32_t first_vertex = 0;
    uint32_t first_instance = 0;
    vkCmdDraw(command_buffers_[current_buffer_], vertex_count, instance_count,
              first_vertex, first_instance);

    // Record dear imgui primitives into command buffer.
    //
    ImGui_ImplVulkan_RenderDrawData(imgui_draw_data,
                                    command_buffers_[current_buffer_]);

    vkCmdEndRenderPass(
        command_buffers_[current_buffer_]);  // End of render pass.

    // Submit the command buffer.
    //
    res = vkEndCommandBuffer(command_buffers_[current_buffer_]);
    if (VK_SUCCESS != res) {
      std::cerr << "Failed to complete recording of command buffer."
                << std::endl;
      return false;
    }

    // Wait until the color attachment is filled.
    VkPipelineStageFlags pipeline_stage_flags =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = nullptr;
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &image_acquire_semaphores_[current_frame_];
    submit_info.pWaitDstStageMask = &pipeline_stage_flags;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffers_[current_buffer_];
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores =
        &render_finished_semaphores_[current_frame_];

    vkResetFences(device_, 1, &in_flight_fences_[current_frame_]);

    // Queue the command buffer for execution.
    static constexpr uint32_t submit_count = 1;
    res = vkQueueSubmit(graphics_queue_, submit_count, &submit_info,
                        in_flight_fences_[current_frame_]);
    if (VK_SUCCESS != res) {
      std::cerr << "Failed to submit command buffer to graphics queue."
                << std::endl;
      return false;
    }

    // Present the swapchain buffer to the display.
    //
    VkPresentInfoKHR present_info = {};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.pNext = nullptr;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &swap_chain_;
    present_info.pImageIndices = &current_buffer_;
    present_info.pWaitSemaphores = &render_finished_semaphores_[current_frame_];
    present_info.waitSemaphoreCount = 1;
    present_info.pResults = nullptr;

    // Present.
    //
    res = vkQueuePresentKHR(present_queue_, &present_info);
    if (VK_SUCCESS != res) {
      std::cerr << "Failed to present image." << std::endl;
      return false;
    }

    current_frame_ = (current_frame_ + 1) % MAX_FRAMES_IN_FLIGHT;

    return true;
  }

  bool draw() {
    if (!init_imgui()) {
      std::cerr << "Failed to init imgui." << std::endl;
      return false;
    }
    std::cout << "Imgui ready." << std::endl;

    bool show_demo_window = false;

    while (!platform_.should_close_window()) {
      platform_.poll_events();

      if (platform_.is_window_resized()) {
        VkExtent2D window_size = platform_.window_size();
        std::cout << "Resizing window to (" << window_size.width << "x"
                  << window_size.height << ")." << std::endl;

        ImGui_ImplVulkan_SetMinImageCount(swap_chain_image_count_);
      }

      // Start the Dear ImGui frame.
      ImGui_ImplVulkan_NewFrame();
      ImGui_ImplGlfw_NewFrame();
      ImGui::NewFrame();

      if (show_demo_window) {
        ImGui::ShowDemoWindow(&show_demo_window);
      }

      {
        // Create a window called "Hello, world!" and append into it.
        ImGui::Begin("Hello, world!");

        // Display some text (you can use a format strings too).
        ImGui::Text("GPU: %s", gpu_properties_.deviceName);

        // Edit bools storing our window open/close state.
        ImGui::Checkbox("Show Demo Window", &show_demo_window);

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                    1000.0f / ImGui::GetIO().Framerate,
                    ImGui::GetIO().Framerate);

        ImGui::End();
      }

      ImGui::Render();
      ImDrawData *draw_data = ImGui::GetDrawData();
      const bool is_minimized = (draw_data->DisplaySize.x <= 0.0f ||
                                 draw_data->DisplaySize.y <= 0.0f);
      if (is_minimized) {
        std::cout << "minimized." << std::endl;
      }

      render_frame(draw_data);
    }
    std::cout << "Closing window." << std::endl;

    vkDeviceWaitIdle(device_);
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    return true;
  }

  bool init_glfw(int width, int height, const std::string &title) {
    return platform_.init(width, height, title);
  }

  void fini_glfw() { platform_.fini(); }

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

    if (enable_validation_layer_) {
      // Validation layer
      instance_extension_names_.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

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

    void fini_instance() {
      vkDestroyInstance(instance_, allocation_callbacks_);
    }

    bool init_debug_callback() {
      VkDebugUtilsMessengerCreateInfoEXT debug_create_info{};

      debug_create_info.sType =
          VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;

      debug_create_info.messageSeverity =
          VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
          VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
          VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

      debug_create_info.messageType =
          VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
          VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
          VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

      debug_create_info.pfnUserCallback = debugCallback;

      debug_create_info.pUserData = nullptr;

      PFN_vkCreateDebugUtilsMessengerEXT f =
          (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
              instance_, "vkCreateDebugUtilsMessengerEXT");
      f(instance_, &debug_create_info, nullptr, &debug_messenger_);

      return true;
    }

    void fini_debug_callback() {}

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

    void fini_device() {
      vkDeviceWaitIdle(device_);
      vkDestroyDevice(device_, allocation_callbacks_);
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

    void fini_command_pool() {
      vkDestroyCommandPool(device_, command_pool_, allocation_callbacks_);
    }

    bool init_command_buffer() {
      // One command buffer per framebuffer.
      //
      VkCommandBufferAllocateInfo command_buffer_allocate_info = {};
      command_buffer_allocate_info.sType =
          VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
      command_buffer_allocate_info.pNext = nullptr;
      command_buffer_allocate_info.commandPool = command_pool_;
      command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
      command_buffer_allocate_info.commandBufferCount = swap_chain_image_count_;

      command_buffers_.resize(swap_chain_image_count_);
      VkResult res = vkAllocateCommandBuffers(
          device_, &command_buffer_allocate_info, command_buffers_.data());
      if (VK_SUCCESS != res) {
        std::cerr << "Failed to create command buffer." << std::endl;
        return false;
      }

      return true;
    }

    bool init_sync_objects() {
      image_acquire_semaphores_.resize(MAX_FRAMES_IN_FLIGHT);
      render_finished_semaphores_.resize(MAX_FRAMES_IN_FLIGHT);
      in_flight_fences_.resize(MAX_FRAMES_IN_FLIGHT);
      images_in_flight_.resize(swap_chain_image_count_);

      VkSemaphoreCreateInfo semaphore_create_info = {};
      semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
      semaphore_create_info.pNext = nullptr;
      semaphore_create_info.flags = 0;

      VkFenceCreateInfo fence_create_info;
      fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
      fence_create_info.pNext = nullptr;
      fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

      for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        if (vkCreateSemaphore(device_, &semaphore_create_info,
                              allocation_callbacks_,
                              &image_acquire_semaphores_[i]) != VK_SUCCESS) {
          std::cerr << "Failed to create image acquire semaphore " << i << "."
                    << std::endl;
          return false;
        }
        if (vkCreateSemaphore(device_, &semaphore_create_info,
                              allocation_callbacks_,
                              &render_finished_semaphores_[i]) != VK_SUCCESS) {
          std::cerr << "Failed to create render finish semaphore " << i << "."
                    << std::endl;
          return false;
        }
        if (vkCreateFence(device_, &fence_create_info, allocation_callbacks_,
                          &in_flight_fences_[i]) != VK_SUCCESS) {
          std::cerr << "Failed to create in flight fence " << i << "."
                    << std::endl;
          return false;
        }
      }

      current_frame_ = 0;

      return true;
    }

    void fini_sync_objects() {
      for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        vkDestroySemaphore(device_, image_acquire_semaphores_[i],
                           allocation_callbacks_);
        vkDestroySemaphore(device_, render_finished_semaphores_[i],
                           allocation_callbacks_);
        vkDestroyFence(device_, in_flight_fences_[i], allocation_callbacks_);
      }
    }

    static void imgui_check_vk_result(VkResult err) {
      if (VK_SUCCESS != err) {
        std::cerr << "Imgui vulkan call failed: " << err << "." << std::endl;
      }
    }

    bool init_imgui() {

      // Setup Dear ImGui context
      IMGUI_CHECKVERSION();
      ImGui::CreateContext();

      // Setup Platform/Renderer bindings
      if (!ImGui_ImplGlfw_InitForVulkan(platform_.window(), false)) {
        std::cerr << "Failed to setup imgui GLFW." << std::endl;
        return false;
      }

      ImGui_ImplVulkan_InitInfo vulkan_init_info = {};
      vulkan_init_info.Instance = instance_;
      vulkan_init_info.PhysicalDevice = gpus_[0];
      vulkan_init_info.Device = device_;
      vulkan_init_info.QueueFamily = graphics_queue_family_index_;
      vulkan_init_info.Queue = graphics_queue_;
      vulkan_init_info.PipelineCache = pipeline_cache_;
      vulkan_init_info.DescriptorPool = descriptor_pool_;
      vulkan_init_info.Allocator = allocation_callbacks_;
      vulkan_init_info.MinImageCount = swap_chain_image_count_;
      vulkan_init_info.ImageCount = buffers_.size();
      vulkan_init_info.CheckVkResultFn = imgui_check_vk_result;

      if (!ImGui_ImplVulkan_Init(&vulkan_init_info, render_pass_)) {
        std::cerr << "Failed to setup imgui vulkan." << std::endl;
        return false;
      }

      ImGuiIO &io = ImGui::GetIO();

      // Enable keyboard support.
      //
      io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

      // Enable gamepad support.
      //
      io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

      // Setup Dear ImGui style
      //
      ImGui::StyleColorsDark();

      io.IniFilename = nullptr;

      // Upload Fonts
      //
      // if (!io.Fonts->AddFontFromFileTTF("", font_size * scale_factor)) {
      //  std::cerr << "Failed to add imgui fonts." << std::endl;
      //  return false;
      //}
      VkCommandBufferBeginInfo command_buffer_begin_info = {};
      command_buffer_begin_info.sType =
          VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
      command_buffer_begin_info.flags =
          VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

      vkBeginCommandBuffer(command_buffers_[0], &command_buffer_begin_info);

      if (!ImGui_ImplVulkan_CreateFontsTexture(command_buffers_[0])) {
        std::cerr << "Failed to create imgui fonts." << std::endl;
        return false;
      }

      vkEndCommandBuffer(command_buffers_[0]);

      VkSubmitInfo submit_info = {};
      submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
      submit_info.commandBufferCount = 1;
      submit_info.pCommandBuffers = &command_buffers_[0];

      VkResult res = vkQueueSubmit(graphics_queue_, 1, &submit_info, nullptr);
      if (VK_SUCCESS != res) {
        std::cerr << "Failed to submit command buffer to graphics queue."
                  << std::endl;
        return false;
      }
      vkDeviceWaitIdle(device_);

      ImGui_ImplVulkan_DestroyFontUploadObjects();

      return true;
    }

    bool execute_begin_command_buffer() {
      VkCommandBufferBeginInfo command_buffer_begin_info = {};
      command_buffer_begin_info.sType =
          VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
      command_buffer_begin_info.pNext = nullptr;
      command_buffer_begin_info.flags = 0;
      command_buffer_begin_info.pInheritanceInfo = nullptr;

      VkResult res = vkBeginCommandBuffer(command_buffers_[current_buffer_],
                                          &command_buffer_begin_info);
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
      std::cout << present_mode_count << " present modes available."
                << std::endl;
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
        std::cout << "Present mode candidate: " << present_modes[i] << "."
                  << std::endl;
        if (present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
          swap_chain_present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
          break;
        }
      }
      free(present_modes);

      // Force mailbox.
      if (swap_chain_present_mode != VK_PRESENT_MODE_MAILBOX_KHR) {
        swap_chain_present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
      }

      std::cout << "Present mode: ";
      switch (swap_chain_present_mode) {
        case VK_PRESENT_MODE_FIFO_KHR:
          std::cout << "fifo";
          break;
        case VK_PRESENT_MODE_MAILBOX_KHR:
          std::cout << "mailbox";
          break;
        default:
          std::cout << swap_chain_present_mode;
      }
      std::cout << "." << std::endl;

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

      // Retrieve the created swap chain images.
      //
      res = vkGetSwapchainImagesKHR(device_, swap_chain_,
                                    &swap_chain_image_count_, nullptr);
      if (VK_SUCCESS != res) {
        std::cerr << "Failed to get swap chain image count." << std::endl;
        return false;
      }
      std::cout << "Swap chain size: " << swap_chain_image_count_ << "."
                << std::endl;

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

    void fini_swap_chain() {
      for (uint32_t i = 0; i < swap_chain_image_count_; ++i) {
        vkDestroyImageView(device_, buffers_[i].view, allocation_callbacks_);
      }
      vkDestroySwapchainKHR(device_, swap_chain_, allocation_callbacks_);
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

    void fini_depth_buffer() {
      vkDestroyImageView(device_, depth_buffer_.view, allocation_callbacks_);
      vkDestroyImage(device_, depth_buffer_.image, allocation_callbacks_);
      vkFreeMemory(device_, depth_buffer_.mem, allocation_callbacks_);
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

    void fini_uniform_buffer() {
      vkDestroyBuffer(device_, uniform_data_.buf, allocation_callbacks_);
      vkFreeMemory(device_, uniform_data_.mem, allocation_callbacks_);
    }

    bool init_descriptor_layout() {
      VkDescriptorSetLayoutBinding layout_bindings[2];
      layout_bindings[0].binding = 0;
      layout_bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      layout_bindings[0].descriptorCount = 1;
      layout_bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
      layout_bindings[0].pImmutableSamplers = nullptr;

      if (use_texture_) {
        layout_bindings[1].binding = 1;
        layout_bindings[1].descriptorType =
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        layout_bindings[1].descriptorCount = 1;
        layout_bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        layout_bindings[1].pImmutableSamplers = nullptr;
      }

      VkDescriptorSetLayoutCreateInfo descriptor_layout = {};
      descriptor_layout.sType =
          VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
      descriptor_layout.pNext = nullptr;
      descriptor_layout.flags = 0;
      descriptor_layout.bindingCount = use_texture_ ? 2 : 1;
      descriptor_layout.pBindings = layout_bindings;

      descriptor_layout_.resize(NUM_DESCRIPTOR_SETS);

      VkResult res = vkCreateDescriptorSetLayout(device_, &descriptor_layout,
                                                 allocation_callbacks_,
                                                 descriptor_layout_.data());
      if (VK_SUCCESS != res) {
        std::cerr << "Failed to create descriptor set layout." << std::endl;
        return false;
      }

      VkPipelineLayoutCreateInfo pipeline_layout_create_info = {};
      pipeline_layout_create_info.sType =
          VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
      pipeline_layout_create_info.pNext = nullptr;
      pipeline_layout_create_info.pushConstantRangeCount = 0;
      pipeline_layout_create_info.pPushConstantRanges = nullptr;
      pipeline_layout_create_info.setLayoutCount = NUM_DESCRIPTOR_SETS;
      pipeline_layout_create_info.pSetLayouts = descriptor_layout_.data();

      res = vkCreatePipelineLayout(device_, &pipeline_layout_create_info,
                                   allocation_callbacks_, &pipeline_layout_);
      if (VK_SUCCESS != res) {
        std::cerr << "Failed to create pipeline layout." << std::endl;
        return false;
      }

      return true;
    }

    void fini_descriptor_layout() {
      for (int i = 0; i < NUM_DESCRIPTOR_SETS; i++) {
        vkDestroyDescriptorSetLayout(device_, descriptor_layout_[i],
                                     allocation_callbacks_);
      }
      vkDestroyPipelineLayout(device_, pipeline_layout_, allocation_callbacks_);
    }

    bool init_render_pass() {
      VkAttachmentDescription attachments[2];
      attachments[0].format = format_;
      attachments[0].samples = NUM_SAMPLES;
      attachments[0].loadOp =
          VK_ATTACHMENT_LOAD_OP_CLEAR;  // TODO: Switch to
                                        // VK_ATTACHMENT_LOAD_OP_LOAD for
                                        // accumulation on ray tracing?
      attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
      attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
      attachments[0].flags = 0;

      if (include_depth_) {
        attachments[1].format = depth_buffer_.format;
        attachments[1].samples = NUM_SAMPLES;
        attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        // attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[1].finalLayout =
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        attachments[1].flags = 0;
      }

      VkAttachmentReference color_reference = {};
      color_reference.attachment = 0;
      color_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

      VkAttachmentReference depth_reference = {};
      depth_reference.attachment = 1;
      depth_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

      VkSubpassDescription subpass_description = {};
      subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
      subpass_description.flags = 0;
      subpass_description.inputAttachmentCount = 0;
      subpass_description.pInputAttachments = nullptr;
      subpass_description.colorAttachmentCount = 1;
      subpass_description.pColorAttachments = &color_reference;
      subpass_description.pResolveAttachments = nullptr;
      subpass_description.pDepthStencilAttachment =
          include_depth_ ? &depth_reference : nullptr;
      subpass_description.preserveAttachmentCount = 0;
      subpass_description.pPreserveAttachments = nullptr;

      // Add a dependency to signal that the image is ready for the swap chain.
      VkSubpassDependency subpass_dependency = {};
      subpass_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;  // Pre-subpass.
      subpass_dependency.dstSubpass = 0;  // The only subpass defined.
      subpass_dependency.srcStageMask =
          VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
      subpass_dependency.dstStageMask =
          VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
      subpass_dependency.srcAccessMask = 0;
      subpass_dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
      subpass_dependency.dependencyFlags = 0;

      VkRenderPassCreateInfo render_pass_create_info = {};
      render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
      render_pass_create_info.pNext = nullptr;
      render_pass_create_info.attachmentCount = include_depth_ ? 2 : 1;
      render_pass_create_info.pAttachments = attachments;
      render_pass_create_info.subpassCount = 1;
      render_pass_create_info.pSubpasses = &subpass_description;
      render_pass_create_info.dependencyCount = 1;
      render_pass_create_info.pDependencies = &subpass_dependency;

      VkResult res = vkCreateRenderPass(device_, &render_pass_create_info,
                                        allocation_callbacks_, &render_pass_);
      if (VK_SUCCESS != res) {
        std::cerr << "Failed to create render pass." << std::endl;
        return false;
      }

      return true;
    }

    void fini_render_pass() {
      vkDestroyRenderPass(device_, render_pass_, allocation_callbacks_);
    }

    bool init_shaders() {
      VkShaderModuleCreateInfo draw_cube_vert_create_info = {};
      draw_cube_vert_create_info.sType =
          VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
      draw_cube_vert_create_info.pNext = nullptr;
      draw_cube_vert_create_info.flags = 0;
      draw_cube_vert_create_info.codeSize = sizeof(draw_cube_vert);
      draw_cube_vert_create_info.pCode = draw_cube_vert;

      VkShaderModuleCreateInfo draw_cube_frag_create_info = {};
      draw_cube_frag_create_info.sType =
          VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
      draw_cube_frag_create_info.pNext = nullptr;
      draw_cube_frag_create_info.flags = 0;
      draw_cube_frag_create_info.codeSize = sizeof(draw_cube_frag);
      draw_cube_frag_create_info.pCode = draw_cube_frag;

      shader_stages_create_info_[0].sType =
          VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
      shader_stages_create_info_[0].pNext = nullptr;
      shader_stages_create_info_[0].pSpecializationInfo = nullptr;
      shader_stages_create_info_[0].flags = 0;
      shader_stages_create_info_[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
      shader_stages_create_info_[0].pName = "main";

      VkResult res = vkCreateShaderModule(
          device_, &draw_cube_vert_create_info, allocation_callbacks_,
          &shader_stages_create_info_[0].module);
      if (VK_SUCCESS != res) {
        std::cerr << "Failed to create cube vertex shader module." << std::endl;
        return false;
      }

      shader_stages_create_info_[1].sType =
          VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
      shader_stages_create_info_[1].pNext = nullptr;
      shader_stages_create_info_[1].pSpecializationInfo = nullptr;
      shader_stages_create_info_[1].flags = 0;
      shader_stages_create_info_[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
      shader_stages_create_info_[1].pName = "main";

      res = vkCreateShaderModule(device_, &draw_cube_frag_create_info,
                                 allocation_callbacks_,
                                 &shader_stages_create_info_[1].module);
      if (VK_SUCCESS != res) {
        std::cerr << "Failed to create cube fragment shader module."
                  << std::endl;
        return false;
      }

      return true;
    }

    void fini_shaders() {
      for (uint32_t i = 0; i < 2; ++i) {
        vkDestroyShaderModule(device_, shader_stages_create_info_[i].module,
                              allocation_callbacks_);
      }
    }

    bool init_framebuffers() {
      VkImageView attachments[2];           // Color and depth.
      attachments[1] = depth_buffer_.view;  // Depth buffer is shared.

      VkFramebufferCreateInfo framebuffer_create_info = {};
      framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
      framebuffer_create_info.pNext = nullptr;
      framebuffer_create_info.renderPass = render_pass_;
      framebuffer_create_info.attachmentCount = include_depth_ ? 2 : 1;
      framebuffer_create_info.pAttachments = attachments;
      VkExtent2D window = platform_.window_size();
      framebuffer_create_info.width = window.width;
      framebuffer_create_info.height = window.height;
      framebuffer_create_info.layers = 1;

      framebuffers_ = (VkFramebuffer *)malloc(swap_chain_image_count_ *
                                              sizeof(VkFramebuffer));

      for (uint32_t i = 0; i < swap_chain_image_count_; ++i) {
        attachments[0] = buffers_[i].view;
        VkResult res =
            vkCreateFramebuffer(device_, &framebuffer_create_info,
                                allocation_callbacks_, &framebuffers_[i]);
        if (VK_SUCCESS != res) {
          std::cerr << "Failed to create framebuffer " << i << "." << std::endl;
          return false;
        }
      }

      return true;
    }

    void fini_framebuffers() {
      for (uint32_t i = 0; i < swap_chain_image_count_; ++i) {
        vkDestroyFramebuffer(device_, framebuffers_[i], allocation_callbacks_);
      }
      free(framebuffers_);
      framebuffers_ = nullptr;
    }

    bool init_vertex_buffer() {
      const void *vertex_data = g_vb_solid_face_colors_Data;
      uint32_t vertex_data_size = sizeof(g_vb_solid_face_colors_Data);
      uint32_t data_stride = sizeof(g_vb_solid_face_colors_Data[0]);

      VkBufferCreateInfo buffer_create_info = {};
      buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
      buffer_create_info.pNext = nullptr;
      buffer_create_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
      buffer_create_info.size = vertex_data_size;
      buffer_create_info.queueFamilyIndexCount = 0;
      buffer_create_info.pQueueFamilyIndices = nullptr;
      buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
      buffer_create_info.flags = 0;

      VkResult res = vkCreateBuffer(device_, &buffer_create_info,
                                    allocation_callbacks_, &vertex_buffer_.buf);
      if (VK_SUCCESS != res) {
        std::cerr << "Failed to create vertex buffer." << std::endl;
        return false;
      }

      VkMemoryRequirements memory_requirements;
      vkGetBufferMemoryRequirements(device_, vertex_buffer_.buf,
                                    &memory_requirements);

      VkMemoryAllocateInfo memory_allocate_info = {};
      memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
      memory_allocate_info.pNext = nullptr;
      memory_allocate_info.memoryTypeIndex = 0;
      memory_allocate_info.allocationSize = memory_requirements.size;

      if (!memory_type_from_properties(memory_requirements.memoryTypeBits,
                                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                           VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                       &memory_allocate_info.memoryTypeIndex)) {
        std::cerr << "Failed to get memory type properties of vertex buffer."
                  << std::endl;
        return false;
      }

      res = vkAllocateMemory(device_, &memory_allocate_info,
                             allocation_callbacks_, &vertex_buffer_.mem);
      if (VK_SUCCESS != res) {
        std::cerr << "Failed to allocate memory for vertex buffer."
                  << std::endl;
        return false;
      }

      vertex_buffer_.buffer_info.range = memory_requirements.size;
      vertex_buffer_.buffer_info.offset = 0;

      uint8_t *pointer_vertex_data;
      VkDeviceSize offset = 0;
      VkMemoryMapFlags flags = 0;
      res = vkMapMemory(device_, vertex_buffer_.mem, offset,
                        memory_requirements.size, flags,
                        (void **)&pointer_vertex_data);
      if (VK_SUCCESS != res) {
        std::cerr << "Failed to map vertex buffer to CPU memory." << std::endl;
        return false;
      }

      memcpy(pointer_vertex_data, vertex_data, vertex_data_size);

      vkUnmapMemory(device_, vertex_buffer_.mem);

      VkDeviceSize memory_offset = 0;
      res = vkBindBufferMemory(device_, vertex_buffer_.buf, vertex_buffer_.mem,
                               memory_offset);
      if (VK_SUCCESS != res) {
        std::cerr << "Failed to bind vertex buffer memory." << std::endl;
        return false;
      }

      vertex_input_binding_.binding = 0;
      vertex_input_binding_.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
      vertex_input_binding_.stride = data_stride;

      vertex_input_attributes_[0].binding = 0;
      vertex_input_attributes_[0].location = 0;
      vertex_input_attributes_[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
      vertex_input_attributes_[0].offset = 0;

      vertex_input_attributes_[1].binding = 0;
      vertex_input_attributes_[1].location = 1;
      vertex_input_attributes_[1].format = use_texture_
                                               ? VK_FORMAT_R32G32_SFLOAT
                                               : VK_FORMAT_R32G32B32A32_SFLOAT;
      vertex_input_attributes_[1].offset = 16;  // After the vexter position.

      return true;
    }

    void fini_vertex_buffer() {
      vkDestroyBuffer(device_, vertex_buffer_.buf, allocation_callbacks_);
      vkFreeMemory(device_, vertex_buffer_.mem, allocation_callbacks_);
    }

    // bool init_texture() {
    //  const std::string texture_name("assets/textures/lunar.ppm");
    //  texture_object_t texture_object;
    //  VkImageUsageFlags image_usage_flags = 0;
    //  VkFormatFeatureFlags format_texture_flags = 0;

    //  if (!load_image(texture_object, texture_name, image_usage_flags,
    //                  format_texture_flags)) {
    //    std::cerr << "Failed to load " << texture_name << " image."
    //              << std::endl;
    //    return false;
    //  }

    //  if (create_sampler(texture_object.sampler)) {
    //    std::cerr << "Failed to create sampler." << std::endl;
    //    return false;
    //  }

    //  textures_.push_back(texture_object);

    //  texture_data_.image_info.imageView = textures_.back().view;
    //  texture_data_.image_info.sampler = textures_.back().sampler;
    //  texture_data_.image_info.imageLayout =
    //      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    //  return true;
    //}

    bool init_descriptor_pool() {
      // VkDescriptorPoolSize descriptor_pool_size[2];

      // descriptor_pool_size[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      // descriptor_pool_size[0].descriptorCount = 1;

      // if (use_texture_) {
      //  descriptor_pool_size[1].type =
      //      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      //  descriptor_pool_size[1].descriptorCount = 1;
      //}
      static constexpr uint32_t pool_descriptor_count = 1000;
      VkDescriptorPoolSize descriptor_pool_size[] = {
          {VK_DESCRIPTOR_TYPE_SAMPLER, pool_descriptor_count},
          {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, pool_descriptor_count},
          {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, pool_descriptor_count},
          {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, pool_descriptor_count},
          {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, pool_descriptor_count},
          {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, pool_descriptor_count},
          {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, pool_descriptor_count},
          {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, pool_descriptor_count},
          {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, pool_descriptor_count},
          {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, pool_descriptor_count},
          {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, pool_descriptor_count}};

      VkDescriptorPoolCreateInfo descriptor_pool_create_info = {};
      descriptor_pool_create_info.sType =
          VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
      descriptor_pool_create_info.pNext = nullptr;
      descriptor_pool_create_info.maxSets = pool_descriptor_count * 11;
      // descriptor_pool_create_info.poolSizeCount = use_texture_ ? 2 : 1;
      descriptor_pool_create_info.poolSizeCount = 11;
      descriptor_pool_create_info.pPoolSizes = descriptor_pool_size;

      VkResult res =
          vkCreateDescriptorPool(device_, &descriptor_pool_create_info,
                                 allocation_callbacks_, &descriptor_pool_);
      if (VK_SUCCESS != res) {
        std::cerr << "Failed to create descriptor pool." << std::endl;
        return false;
      }

      return true;
    }

    void fini_descriptor_pool() {
      vkDestroyDescriptorPool(device_, descriptor_pool_, allocation_callbacks_);
    }

    bool init_descriptor_set() {
      VkDescriptorSetAllocateInfo descriptor_set_allocate_info = {};
      descriptor_set_allocate_info.sType =
          VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
      descriptor_set_allocate_info.pNext = nullptr;
      descriptor_set_allocate_info.descriptorPool = descriptor_pool_;
      descriptor_set_allocate_info.descriptorSetCount = NUM_DESCRIPTOR_SETS;
      descriptor_set_allocate_info.pSetLayouts = descriptor_layout_.data();

      descriptor_set_.resize(NUM_DESCRIPTOR_SETS);
      VkResult res = vkAllocateDescriptorSets(
          device_, &descriptor_set_allocate_info, descriptor_set_.data());
      if (VK_SUCCESS != res) {
        std::cerr << "Failed to allocate descriptor set." << std::endl;
        return false;
      }

      VkWriteDescriptorSet write_descriptor_set[2];

      write_descriptor_set[0] = {};
      write_descriptor_set[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      write_descriptor_set[0].pNext = nullptr;
      write_descriptor_set[0].dstSet = descriptor_set_[0];
      write_descriptor_set[0].descriptorCount = 1;
      write_descriptor_set[0].descriptorType =
          VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      write_descriptor_set[0].pBufferInfo = &uniform_data_.buffer_info;
      write_descriptor_set[0].dstArrayElement = 0;
      write_descriptor_set[0].dstBinding = 0;

      if (use_texture_) {
        write_descriptor_set[1] = {};
        write_descriptor_set[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write_descriptor_set[1].pNext = nullptr;
        write_descriptor_set[1].dstSet = descriptor_set_[0];
        write_descriptor_set[1].descriptorCount = 1;
        write_descriptor_set[1].descriptorType =
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write_descriptor_set[1].pImageInfo = &texture_data_.image_info;
        write_descriptor_set[1].dstArrayElement = 0;
        write_descriptor_set[1].dstBinding = 1;
      }

      uint32_t descriptor_copy_count = 0;
      const VkCopyDescriptorSet *descriptor_copies = nullptr;
      vkUpdateDescriptorSets(device_, use_texture_ ? 2 : 1,
                             write_descriptor_set, descriptor_copy_count,
                             descriptor_copies);

      return true;
    }

    bool init_pipeline_cache() {
      VkPipelineCacheCreateInfo pipeline_cache_create_info = {};
      pipeline_cache_create_info.sType =
          VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
      pipeline_cache_create_info.pNext = nullptr;
      pipeline_cache_create_info.initialDataSize = 0;
      pipeline_cache_create_info.pInitialData = nullptr;
      pipeline_cache_create_info.flags = 0;

      VkResult res =
          vkCreatePipelineCache(device_, &pipeline_cache_create_info,
                                allocation_callbacks_, &pipeline_cache_);
      if (VK_SUCCESS != res) {
        std::cerr << "Failed to create pipeline cache." << std::endl;
        return false;
      }

      return true;
    }

    void fini_pipeline_cache() {
      vkDestroyPipelineCache(device_, pipeline_cache_, allocation_callbacks_);
    }

    bool init_pipeline() {
      VkDynamicState dynamic_states[2];  // Viewport + scissor.
      memset(dynamic_states, 0, sizeof(dynamic_states));

      // Dynamic State
      //
      VkPipelineDynamicStateCreateInfo pipeline_dynamic_state_create_info = {};
      pipeline_dynamic_state_create_info.sType =
          VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
      pipeline_dynamic_state_create_info.pNext = nullptr;
      pipeline_dynamic_state_create_info.pDynamicStates = dynamic_states;
      pipeline_dynamic_state_create_info.dynamicStateCount = 0;

      // Pipeline Vertex Input State
      //
      VkPipelineVertexInputStateCreateInfo vi;
      memset(&vi, 0, sizeof(vi));
      vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
      if (include_depth_) {
        vi.pNext = nullptr;
        vi.flags = 0;
        vi.vertexBindingDescriptionCount = 1;
        vi.pVertexBindingDescriptions = &vertex_input_binding_;
        vi.vertexAttributeDescriptionCount = 2;
        vi.pVertexAttributeDescriptions = vertex_input_attributes_;
      }

      // Pipeline Vertex Input Assembly State
      //
      VkPipelineInputAssemblyStateCreateInfo ia;
      ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
      ia.pNext = nullptr;
      ia.flags = 0;
      ia.primitiveRestartEnable = VK_FALSE;
      ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

      // Pipeline Rasterization State
      //
      VkPipelineRasterizationStateCreateInfo rs;
      rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
      rs.pNext = nullptr;
      rs.flags = 0;
      rs.polygonMode = VK_POLYGON_MODE_FILL;
      rs.cullMode = VK_CULL_MODE_BACK_BIT;
      rs.frontFace = VK_FRONT_FACE_CLOCKWISE;
      rs.depthClampEnable = VK_FALSE;
      rs.rasterizerDiscardEnable = VK_FALSE;
      rs.depthBiasEnable = VK_FALSE;
      rs.depthBiasConstantFactor = 0;
      rs.depthBiasClamp = 0;
      rs.depthBiasSlopeFactor = 0;
      rs.lineWidth = 1.0f;

      // Pipeline Color Blend State
      //
      VkPipelineColorBlendAttachmentState cb_attachment_state[1];
      cb_attachment_state[0].colorWriteMask =
          VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
      cb_attachment_state[0].blendEnable = VK_FALSE;
      cb_attachment_state[0].alphaBlendOp = VK_BLEND_OP_ADD;
      cb_attachment_state[0].colorBlendOp = VK_BLEND_OP_ADD;
      cb_attachment_state[0].srcColorBlendFactor = VK_BLEND_FACTOR_ZERO;
      cb_attachment_state[0].dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
      cb_attachment_state[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
      cb_attachment_state[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;

      VkPipelineColorBlendStateCreateInfo cb;
      cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
      cb.flags = 0;
      cb.pNext = nullptr;
      cb.attachmentCount = 1;
      cb.pAttachments = cb_attachment_state;
      cb.logicOpEnable = VK_FALSE;
      cb.logicOp = VK_LOGIC_OP_NO_OP;
      cb.blendConstants[0] = 1.0f;
      cb.blendConstants[1] = 1.0f;
      cb.blendConstants[2] = 1.0f;
      cb.blendConstants[3] = 1.0f;

      // Pipeline Viewport State
      //
      VkPipelineViewportStateCreateInfo vp;
      vp.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
      vp.pNext = nullptr;
      vp.flags = 0;
      vp.viewportCount = NUM_VIEWPORTS_AND_SCISSORS;
      dynamic_states[pipeline_dynamic_state_create_info.dynamicStateCount++] =
          VK_DYNAMIC_STATE_VIEWPORT;
      vp.scissorCount = NUM_VIEWPORTS_AND_SCISSORS;
      dynamic_states[pipeline_dynamic_state_create_info.dynamicStateCount++] =
          VK_DYNAMIC_STATE_SCISSOR;
      vp.pScissors = nullptr;
      vp.pViewports = nullptr;

      // Pipeline Depth Stencil State
      //
      VkPipelineDepthStencilStateCreateInfo ds;
      ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
      ds.pNext = nullptr;
      ds.flags = 0;
      ds.depthTestEnable = include_depth_;
      ds.depthWriteEnable = include_depth_;
      ds.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
      ds.depthBoundsTestEnable = VK_FALSE;
      ds.stencilTestEnable = VK_FALSE;
      ds.back.failOp = VK_STENCIL_OP_KEEP;
      ds.back.passOp = VK_STENCIL_OP_KEEP;
      ds.back.compareOp = VK_COMPARE_OP_ALWAYS;
      ds.back.compareMask = 0;
      ds.back.reference = 0;
      ds.back.depthFailOp = VK_STENCIL_OP_KEEP;
      ds.back.writeMask = 0;
      ds.minDepthBounds = 0;
      ds.maxDepthBounds = 0;
      ds.stencilTestEnable = VK_FALSE;
      ds.front = ds.back;

      // Pipeline Multisample State
      //
      VkPipelineMultisampleStateCreateInfo ms;
      ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
      ms.pNext = nullptr;
      ms.flags = 0;
      ms.pSampleMask = nullptr;
      ms.rasterizationSamples = NUM_SAMPLES;
      ms.sampleShadingEnable = VK_FALSE;
      ms.alphaToCoverageEnable = VK_FALSE;
      ms.alphaToOneEnable = VK_FALSE;
      ms.minSampleShading = 0.0;

      // Pipeline
      //
      VkGraphicsPipelineCreateInfo pipeline;
      pipeline.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
      pipeline.pNext = nullptr;
      pipeline.layout = pipeline_layout_;
      pipeline.basePipelineHandle = VK_NULL_HANDLE;
      pipeline.basePipelineIndex = 0;
      pipeline.flags = 0;
      pipeline.pVertexInputState = &vi;
      pipeline.pInputAssemblyState = &ia;
      pipeline.pRasterizationState = &rs;
      pipeline.pColorBlendState = &cb;
      pipeline.pTessellationState = nullptr;
      pipeline.pMultisampleState = &ms;
      pipeline.pDynamicState = &pipeline_dynamic_state_create_info;
      pipeline.pViewportState = &vp;
      pipeline.pDepthStencilState = &ds;
      pipeline.pStages = shader_stages_create_info_;
      pipeline.stageCount = 2;
      pipeline.renderPass = render_pass_;
      pipeline.subpass = 0;

      static constexpr uint32_t create_info_count = 1;
      VkResult res = vkCreateGraphicsPipelines(
          device_, pipeline_cache_, create_info_count, &pipeline,
          allocation_callbacks_, &pipeline_);
      if (VK_SUCCESS != res) {
        std::cerr << "Failed to create pipeline." << std::endl;
        return false;
      }

      return true;
    }

    void fini_pipeline() {
      vkDestroyPipeline(device_, pipeline_, allocation_callbacks_);
    }

    void fini() {
      fini_pipeline();
      fini_pipeline_cache();
      fini_descriptor_pool();
      fini_vertex_buffer();
      fini_framebuffers();
      fini_shaders();
      fini_render_pass();
      fini_descriptor_layout();
      fini_uniform_buffer();
      fini_depth_buffer();
      fini_swap_chain();
      fini_sync_objects();
      fini_command_pool();
      fini_device();
      vkDestroySurfaceKHR(instance_, surface_, allocation_callbacks_);
      fini_glfw();
      fini_debug_callback();
      fini_instance();
    }

   private:
    platform platform_;
    VkInstance instance_;
    VkDebugUtilsMessengerEXT debug_messenger_;
    std::vector<VkPhysicalDevice> gpus_;
    std::vector<VkQueueFamilyProperties> queue_props_;
    VkPhysicalDeviceMemoryProperties memory_properties_;
    VkDevice device_;
    VkQueue graphics_queue_;
    VkQueue present_queue_;
    uint32_t graphics_queue_family_index_;
    uint32_t present_queue_family_index_;
    VkPhysicalDeviceProperties gpu_properties_;

    VkFramebuffer *framebuffers_;
    VkFormat format_;

    uint32_t swap_chain_image_count_;
    VkSwapchainKHR swap_chain_;
    std::vector<swap_chain_buffer_t> buffers_;
    uint32_t current_buffer_;

    VkCommandPool command_pool_;
    std::vector<VkCommandBuffer> command_buffers_;

    std::vector<VkSemaphore> image_acquire_semaphores_;
    std::vector<VkSemaphore> render_finished_semaphores_;
    std::vector<VkFence> in_flight_fences_;
    std::vector<VkFence> images_in_flight_;
    uint32_t current_frame_;

    depth_buffer_t depth_buffer_;
    VkPipelineLayout pipeline_layout_;
    std::vector<VkDescriptorSetLayout> descriptor_layout_;
    VkPipelineCache pipeline_cache_;
    VkRenderPass render_pass_;
    VkPipeline pipeline_;

    VkPipelineShaderStageCreateInfo shader_stages_create_info_[2];

    VkDescriptorPool descriptor_pool_;
    std::vector<VkDescriptorSet> descriptor_set_;

    uniform_data_t uniform_data_;

    texture_data_t texture_data_;

    vertex_buffer_t vertex_buffer_;
    VkVertexInputBindingDescription vertex_input_binding_;
    VkVertexInputAttributeDescription vertex_input_attributes_[2];

    std::vector<texture_object_t> textures_;

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

    uint32_t queue_family_count_;

    VkViewport viewport_;
    VkRect2D scissor_;

    const VkAllocationCallbacks *allocation_callbacks_;

    bool enable_validation_layer_;

    std::string application_name_;
    uint32_t application_version_;
    const std::string engine_name_ = "rtx_engine";
    static constexpr uint32_t engine_version_ = 1;

    static const VkSampleCountFlagBits NUM_SAMPLES = VK_SAMPLE_COUNT_1_BIT;

    // Number of descriptor sets needs to be the same at alloc, pipeline layout
    // creation, and descriptor set layout creation.
    static constexpr int NUM_DESCRIPTOR_SETS = 1;

    // Number of viewports and number of scissors have to be the same at
    // pipeline creation and in any call to set them dynamically.
    static constexpr int NUM_VIEWPORTS_AND_SCISSORS = 1;

    // Number of frames that can be drawed concurrently.
    static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;

    static constexpr bool include_depth_ = true;  // TODO: False on ray tracing?
    static constexpr bool use_texture_ = false;   // TODO: False on ray tracing?

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

    // bool load_image(texture_object_t &texture_object,
    //                const std::string &texture_name,
    //                VkImageUsageFlags image_usage_flags,
    //                VkFormatFeatureFlags format_texture_flags) {
    //  // Get image dimensions.
    //  uint64_t row_pith = 0;
    //  unsigned char *data_pointer = nullptr;
    //  if (!load_ppm(texture_object, texture_name, row_pitch, data_pointer)) {
    //    std::cerr << "Failed to load ppm " << texture_name << "." <<
    //    std::endl; return false;
    //  }

    //  VkFormatProperties format_properties;
    //  vkGetPhysicalDeviceFormatProperties(gpus_[0], VK_FORMAT_R8G8B8A8_UNORM,
    //                                      &format_properties);

    //  // Check if linear tiled image can be used for the texture.
    //  // Else, use a staging buffer for the texture data.
    //  VkFormatFeatureFlags all_features =
    //      (VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | format_texture_flags);
    //  texture_object.needs_staging = ((format_properties.linearTilingFeatures
    //  &
    //                                   all_features) != all_features);

    //  if (texture_object.needs_staging) {
    //    if ((format_properties.optimalTilingFeatures & all_features) !=
    //        all_features) {
    //      std::cerr << "Optimal tiling features mismatch." << std::endl;
    //      return false;
    //    }
    //    if (!create_buffer(texture_object)) {
    //      std::cerr << "Failed to create buffer for image." << std::endl;
    //      return false;
    //    }
    //    format_texture_flags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    //  } else {
    //    texture_object.buffer = VK_NULL_HANDLE;
    //    texture_object.buffer_memory = VK_NULL_HANDLE;
    //  }

    //  VkImageCreateInfo image_create_info = {};
    //  image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    //  image_create_info.pNext = nullptr;
    //  image_create_info.imageType = VK_IMAGE_TYPE_2D;
    //  image_create_info.format = VK_FORMAT_R8G8B8A8_UNORM;
    //  image_create_info.extent.width = texture_object.texture_width;
    //  image_create_info.extent.height = texture_object.texture_height;
    //  image_create_info.extent.depth = 1;
    //  image_create_info.mipLevels = 1;
    //  image_create_info.arrayLayers = 1;
    //  image_create_info.samples = NUM_SAMPLES;
    //  image_create_info.tiling = texture_object.needs_staging
    //                                 ? VK_IMAGE_TILING_OPTIMAL
    //                                 : VK_IMAGE_TILING_LINEAR;
    //  image_create_info.initialLayout = texture_object.needs_staging
    //                                        ? VK_IMAGE_LAYOUT_UNDEFINED
    //                                        : VK_IMAGE_LAYOUT_PREINITIALIZED;
    //  image_create_info.usage = VK_IMAGE_USAGE_SAMPLED_BIT |
    //  image_usage_flags; image_create_info.queueFamilyIndexCount = 0;
    //  image_create_info.pQueueFamilyIndices = nullptr;
    //  image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    //  image_create_info.flags = 0;

    //  VkMemoryAllocateInfo memory_allocate_info = {};
    //  memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    //  memory_allocate_info.pNext = nullptr;
    //  memory_allocate_info.allocationSize = 0;
    //  memory_allocate_info.memoryTypeIndex = 0;

    //  VkMemoryRequirements memory_requirements;
    //  res = vkCreateImage(device_, &image_create_info, allocation_callbacks_,
    //                      &texture_object.image);
    //  if (VK_SUCCESS != res) {
    //    std::cerr << "Failed to create image." << std::endl;
    //    return false;
    //  }

    //  vkGetImageMemoryRequirements(device_, texture_object.image,
    //                               &memory_requirements);
    //  memory_allocate_info.allocationSize = memory_requirements.size;

    //  VkFlags requirements = texture_object.needs_staging
    //                             ? 0
    //                             : (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
    //                                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    //  if (!memory_type_from_properties(memory_requirements.memoryTypeBits,
    //                                   requirements,
    //                                   &memory_allocate_info.memoryTypeIndex))
    //                                   {
    //    std::cerr << "Failed to get memory properties for texture image."
    //              << std::endl;
    //    return false;
    //  }

    //  res =
    //      vkAllocateMemory(device_, &memory_allocate_info,
    //                       allocation_callbacks_,
    //                       &texture_object.image_memory);
    //  if (VK_SUCCESS != res) {
    //    std::cerr << "Failed to allocate memory for image." << std::endl;
    //    return false;
    //  }

    //  VkDeviceSize memory_offset = 0;
    //  res = vkBindImageMemory(device_, texture_object.image,
    //                          texture_object.image_memory, memory_offset);
    //  if (VK_SUCCESS != res) {
    //    std::cerr << "Failed to bind memory for image." << std::endl;
    //    return false;
    //  }

    //  res = vkEndCommandBuffer(

    //  return true;
    //}

    bool load_ppm(texture_object_t &texture_object,
                  const std::string &texture_name, uint64_t row_pitch,
                  unsigned char *data_pointer) {
      // PPM format expected from http://netpbm.sourceforge.net/doc/ppm.html
      //  1. magic number
      //  2. whitespace
      //  3. width
      //  4. whitespace
      //  5. height
      //  6. whitespace
      //  7. max color value
      //  8. whitespace
      //  7. data

      // Comments are not supported, but are detected and we kick out
      // Only 8 bits per channel is supported
      // If data_pointer is nullptr, only width and height are returned

      // Read in values from the PPM file as characters to check for comments
      char magicStr[3] = {};
      char heightStr[6] = {};
      char widthStr[6] = {};
      char formatStr[6] = {};

      FILE *fPtr = fopen(texture_name.c_str(), "rb");
      if (!fPtr) {
        std::cerr << "Bad filename in load_ppm: " << texture_name << "."
                  << std::endl;
        return false;
      }

      // Read the four values from file, accounting with any and all whitepace
      int count = fscanf(fPtr, "%s %s %s %s ", magicStr, widthStr, heightStr,
                         formatStr);
      if (count != 4) {
        std::cerr << "Failed to read PPM header." << std::endl;
        return false;
      }

      // Kick out if comments present
      if (magicStr[0] == '#' || widthStr[0] == '#' || heightStr[0] == '#' ||
          formatStr[0] == '#') {
        std::cerr << "Unhandled comment in PPM file." << std::endl;
        return false;
      }

      // Only one magic value is valid
      if (strncmp(magicStr, "P6", sizeof(magicStr))) {
        std::cerr << "Unhandled PPM magic number: " << magicStr << "."
                  << std::endl;
        return false;
      }

      texture_object.texture_width = atoi(widthStr);
      texture_object.texture_height = atoi(heightStr);

      // Ensure we got something sane for width/height
      static constexpr int saneDimension = 32768;  //??
      if (texture_object.texture_width <= 0 ||
          texture_object.texture_width > saneDimension) {
        std::cerr << "Width seems wrong. Update load_ppm if not: "
                  << texture_object.texture_width << "." << std::endl;
        return false;
      }
      if (texture_object.texture_height <= 0 ||
          texture_object.texture_height > saneDimension) {
        std::cerr << "Height seems wrong. Update load_ppm if not: "
                  << texture_object.texture_height << "." << std::endl;
        return false;
      }

      if (data_pointer == nullptr) {
        // If no destination pointer, caller only wanted dimensions
        fclose(fPtr);
        return true;
      }

      // Now read the data
      for (int y = 0; y < texture_object.texture_height; y++) {
        unsigned char *rowPtr = data_pointer;
        for (int x = 0; x < texture_object.texture_width; x++) {
          count = fread(rowPtr, 3, 1, fPtr);
          assert(count == 1);
          rowPtr[3] = 255; /* Alpha of 1 */
          rowPtr += 4;
        }
        data_pointer += row_pitch;
      }
      fclose(fPtr);

      return true;
    }

    bool create_buffer(texture_object_t &texture_object) {
      VkBufferCreateInfo buffer_create_info = {};
      buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
      buffer_create_info.pNext = nullptr;
      buffer_create_info.flags = 0;
      buffer_create_info.size =
          texture_object.texture_width * texture_object.texture_height * 4;
      buffer_create_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
      buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
      buffer_create_info.queueFamilyIndexCount = 0;
      buffer_create_info.pQueueFamilyIndices = nullptr;

      VkResult res =
          vkCreateBuffer(device_, &buffer_create_info, allocation_callbacks_,
                         &texture_object.buffer);
      if (VK_SUCCESS != res) {
        std::cerr << "Failed to create buffer." << std::endl;
        return false;
      }

      VkMemoryAllocateInfo memory_allocate_info = {};
      memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
      memory_allocate_info.pNext = nullptr;
      memory_allocate_info.allocationSize = 0;
      memory_allocate_info.memoryTypeIndex = 0;

      VkMemoryRequirements memory_requirements = {};
      vkGetBufferMemoryRequirements(device_, texture_object.buffer,
                                    &memory_requirements);
      memory_allocate_info.allocationSize = memory_requirements.size;
      texture_object.buffer_size = memory_requirements.size;

      VkFlags requirements = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                             VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
      if (!memory_type_from_properties(memory_requirements.memoryTypeBits,
                                       requirements,
                                       &memory_allocate_info.memoryTypeIndex)) {
        std::cerr << "Failed to get memory type properties for image buffer."
                  << std::endl;
        return false;
      }

      res = vkAllocateMemory(device_, &memory_allocate_info,
                             allocation_callbacks_,
                             &texture_object.buffer_memory);
      if (VK_SUCCESS != res) {
        std::cerr << "Failed to allocate buffer." << std::endl;
        return false;
      }

      VkDeviceSize memory_offset = 0;
      res = vkBindBufferMemory(device_, texture_object.buffer,
                               texture_object.buffer_memory, memory_offset);
      if (VK_SUCCESS != res) {
        std::cerr << "Failed to bind buffer." << std::endl;
        return false;
      }

      return true;
    }

    bool create_sampler(VkSampler &sampler) { return true; }

    void init_viewports() {
      VkExtent2D window_size = platform_.window_size();
      viewport_.height = window_size.height;
      viewport_.width = window_size.width;
      viewport_.minDepth = 0.0f;
      viewport_.maxDepth = 1.0f;
      viewport_.x = 0;
      viewport_.y = 0;

      static constexpr uint32_t first_viewport = 0;
      vkCmdSetViewport(command_buffers_[current_buffer_], first_viewport,
                       NUM_VIEWPORTS_AND_SCISSORS, &viewport_);
    }

    void init_scissors() {
      VkExtent2D window_size = platform_.window_size();
      scissor_.extent.height = window_size.height;
      scissor_.extent.width = window_size.width;
      scissor_.offset.x = 0;
      scissor_.offset.y = 0;

      static constexpr uint32_t first_scissor = 0;
      vkCmdSetScissor(command_buffers_[current_buffer_], first_scissor,
                      NUM_VIEWPORTS_AND_SCISSORS, &scissor_);
    }
};

}  // namespace rtx
