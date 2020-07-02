#pragma once

#include <algorithm>
#include <string>
#include <unordered_map>
#include <vector>

#include <vulkan/vulkan.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
// perspective, translate, rotate.
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include "depth_buffer.h"
#include "layer_properties.h"
#include "platform.h"
#include "swap_chain_buffer.h"
#include "uniform_data.h"
#include "vertex_buffer.h"

// shaders
#include "draw_cube.frag.h"
#include "draw_cube.vert.h"

namespace rtx {
struct Vertex {
  glm::vec3 pos;
  glm::vec2 tex_coord;

  bool operator==(const Vertex &other) const {
    return pos == other.pos && tex_coord == other.tex_coord;
  }
};
}  // namespace rtx

namespace std {
template <>
struct hash<rtx::Vertex> {
  size_t operator()(rtx::Vertex const &vertex) const {
    return (hash<glm::vec3>()(vertex.pos) ^
            (hash<glm::vec2>()(vertex.tex_coord) << 1));
  }
};
}  // namespace std

namespace rtx {

static VKAPI_ATTR VkBool32 VKAPI_CALL
debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
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
        debug_messenger_(),
        gpus_(),
        queue_props_(),
        memory_properties_(),
        device_(),
        graphics_queue_(),
        present_queue_(),
        graphics_queue_family_index_(0),
        present_queue_family_index_(0),
        gpu_properties_(),
        framebuffers_(nullptr),
        window_size_(),
        format_(),
        swap_chain_image_count_(3),
        swap_chain_(),
        buffers_(),
        current_buffer_(0),
        command_pool_(),
        command_buffers_(),
        image_acquire_semaphores_(),
        render_finished_semaphores_(),
        in_flight_fences_(),
        images_in_flight_(),
        current_frame_(0),
        depth_buffer_(),
        pipeline_layout_(),
        descriptor_layout_(),
        pipeline_cache_(),
        render_pass_(),
        pipeline_(),
        shader_stages_create_info_(),
        descriptor_pool_(),
        descriptor_set_(),
        uniform_data_(),
        texture_image_(),
        texture_image_memory_(),
        texture_image_view_(),
        texture_sampler_(),
        vertex_buffer_(),
        vertices_(),
        indices_(),
        vertex_input_binding_(),
        vertex_input_attributes_(),
        projection_(),
        view_(),
        model_(),
        clip_(),
        mvp_(),
        fov_(),
        instance_layer_properties_(),
        instance_extension_names_(),
        device_extension_names_(),
        validation_layer_names_(),
        surface_(),
        queue_family_count_(0),
        viewport_(),
        scissor_(),
        allocation_callbacks_(nullptr),
        enable_validation_layer_(debug),
        application_name_(),
        application_version_(0) {
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
      if (!setup_debug_messenger()) {
        std::cerr << "setup_debug_messenger() failed." << std::endl;
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

    if (!init_device_queue()) {
      std::cerr << "init_device_queue() failed." << std::endl;
      return false;
    }

    if (!init_pipeline_cache()) {
      std::cerr << "init_pipeline_cache() failed." << std::endl;
      return false;
    }

    if (!platform_.init_framebuffer()) {
      std::cerr << "platfom.init_framebuffer() failed." << std::endl;
      return false;
    }

    if (!init_command_pool()) {
      std::cerr << "init_command_pool() failed." << std::endl;
      return false;
    }

    if (!init_sync_objects()) {
      std::cerr << "init_sync_objects() failed." << std::endl;
      return false;
    }

    std::string model_path("assets/models/viking_room.obj");
    if (!load_model(model_path)) {
      std::cerr << "load_model() failed." << std::endl;
      return false;
    }

    if (!init_vertex_buffer()) {
      std::cerr << "init_vertex_buffer() failed." << std::endl;
      return false;
    }

    std::string texture_path("assets/textures/viking_room.png");
    if (!create_texture_image(texture_path)) {
      std::cerr << "create_texture_image() failed." << std::endl;
      return false;
    }

    if (!create_texture_image_view()) {
      std::cerr << "create_texture_image_view() failed." << std::endl;
      return false;
    }

    if (!create_texture_sampler()) {
      std::cerr << "create_texture_sampler() failed." << std::endl;
      return false;
    }

    if (!create_swap_chain()) {
      std::cerr << "create_swap_chain() failed." << std::endl;
      return false;
    }

    return true;
  }

  // bool render_frame(ImDrawData *imgui_draw_data) {
  bool render_frame() {
    VkResult res;


    static constexpr VkBool32 wait_all = VK_TRUE;
    static constexpr uint64_t timeout = UINT64_MAX;
    res = vkWaitForFences(device_, 1, &in_flight_fences_[current_frame_],
                          wait_all, timeout);
    if (VK_SUCCESS != res) {
      std::cerr << "Failed to wait for in flight fence." << std::endl;
      return false;
    }

    if (platform_.is_window_resized()) {
      platform_.set_already_resized();
      if (!recreate_swap_chain()) {
        std::cerr
            << "Failed to recreate swap chain before acquiring next image."
            << std::endl;
        return false;
      }
      return true;
    }

    // Get the index of the next available swapchain image.
    //
    res = vkAcquireNextImageKHR(device_, swap_chain_, timeout,
                                image_acquire_semaphores_[current_frame_],
                                VK_NULL_HANDLE, &current_buffer_);
    if (VK_ERROR_OUT_OF_DATE_KHR == res) {
      // std::cout << "Recreate after acquiring image." << std::endl;
      // platform_.set_already_resized();
      // if (!recreate_swap_chain()) {
      //  std::cerr << "Failed to recreate swap chain after acquiring next
      //  image."
      //            << std::endl;
      //  return false;
      //}
      // return true;
    } else if (VK_SUCCESS != res && VK_SUBOPTIMAL_KHR != res) {
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
    render_pass_begin_info.renderArea.extent.width = window_size_.width;
    render_pass_begin_info.renderArea.extent.height = window_size_.height;

    VkClearValue clear_values[2];
    // Black with alpha.
    clear_values[0].color.float32[0] = 0.0f;
    clear_values[0].color.float32[1] = 0.0f;
    clear_values[0].color.float32[2] = 0.0f;
    clear_values[0].color.float32[3] = 1.0f;
    //
    clear_values[1].depthStencil.depth =
        1.0f;  // Furthest possible depth. Range: [0.0f-1.0f]
    clear_values[1].depthStencil.stencil = 0;

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

    // Bind the index vertex buffer.
    //
    VkDeviceSize index_offset = 0;
    vkCmdBindIndexBuffer(command_buffers_[current_buffer_],
                         vertex_buffer_.index_buf, index_offset,
                         VK_INDEX_TYPE_UINT32);

    // Set the viewport and the scissors rectangle.
    //
    init_viewports();
    init_scissors();

    // Draw the vertices.
    //

    uint32_t index_count = indices_.size();
    uint32_t instance_count = 1;
    uint32_t first_vertex = 0;
    uint32_t vertex_offset = 0;
    uint32_t first_instance = 0;
    vkCmdDrawIndexed(command_buffers_[current_buffer_], index_count,
                     instance_count, first_vertex, vertex_offset,
                     first_instance);

    // Record dear imgui primitives into command buffer.
    //
    ImGui::Render();
    ImDrawData *imgui_draw_data = ImGui::GetDrawData();
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

    if (VK_ERROR_OUT_OF_DATE_KHR == res || VK_SUBOPTIMAL_KHR == res) {
      // platform_.set_already_resized();
      // std::cout << "Recreate after presenting image." << std::endl;
      // if (!recreate_swap_chain()) {
      //  std::cerr << "Failed to recreate swap chain after presenting."
      //            << std::endl;
      //  return false;
      //}
    } else if (VK_SUCCESS != res) {
      std::cerr << "Failed to present image." << std::endl;
      return false;
    }

    current_frame_ = (current_frame_ + 1) % MAX_FRAMES_IN_FLIGHT;

    return true;
  }

  bool draw() {

    bool show_demo_window = false;
    bool show_hello_world = false;
    bool show_status = true;

    while (!platform_.should_close_window()) {
      platform_.poll_events();

      // Start the Dear ImGui frame.
      ImGui_ImplVulkan_NewFrame();
      ImGui_ImplGlfw_NewFrame();
      ImGui::NewFrame();

      if (show_demo_window) {
        ImGui::ShowDemoWindow(&show_demo_window);
      }

      if (show_hello_world) {
        // Create a window called "Hello, world!" and append into it.
        ImGui::Begin("Hello, world!");

        // Edit bools storing our window open/close state.
        ImGui::Checkbox("Show Demo Window", &show_demo_window);

        ImGui::End();
      }

      if (show_status) {
        const float DISTANCE = 10.0f;
        ImGuiIO &io = ImGui::GetIO();
        ImVec2 window_pos = ImVec2(io.DisplaySize.x - DISTANCE, DISTANCE);
        ImVec2 window_pos_pivot = ImVec2(1.0f, 0.0f);
        ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);

        ImGui::SetNextWindowBgAlpha(0.35f);  // Transparent background

        ImGuiWindowFlags window_flags =
            ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav |
            ImGuiWindowFlags_NoMove;

        ImGui::Begin("Stats", &show_status, window_flags);

        ImGui::Text("GPU: %s", gpu_properties_.deviceName);

        ImGui::Separator();

        ImGui::Text("Window size: %.0f x %.0f", io.DisplaySize.x,
                    io.DisplaySize.y);
        ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
        ImGui::Text("%.3f ms/frame", 1000.0f / ImGui::GetIO().Framerate);

        ImGui::End();
      }
      if (!render_frame()) {
        std::cerr << "Rendering frame failed." << std::endl;
        break;
      }
    }
    std::cout << "Closing window." << std::endl;

    vkDeviceWaitIdle(device_);

    return true;
  }

  bool init_glfw(int width, int height, const std::string &title) {
    return platform_.init(width, height, title);
  }

  void fini_glfw() {
    std::cout << "fini_glfw." << std::endl;
    platform_.fini();
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

    if (enable_validation_layer_) {
      // Validation layer
      instance_extension_names_.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
      validation_layer_names_.push_back("VK_LAYER_KHRONOS_validation");
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

    void populate_debug_create_info(
        VkDebugUtilsMessengerCreateInfoEXT &debug_create_info) {
      debug_create_info = {};
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
      debug_create_info.pfnUserCallback = debug_callback;
    }

    bool setup_debug_messenger() {

      VkDebugUtilsMessengerCreateInfoEXT debug_create_info;
      populate_debug_create_info(debug_create_info);

      PFN_vkCreateDebugUtilsMessengerEXT func =
          (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
              instance_, "vkCreateDebugUtilsMessengerEXT");
      if (!func) {
        std::cerr << "Failed to get instance addr of debug messenger."
                  << std::endl;
        return false;
      }

      VkResult res = func(instance_, &debug_create_info, allocation_callbacks_,
                          &debug_messenger_);
      if (VK_SUCCESS != res) {
        std::cerr << "Failed to setup debug messenger." << std::endl;
        return false;
      }

      return true;
    }

    void fini_debug_messenger() {
      std::cout << "fini_debug_messenger." << std::endl;
      PFN_vkDestroyDebugUtilsMessengerEXT func =
          (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
              instance_, "vkDestroyDebugUtilsMessengerEXT");
      if (func) {
        func(instance_, debug_messenger_, allocation_callbacks_);
      }
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
      instance_create_info.flags = 0;
      instance_create_info.pApplicationInfo = &application_info;
      instance_create_info.enabledExtensionCount =
          instance_extension_names_.size();
      instance_create_info.ppEnabledExtensionNames =
          instance_extension_names_.data();

      VkDebugUtilsMessengerCreateInfoEXT debug_create_info;
      if (enable_validation_layer_) {
        populate_debug_create_info(debug_create_info);

        instance_create_info.pNext =
            (VkDebugUtilsMessengerCreateInfoEXT *)&debug_create_info;
        instance_create_info.enabledLayerCount = validation_layer_names_.size();
        instance_create_info.ppEnabledLayerNames =
            validation_layer_names_.size() ? validation_layer_names_.data()
                                           : nullptr;
      } else {
        instance_create_info.pNext = nullptr;
        instance_create_info.enabledLayerCount = 0;
      }

      VkResult res = vkCreateInstance(&instance_create_info,
                                      allocation_callbacks_, &instance_);
      if (VK_SUCCESS != res) {
        std::cerr << "Failed to create vulkan instance." << std::endl;
        return false;
      }

      return true;
    }

    void fini_instance() {
      std::cout << "fini_instance." << std::endl;
      vkDestroyInstance(instance_, allocation_callbacks_);
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

      uint32_t gpu_picked = -1;
      for (uint32_t i = 0; i < gpus_.size(); ++i) {
        VkPhysicalDeviceProperties gpu_properties;
        vkGetPhysicalDeviceProperties(gpus_[i], &gpu_properties);
        if (is_gpu_suitable(gpus_[i])) {
          std::cout << "GPU " << i << ": " << gpu_properties.deviceName
                    << " meets requirements." << std::endl;
          gpu_picked = i;
        } else {
          std::cout << "GPU " << i << " :" << gpu_properties.deviceName
                    << " doesn't meet requirements." << std::endl;
        }
      }
      if (0 != gpu_picked) {
        std::cerr << "GPU 0 doesn't support all required features."
                  << std::endl;
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

      format_ = VK_FORMAT_UNDEFINED;
      for (uint32_t i = 0; i < format_count; ++i) {
        if (VK_FORMAT_B8G8R8A8_SRGB == surface_formats[i].format) {
          format_ = VK_FORMAT_B8G8R8A8_SRGB;
          break;
        }
      }
      if (1 == format_count &&
          VK_FORMAT_UNDEFINED == surface_formats[0].format) {
        format_ = VK_FORMAT_B8G8R8A8_SRGB;
      }
      if (VK_FORMAT_B8G8R8A8_SRGB != format_) {
        std::cerr << "Unsupported surface format VK_FORMAT_B8G8R8A8_SRGB."
                  << std::endl;
        return false;
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

      VkPhysicalDeviceFeatures device_features{};
      device_features.samplerAnisotropy = VK_TRUE;
      device_create_info.pEnabledFeatures = &device_features;

      if (enable_validation_layer_) {
        device_create_info.enabledLayerCount = validation_layer_names_.size();
        device_create_info.ppEnabledLayerNames =
            validation_layer_names_.size() ? validation_layer_names_.data()
                                           : nullptr;
      } else {
        device_create_info.enabledLayerCount = 0;
      }
      device_create_info.enabledLayerCount = validation_layer_names_.size();
      device_create_info.ppEnabledLayerNames =
          validation_layer_names_.size() ? validation_layer_names_.data()
                                         : nullptr;

      VkResult res = vkCreateDevice(gpus_[0], &device_create_info,
                                    allocation_callbacks_, &device_);
      if (res != VK_SUCCESS) {
        std::cerr << "Failed to create device." << std::endl;
        return false;
      }

      return true;
    }

    void fini_device() {
      std::cout << "fini_device." << std::endl;
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
      std::cout << "fini_command_pool." << std::endl;
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

    void fini_command_buffer() {
      std::cout << "fini_command_buffer." << std::endl;
      vkFreeCommandBuffers(device_, command_pool_, swap_chain_image_count_,
                           command_buffers_.data());
      command_buffers_.clear();
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
      std::cout << "fini_sync_objects." << std::endl;
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

    bool init_imgui_resize() {
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
      ImGui_ImplVulkan_SetMinImageCount(swap_chain_image_count_);
      return true;
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

      if (!init_imgui_resize()) {
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

      VkCommandBuffer command_buffer;
      if (!begin_single_time_commands(command_buffer)) {
        std::cerr << "imgui: begin of single time command failed." << std::endl;
        return false;
      }

      if (!ImGui_ImplVulkan_CreateFontsTexture(command_buffer)) {
        std::cerr << "Failed to create imgui fonts." << std::endl;
        return false;
      }

      if (!end_single_time_commands(command_buffer)) {
        std::cerr << "imgui: end of single time command failed." << std::endl;
        return false;
      }

      ImGui_ImplVulkan_DestroyFontUploadObjects();

      return true;
    }

    void fini_imgui() {
      std::cout << "fini_imgui." << std::endl;
      ImGui_ImplGlfw_Shutdown();
      ImGui::DestroyContext();
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

    bool create_swap_chain() {
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

      if (!init_pipeline()) {
        std::cerr << "init_pipeline() failed." << std::endl;
        return false;
      }

      if (!init_framebuffers()) {
        std::cerr << "init_framebuffers() failed." << std::endl;
        return false;
      }

      if (!init_descriptor_pool()) {
        std::cerr << "init_descriptor_pool() failed." << std::endl;
        return false;
      }

      if (!init_descriptor_set()) {
        std::cerr << "init_descriptor_set() failed." << std::endl;
        return false;
      }

      if (!init_command_buffer()) {
        std::cerr << "init_command_buffer() failed." << std::endl;
        return false;
      }

      if (!init_imgui()) {
        std::cerr << "Failed to init imgui." << std::endl;
        return false;
      }
      std::cout << "Imgui ready." << std::endl;

      return true;
    }

    void cleanup_swap_chain() {
      fini_imgui();
      fini_command_buffer();
      fini_descriptor_pool();
      fini_framebuffers();
      fini_pipeline();
      fini_shaders();
      fini_render_pass();
      fini_descriptor_layout();
      fini_depth_buffer();
      fini_uniform_buffer();
      fini_swap_chain();
    }

    bool recreate_swap_chain() {
      std::cout << "Recreating swap chain." << std::endl;

      VkExtent2D window = platform_.window_size();
      while (0 == window.width || 0 == window.height) {
        window = platform_.window_size();
        platform_.wait_events();
      }

      VkResult res = vkDeviceWaitIdle(device_);
      if (VK_SUCCESS != res) {
        std::cerr << "recreate swap chain: Failed to wait for device."
                  << std::endl;
        std::cout << "Device wait result: ";
        switch (res) {
          case VK_SUCCESS:
            std::cout << "success";
            break;
          case VK_ERROR_OUT_OF_HOST_MEMORY:
            std::cout << "out of host memory";
            break;
          case VK_ERROR_OUT_OF_DEVICE_MEMORY:
            std::cout << "out of device memory";
            break;
          case VK_ERROR_DEVICE_LOST:
            std::cout << "device lost";
            break;
          default:
            std::cout << "unknown";
        }
        std::cout << "." << std::endl;
        return false;
      }

      cleanup_swap_chain();

      if (!create_swap_chain()) {
        std::cerr << "create_swap_chain() failed." << std::endl;
        return false;
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
      // If no mailbox try immediate.
      if (swap_chain_present_mode != VK_PRESENT_MODE_MAILBOX_KHR) {
        for (uint32_t i = 0; i < present_mode_count; ++i) {
          if (present_modes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR) {
            swap_chain_present_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;
            break;
          }
        }
      }
      free(present_modes);

      // Force mailbox.
      // if (swap_chain_present_mode != VK_PRESENT_MODE_MAILBOX_KHR) {
      //  swap_chain_present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
      //}

      std::cout << "Present mode: ";
      switch (swap_chain_present_mode) {
        case VK_PRESENT_MODE_IMMEDIATE_KHR:
          std::cout << "immediate";
          break;
        case VK_PRESENT_MODE_MAILBOX_KHR:
          std::cout << "mailbox";
          break;
        case VK_PRESENT_MODE_FIFO_KHR:
          std::cout << "fifo";
          break;
        case VK_PRESENT_MODE_FIFO_RELAXED_KHR:
          std::cout << "fifo relaxed";
          break;
        default:
          std::cout << swap_chain_present_mode;
      }
      std::cout << "." << std::endl;

      // Try to use triple buffering
      uint32_t triple_buffering = 3;
      swap_chain_image_count_ =
          std::max(surface_capabilities.minImageCount, triple_buffering);
      swap_chain_image_count_ =
          std::min(swap_chain_image_count_, surface_capabilities.maxImageCount);
      std::cout << "Buffering images: " << swap_chain_image_count_ << "."
                << std::endl;

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
      swap_chain_create_info.minImageCount = swap_chain_image_count_;
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

      buffers_.clear();
      for (uint32_t i = 0; i < swap_chain_image_count_; ++i) {
        swap_chain_buffer_t swap_chain_buffer;

        swap_chain_buffer.image = swap_chain_images[i];
        if (!create_image_view(swap_chain_buffer.image, format_,
                               VK_IMAGE_ASPECT_COLOR_BIT,
                               swap_chain_buffer.view)) {
          std::cerr << "Failed to create swap chain image view " << i << "."
                    << std::endl;
          return false;
        }

        buffers_.push_back(swap_chain_buffer);
      }
      free(swap_chain_images);
      current_buffer_ = 0;

      return true;
    }

    void fini_swap_chain() {
      std::cout << "fini_swap_chain." << std::endl;
      for (uint32_t i = 0; i < swap_chain_image_count_; ++i) {
        vkDestroyImageView(device_, buffers_[i].view, allocation_callbacks_);
      }
      vkDestroySwapchainKHR(device_, swap_chain_, allocation_callbacks_);
    }

    bool find_supported_format(const std::vector<VkFormat> &candidates,
                               VkImageTiling tiling,
                               VkFormatFeatureFlags features,
                               VkFormat &format) {
      for (VkFormat candidate_format : candidates) {
        VkFormatProperties format_properties;
        vkGetPhysicalDeviceFormatProperties(gpus_[0], candidate_format,
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

    bool find_depth_format(VkFormat &depth_format) {
      return find_supported_format(
          {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT,
           VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D16_UNORM},
          VK_IMAGE_TILING_OPTIMAL,
          VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT, depth_format);
    }

    bool has_stencil_component(VkFormat format) {
      return format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
             format == VK_FORMAT_D24_UNORM_S8_UINT;
    }

    bool init_depth_buffer() {
      VkFormat depth_format;
      if (!find_depth_format(depth_format)) {
        std::cerr << "Failed to find asupported depth format." << std::endl;
        return false;
      }

      VkExtent2D window_size = platform_.window_size();

      if (!create_image(window_size.width, window_size.height, depth_format,
                        VK_IMAGE_TILING_OPTIMAL,
                        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                        depth_buffer_.image, depth_buffer_.mem)) {
        std::cerr << "Failed to create depth buffer image." << std::endl;
        return false;
      }

      if (!create_image_view(depth_buffer_.image, depth_format,
                             VK_IMAGE_ASPECT_DEPTH_BIT, depth_buffer_.view)) {
        std::cerr << "Failed to create depth buffer image view." << std::endl;
        return false;
      }

      depth_buffer_.format = depth_format;

      // There is no need to transition the layout of the image to a depth
      // attachment because that will happen on the render pass.

      return true;
    }

    void fini_depth_buffer() {
      std::cout << "fini_depth_buffer." << std::endl;
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
      auto eye = glm::vec3(2.0, 2.0, 2.0);
      auto center = glm::vec3(0.0, 0.0, 0.0);
      auto up = glm::vec3(0.0, 0.0, 1.0);
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
      std::cout << "fini_uniform_buffer." << std::endl;
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

        layout_bindings[1].binding = 1;
        layout_bindings[1].descriptorType =
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        layout_bindings[1].descriptorCount = 1;
        layout_bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        layout_bindings[1].pImmutableSamplers = nullptr;

      VkDescriptorSetLayoutCreateInfo descriptor_layout = {};
      descriptor_layout.sType =
          VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
      descriptor_layout.pNext = nullptr;
      descriptor_layout.flags = 0;
      descriptor_layout.bindingCount = 2;
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
      std::cout << "fini_descriptor_layout." << std::endl;
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

      // Depth buffer attachment.
      //
      attachments[1].format = depth_buffer_.format;
      attachments[1].samples = NUM_SAMPLES;
      attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
      attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      attachments[1].finalLayout =
          VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
      attachments[1].flags = 0;

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
      subpass_description.pDepthStencilAttachment = &depth_reference;
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
      render_pass_create_info.attachmentCount = 2;
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
      std::cout << "fini_render_pass." << std::endl;
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
      std::cout << "fini_shaders." << std::endl;
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
      framebuffer_create_info.attachmentCount = 2;
      framebuffer_create_info.pAttachments = attachments;

      window_size_ = platform_.window_size();
      framebuffer_create_info.width = window_size_.width;
      framebuffer_create_info.height = window_size_.height;
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
      std::cout << "fini_framebuffers." << std::endl;
      for (uint32_t i = 0; i < swap_chain_image_count_; ++i) {
        vkDestroyFramebuffer(device_, framebuffers_[i], allocation_callbacks_);
      }
      free(framebuffers_);
      framebuffers_ = nullptr;
    }

    bool init_vertex_buffer() {
      const void *vertex_data = vertices_.data();
      uint32_t vertex_data_size = sizeof(vertices_[0]) * vertices_.size();
      uint32_t data_stride = sizeof(vertices_[0]);

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
      vertex_input_attributes_[0].format = VK_FORMAT_R32G32B32_SFLOAT;
      vertex_input_attributes_[0].offset = offsetof(Vertex, pos);

      vertex_input_attributes_[1].binding = 0;
      vertex_input_attributes_[1].location = 1;
      vertex_input_attributes_[1].format =
          VK_FORMAT_R32G32_SFLOAT;  // texture coordinates are 2D.
                                    // VK_FORMAT_R32G32B32A32_SFLOAT;  // colors
                                    // are rgba.
      vertex_input_attributes_[1].offset =
          offsetof(Vertex, tex_coord);  // After the vexter position.

      if (!init_vertex_index_buffer()) {
        std::cerr << "init_vertex_index_buffer() failed." << std::endl;
        return false;
      }

      return true;
    }

    bool init_vertex_index_buffer() {
      VkDeviceSize buffer_size = sizeof(indices_[0]) * indices_.size();

      VkBuffer staging_buffer;
      VkDeviceMemory statging_buffer_memory;

      VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
      VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                         VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
      if (!create_buffer(buffer_size, usage, properties, staging_buffer,
                         statging_buffer_memory)) {
        std::cerr << "Failed to create vertex index staging buffer."
                  << std::endl;
        return false;
      }

      void *data;
      VkDeviceSize offset = 0;
      VkMemoryMapFlags flags = 0;
      vkMapMemory(device_, statging_buffer_memory, offset, buffer_size, flags,
                  &data);
      memcpy(data, indices_.data(), (size_t)buffer_size);
      vkUnmapMemory(device_, statging_buffer_memory);

      usage =
          VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
      properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
      if (!create_buffer(buffer_size, usage, properties,
                         vertex_buffer_.index_buf, vertex_buffer_.index_mem)) {
        std::cerr << "Failed to create vertex index buffer." << std::endl;
        return false;
      }

      if (!copy_buffer(staging_buffer, vertex_buffer_.index_buf, buffer_size)) {
        std::cerr << "Failed to copy vertex index staging buffer to vertex "
                     "index buffer."
                  << std::endl;
        return false;
      }

      vkDestroyBuffer(device_, staging_buffer, allocation_callbacks_);
      vkFreeMemory(device_, statging_buffer_memory, allocation_callbacks_);

      return true;
    }

    void fini_vertex_buffer() {
      std::cout << "fini_vertex_buffer." << std::endl;

      vkDestroyBuffer(device_, vertex_buffer_.index_buf, allocation_callbacks_);
      vkFreeMemory(device_, vertex_buffer_.index_mem, allocation_callbacks_);

      vkDestroyBuffer(device_, vertex_buffer_.buf, allocation_callbacks_);
      vkFreeMemory(device_, vertex_buffer_.mem, allocation_callbacks_);
    }

    bool load_model(const std::string &model_path) {
      tinyobj::attrib_t attrib;
      std::vector<tinyobj::shape_t> shapes;
      std::vector<tinyobj::material_t> materials;
      std::string warn, err;

      if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err,
                            model_path.c_str())) {
        std::cerr << "Failed to load model: " << warn << ", " << err << "."
                  << std::endl;
        return false;
      }

      std::unordered_map<Vertex, uint32_t> unique_vertices;

      for (const auto &shape : shapes) {
        for (const auto &index : shape.mesh.indices) {
          Vertex vertex{};

          vertex.pos = {attrib.vertices[3 * index.vertex_index + 0],
                        attrib.vertices[3 * index.vertex_index + 1],
                        attrib.vertices[3 * index.vertex_index + 2]};

          vertex.tex_coord = {
              attrib.texcoords[2 * index.texcoord_index + 0],
              1.0f - attrib.texcoords[2 * index.texcoord_index + 1]};

          if (unique_vertices.count(vertex) == 0) {
            unique_vertices[vertex] = static_cast<uint32_t>(vertices_.size());
            vertices_.push_back(vertex);
          }
          indices_.push_back(unique_vertices[vertex]);
        }
      }

      return true;
    }

    bool init_descriptor_pool() {
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
      std::cout << "fini_descriptor_pool." << std::endl;
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

      // Uniform buffer
      //
      write_descriptor_set[0] = {};
      write_descriptor_set[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      write_descriptor_set[0].pNext = nullptr;
      write_descriptor_set[0].dstSet = descriptor_set_[0];
      write_descriptor_set[0].descriptorCount = 1;
      write_descriptor_set[0].descriptorType =
          VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      write_descriptor_set[0].pBufferInfo = &uniform_data_.buffer_info;
      write_descriptor_set[0].pImageInfo = nullptr;
      write_descriptor_set[0].dstArrayElement = 0;
      write_descriptor_set[0].dstBinding = 0;

      // Texture
      //
      VkDescriptorImageInfo image_info{};
      image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      image_info.imageView = texture_image_view_;
      image_info.sampler = texture_sampler_;

      write_descriptor_set[1] = {};
      write_descriptor_set[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      write_descriptor_set[1].pNext = nullptr;
      write_descriptor_set[1].dstSet = descriptor_set_[0];
      write_descriptor_set[1].descriptorCount = 1;
      write_descriptor_set[1].descriptorType =
          VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      write_descriptor_set[1].pImageInfo = &image_info;
      write_descriptor_set[1].dstArrayElement = 0;
      write_descriptor_set[1].dstBinding = 1;

      uint32_t descriptor_copy_count = 0;
      const VkCopyDescriptorSet *descriptor_copies = nullptr;
      vkUpdateDescriptorSets(device_, 2, write_descriptor_set,
                             descriptor_copy_count, descriptor_copies);

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
      std::cout << "fini_pipeline_cache." << std::endl;
      vkDestroyPipelineCache(device_, pipeline_cache_, allocation_callbacks_);
    }

    bool init_pipeline() {
      std::cout << "Hi pipeline." << std::endl;

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
      vi.pNext = nullptr;
      vi.flags = 0;
      vi.vertexBindingDescriptionCount = 1;
      vi.pVertexBindingDescriptions = &vertex_input_binding_;
      vi.vertexAttributeDescriptionCount = 2;
      vi.pVertexAttributeDescriptions = vertex_input_attributes_;

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
      rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
      rs.depthClampEnable = VK_FALSE;
      rs.rasterizerDiscardEnable = VK_FALSE;
      rs.depthBiasEnable = VK_FALSE;
      rs.depthBiasConstantFactor = 0;
      rs.depthBiasClamp = 0;
      rs.depthBiasSlopeFactor = 0;
      rs.lineWidth = 1.0f;

      // Pipeline Color Blend State
      //
      VkPipelineColorBlendAttachmentState cb_attachment_state;
      cb_attachment_state.colorWriteMask =
          VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
      cb_attachment_state.blendEnable = VK_FALSE;
      cb_attachment_state.alphaBlendOp = VK_BLEND_OP_ADD;
      cb_attachment_state.colorBlendOp = VK_BLEND_OP_ADD;
      cb_attachment_state.srcColorBlendFactor = VK_BLEND_FACTOR_ZERO;
      cb_attachment_state.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
      cb_attachment_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
      cb_attachment_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;

      VkPipelineColorBlendStateCreateInfo cb;
      cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
      cb.flags = 0;
      cb.pNext = nullptr;
      cb.attachmentCount = 1;
      cb.pAttachments = &cb_attachment_state;
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

      ds.depthTestEnable = VK_TRUE;   // True because depth buffer used.
      ds.depthWriteEnable = VK_TRUE;  // True because depth buffer used.

      ds.depthCompareOp = VK_COMPARE_OP_LESS;  // Lower depth = closer.

      ds.depthBoundsTestEnable = VK_FALSE;
      ds.minDepthBounds = 0;
      ds.maxDepthBounds = 0;

      ds.stencilTestEnable = VK_FALSE;  // Stencil not used.
      ds.back.failOp = VK_STENCIL_OP_KEEP;
      ds.back.passOp = VK_STENCIL_OP_KEEP;
      ds.back.compareOp = VK_COMPARE_OP_ALWAYS;
      ds.back.compareMask = 0;
      ds.back.reference = 0;
      ds.back.depthFailOp = VK_STENCIL_OP_KEEP;
      ds.back.writeMask = 0;
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
      std::cout << "fini_pipeline." << std::endl;
      std::cout << "Bye pipeline." << std::endl;
      vkDestroyPipeline(device_, pipeline_, allocation_callbacks_);
    }

    void fini() {
      cleanup_swap_chain();

      cleanup_texture_sampler();
      cleanup_texture_image_view();
      cleanup_texture_image();

      fini_vertex_buffer();

      fini_sync_objects();

      fini_command_pool();

      fini_pipeline_cache();

      fini_device();

      if (enable_validation_layer_) {
        fini_debug_messenger();
      }

      vkDestroySurfaceKHR(instance_, surface_, allocation_callbacks_);

      fini_glfw();

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
    VkExtent2D window_size_;
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

    VkImage texture_image_;
    VkDeviceMemory texture_image_memory_;
    VkImageView texture_image_view_;
    VkSampler texture_sampler_;

    vertex_buffer_t vertex_buffer_;
    std::vector<Vertex> vertices_;
    std::vector<uint32_t> indices_;

    VkVertexInputBindingDescription vertex_input_binding_;
    VkVertexInputAttributeDescription vertex_input_attributes_[2];


    glm::mat4 projection_;
    glm::mat4 view_;
    glm::mat4 model_;
    glm::mat4 clip_;
    glm::mat4 mvp_;
    float fov_;

    std::vector<layer_properties_t> instance_layer_properties_;
    std::vector<const char *> instance_extension_names_;
    std::vector<const char *> device_extension_names_;
    std::vector<const char *> validation_layer_names_;

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

    bool is_gpu_suitable(VkPhysicalDevice gpu) {
      VkPhysicalDeviceFeatures supported_features;
      vkGetPhysicalDeviceFeatures(gpu, &supported_features);

      return supported_features.samplerAnisotropy;
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
        std::cerr << "Failed to create buffer." << std::endl;
        return false;
      }

      VkMemoryRequirements memory_requirements;
      vkGetBufferMemoryRequirements(device_, buffer, &memory_requirements);

      VkMemoryAllocateInfo memory_allocate_info{};
      memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
      memory_allocate_info.pNext = nullptr;
      memory_allocate_info.allocationSize = memory_requirements.size;

      if (!memory_type_from_properties(memory_requirements.memoryTypeBits,
                                       properties,
                                       &memory_allocate_info.memoryTypeIndex)) {
        std::cerr << "Failed to get memory type properties for image buffer."
                  << std::endl;
        return false;
      }

      res = vkAllocateMemory(device_, &memory_allocate_info,
                             allocation_callbacks_, &buffer_memory);
      if (VK_SUCCESS != res) {
        std::cerr << "Failed to allocate buffer." << std::endl;
        return false;
      }

      VkDeviceSize memory_offset = 0;
      res = vkBindBufferMemory(device_, buffer, buffer_memory, memory_offset);
      if (VK_SUCCESS != res) {
        std::cerr << "Failed to bind buffer." << std::endl;
        return false;
      }

      return true;
    }

    bool create_image(uint32_t width, uint32_t height, VkFormat format,
                      VkImageTiling tiling, VkImageUsageFlags usage,
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
          NUM_SAMPLES;  // TODO: Always VK_SAMPLE_COUNT_1_BIT?
      image_create_info.flags = 0;

      VkResult res = vkCreateImage(device_, &image_create_info,
                                   allocation_callbacks_, &image);
      if (VK_SUCCESS != res) {
        std::cerr << "Failed to create texture image." << std::endl;
        return false;
      }

      VkMemoryRequirements memory_requirements;
      vkGetImageMemoryRequirements(device_, image, &memory_requirements);

      VkMemoryAllocateInfo memory_allocate_info{};
      memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
      memory_allocate_info.pNext = nullptr;
      memory_allocate_info.allocationSize = memory_requirements.size;

      if (!memory_type_from_properties(memory_requirements.memoryTypeBits,
                                       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                       &memory_allocate_info.memoryTypeIndex)) {
        std::cerr << "Failed to get memory type properties for texture image."
                  << std::endl;
        return false;
      }
      res = vkAllocateMemory(device_, &memory_allocate_info,
                             allocation_callbacks_, &image_memory);
      if (VK_SUCCESS != res) {
        std::cerr << "Failed to allocate texture image memory." << std::endl;
        return false;
      }

      VkDeviceSize memory_offset = 0;
      vkBindImageMemory(device_, image, image_memory, memory_offset);

      return true;
    }

    bool create_texture_image(const std::string &texture_path) {
      int texture_width, texture_height, texture_channels;
      stbi_uc *pixels =
          stbi_load(texture_path.c_str(), &texture_width, &texture_height,
                    &texture_channels, STBI_rgb_alpha);
      if (!pixels) {
        std::cerr << "Failed to load texture." << std::endl;
        return false;
      }

      VkDeviceSize image_size = texture_width * texture_height * 4;
      VkBuffer staging_buffer;
      VkDeviceMemory staging_buffer_memory;
      VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
      VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                         VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
      if (!create_buffer(image_size, usage, properties, staging_buffer,
                         staging_buffer_memory)) {
        std::cerr << "Failed to create texture buffer." << std::endl;
        return false;
      }

      void *pixel_data;
      VkDeviceSize offset = 0;
      VkMemoryMapFlags flags = 0;
      vkMapMemory(device_, staging_buffer_memory, offset, image_size, flags,
                  &pixel_data);
      memcpy(pixel_data, pixels, static_cast<size_t>(image_size));
      vkUnmapMemory(device_, staging_buffer_memory);

      stbi_image_free(pixels);

      if (!create_image(
              static_cast<uint32_t>(texture_width),
              static_cast<uint32_t>(texture_height), VK_FORMAT_R8G8B8A8_SRGB,
              VK_IMAGE_TILING_OPTIMAL,
              VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
              VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, texture_image_,
              texture_image_memory_)) {
        std::cerr << "Failed to create texture image." << std::endl;
        return false;
      }

      if (!transition_image_layout(texture_image_, VK_FORMAT_R8G8B8A8_SRGB,
                                   VK_IMAGE_LAYOUT_UNDEFINED,
                                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)) {
        std::cerr << "Transition of texture image to copy failed." << std::endl;
        return false;
      }

      if (!copy_buffer_to_image(staging_buffer, texture_image_,
                                static_cast<uint32_t>(texture_width),
                                static_cast<uint32_t>(texture_height))) {
        std::cerr << "Failed to copy texture buffer to texture image."
                  << std::endl;
        return false;
      }

      if (!transition_image_layout(texture_image_, VK_FORMAT_R8G8B8A8_SRGB,
                                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)) {
        std::cerr << "Transition of texture image to read failed." << std::endl;
        return false;
      }

      vkDestroyBuffer(device_, staging_buffer, allocation_callbacks_);
      vkFreeMemory(device_, staging_buffer_memory, allocation_callbacks_);

      return true;
    }

    void cleanup_texture_image() {
      vkDestroyImage(device_, texture_image_, allocation_callbacks_);
      vkFreeMemory(device_, texture_image_memory_, allocation_callbacks_);
    }

    bool create_image_view(VkImage image, VkFormat format,
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

      VkResult res = vkCreateImageView(device_, &image_view_create_info,
                                       allocation_callbacks_, &image_view);
      if (VK_SUCCESS != res) {
        std::cerr << "Failed to create image view." << std::endl;
        return false;
      }

      return true;
    }

    bool create_texture_image_view() {
      if (!create_image_view(texture_image_, VK_FORMAT_R8G8B8A8_SRGB,
                             VK_IMAGE_ASPECT_COLOR_BIT, texture_image_view_)) {
        std::cerr << "Failed to create texture image view." << std::endl;
        return false;
      }
      if (VK_NULL_HANDLE == texture_image_view_) {
        std::cerr << "texture_image_view_ is null." << std::endl;
        return false;
      }
      return true;
    }

    void cleanup_texture_image_view() {
      vkDestroyImageView(device_, texture_image_view_, allocation_callbacks_);
    }

    bool create_texture_sampler() {
      VkSamplerCreateInfo sampler_create_info{};
      sampler_create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
      sampler_create_info.pNext = nullptr;

      sampler_create_info.magFilter = VK_FILTER_LINEAR;
      sampler_create_info.minFilter = VK_FILTER_LINEAR;

      sampler_create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
      sampler_create_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
      sampler_create_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

      sampler_create_info.anisotropyEnable = VK_TRUE;
      sampler_create_info.maxAnisotropy = 16.0f;

      sampler_create_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;

      sampler_create_info.unnormalizedCoordinates = VK_FALSE;

      sampler_create_info.compareEnable = VK_FALSE;
      sampler_create_info.compareOp = VK_COMPARE_OP_ALWAYS;

      sampler_create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
      sampler_create_info.mipLodBias = 0.0f;
      sampler_create_info.minLod = 0.0f;
      sampler_create_info.maxLod = 0.0f;

      VkResult res = vkCreateSampler(device_, &sampler_create_info,
                                     allocation_callbacks_, &texture_sampler_);
      if (VK_SUCCESS != res) {
        std::cerr << "Failed to create texture sampler." << std::endl;
        return false;
      }

      return true;
    }

    void cleanup_texture_sampler() {
      vkDestroySampler(device_, texture_sampler_, allocation_callbacks_);
    }

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

    bool begin_single_time_commands(VkCommandBuffer &command_buffer) {
      VkCommandBufferAllocateInfo cmd_allocate_info{};
      cmd_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
      cmd_allocate_info.pNext = nullptr;
      cmd_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
      cmd_allocate_info.commandPool = command_pool_;
      cmd_allocate_info.commandBufferCount = 1;

      VkResult res = vkAllocateCommandBuffers(device_, &cmd_allocate_info,
                                              &command_buffer);
      if (VK_SUCCESS != res) {
        std::cerr << "Failed to create single time command buffer."
                  << std::endl;
        return false;
      }

      VkCommandBufferBeginInfo cmd_begin_info{};
      cmd_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
      cmd_begin_info.pNext = nullptr;
      cmd_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

      res = vkBeginCommandBuffer(command_buffer, &cmd_begin_info);
      if (VK_SUCCESS != res) {
        std::cerr << "Failed to begin single time command buffer." << std::endl;
        return false;
      }

      return true;
    }

    bool end_single_time_commands(VkCommandBuffer &command_buffer) {
      VkResult res = vkEndCommandBuffer(command_buffer);
      if (VK_SUCCESS != res) {
        std::cerr
            << "Failed to complete recording of single time command buffer."
            << std::endl;
        return false;
      }

      VkSubmitInfo submit_info{};
      submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
      submit_info.commandBufferCount = 1;
      submit_info.pCommandBuffers = &command_buffer;

      res = vkQueueSubmit(graphics_queue_, 1, &submit_info, VK_NULL_HANDLE);
      if (VK_SUCCESS != res) {
        std::cerr
            << "Failed to submit single time command buffer to graphics queue."
            << std::endl;
        return false;
      }

      res = vkQueueWaitIdle(graphics_queue_);
      if (VK_SUCCESS != res) {
        std::cerr << "Failed to wait for graphics queue to complete execution "
                     "of single time command buffer."
                  << std::endl;
        return false;
      }

      vkFreeCommandBuffers(device_, command_pool_, 1, &command_buffer);

      return true;
    }

    bool copy_buffer(VkBuffer src_buffer, VkBuffer dst_buffer,
                     VkDeviceSize size) {
      VkCommandBuffer command_buffer;
      if (!begin_single_time_commands(command_buffer)) {
        std::cerr << "copy buffer: begin of single time command failed."
                  << std::endl;
        return false;
      }

      VkBufferCopy copy_region{};
      copy_region.size = size;

      static constexpr uint32_t region_count = 1;
      vkCmdCopyBuffer(command_buffer, src_buffer, dst_buffer, region_count,
                      &copy_region);

      if (!end_single_time_commands(command_buffer)) {
        std::cerr << "copy buffer: end of single time command failed."
                  << std::endl;
        return false;
      }

      return true;
    }

    bool transition_image_layout(VkImage image, VkFormat format,
                                 VkImageLayout old_layout,
                                 VkImageLayout new_layout) {
      VkCommandBuffer command_buffer;
      if (!begin_single_time_commands(command_buffer)) {
        std::cerr
            << "transition image layout: begin of single time command failed."
            << std::endl;
        return false;
      }

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

      VkPipelineStageFlags src_stage_mask;
      VkPipelineStageFlags dst_stage_mask;

      if (VK_IMAGE_LAYOUT_UNDEFINED == old_layout &&
          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL == new_layout) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        src_stage_mask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dst_stage_mask = VK_PIPELINE_STAGE_TRANSFER_BIT;
      } else if (VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL == old_layout &&
                 VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL == new_layout) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        src_stage_mask = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dst_stage_mask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
      } else {
        std::cerr << "Unsupported layout transition." << std::endl;
        return false;
      }

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

      if (!end_single_time_commands(command_buffer)) {
        std::cerr
            << "transition image layout: end of single time command failed."
            << std::endl;
        return false;
      }

      return true;
    }

    bool copy_buffer_to_image(VkBuffer buffer, VkImage image, uint32_t width,
                              uint32_t height) {
      VkCommandBuffer command_buffer;
      if (!begin_single_time_commands(command_buffer)) {
        std::cerr
            << "transition image layout: begin of single time command failed."
            << std::endl;
        return false;
      }

      VkBufferImageCopy region{};
      region.bufferOffset = 0;
      region.bufferRowLength = 0;
      region.bufferImageHeight = 0;
      region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      region.imageSubresource.mipLevel = 0;
      region.imageSubresource.baseArrayLayer = 0;
      region.imageSubresource.layerCount = 1;
      region.imageOffset = {0, 0, 0};
      region.imageExtent = {width, height, 1};

      static constexpr uint32_t region_count = 1;
      vkCmdCopyBufferToImage(command_buffer, buffer, image,
                             VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, region_count,
                             &region);

      if (!end_single_time_commands(command_buffer)) {
        std::cerr
            << "transition image layout: end of single time command failed."
            << std::endl;
        return false;
      }

      return true;
    }
};

}  // namespace rtx
