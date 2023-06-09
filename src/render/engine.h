#pragma once

#include <algorithm>
#include <string>
#include <unordered_map>
#include <vector>

#include <vulkan/vulkan.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include "acceleration_structure.h"
#include "camera.h"
#include "constants.h"
#include "depth_buffer.h"
#include "glm.h"
#include "helpers.h"
#include "layer_properties.h"
#include "memory.h"
#include "object.h"
#include "platform.h"
#include "ray_tracing_extensions.h"
#include "raytracing/descriptor_pool.h"
#include "raytracing/ray_tracer.h"
#include "swap_chain_buffer.h"
#include "uniform_data.h"
#include "vertex.h"

// shaders
#include "draw_cube.frag.h"
#include "draw_cube.vert.h"

// Ray tracing shaders
#include "raytrace.rchit.h"
#include "raytrace.rgen.h"
#include "raytrace.rmiss.h"
#include "raytrace_shadow.rmiss.h"

namespace rtx {

static VKAPI_ATTR VkBool32 VKAPI_CALL
debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
               VkDebugUtilsMessageTypeFlagsEXT messageType,
               const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
               void *pUserData) {
  // std::cerr << "Validation layer: " << pCallbackData->pMessage << std::endl;

  return VK_FALSE;
}

std::ostream &operator<<(std::ostream &os, VkResult res) {
  switch (res) {
    // Success codes
    //
    case VK_SUCCESS:
      os << "VK_SUCCESS Command successfully completed";
      break;
    case VK_NOT_READY:
      os << "VK_NOT_READY A fence or query has not yet completed";
      break;
    case VK_TIMEOUT:
      os << "VK_TIMEOUT A wait operation has not completed in the specified "
            "time";
      break;
    case VK_EVENT_SET:
      os << "VK_EVENT_SET An event is signaled";
      break;
    case VK_EVENT_RESET:
      os << "VK_EVENT_RESET An event is unsignaled";
      break;
    case VK_INCOMPLETE:
      os << "VK_INCOMPLETE A return array was too small for the result";
      break;
    case VK_SUBOPTIMAL_KHR:
      os << "VK_SUBOPTIMAL_KHR A swapchain no longer matches the surface "
            "properties exactly, but can still be used to present to the "
            "surface successfully";
      break;
    case VK_THREAD_IDLE_KHR:
      os << "VK_THREAD_IDLE_KHR A deferred operation is not complete but there "
            "is currently no work for this thread to do at the time of this "
            "call";
      break;
    case VK_THREAD_DONE_KHR:
      os << "VK_THREAD_DONE_KHR A deferred operation is not complete but there "
            "is no work remaining to assign to additional threads";
      break;
    case VK_OPERATION_DEFERRED_KHR:
      os << "VK_OPERATION_DEFERRED_KHR A deferred operation was requested and "
            "at least some of the work was deferred";
      break;
    case VK_OPERATION_NOT_DEFERRED_KHR:
      os << "VK_OPERATION_NOT_DEFERRED_KHR A deferred operation was requested "
            "and no operations were deferred";
      break;
#ifdef VK_VERSION_1_3
    case VK_PIPELINE_COMPILE_REQUIRED:
      os << "VK_PIPELINE_COMPILE_REQUIRED A requested pipeline creation would "
            "have required compilation, but the application requested "
            "compilation to not be performed";
      break;
#endif  // VK_VERSION_1_3

      // Error codes
      //
    case VK_ERROR_OUT_OF_HOST_MEMORY:
      os << "VK_ERROR_OUT_OF_HOST_MEMORY A host memory allocation has failed";
      break;
    case VK_ERROR_OUT_OF_DEVICE_MEMORY:
      os << "VK_ERROR_OUT_OF_DEVICE_MEMORY A device memory allocation has "
            "failed";
      break;
    case VK_ERROR_INITIALIZATION_FAILED:
      os << "VK_ERROR_INITIALIZATION_FAILED Initialization of an object could "
            "not be completed for implementation-specific reasons";
      break;
    case VK_ERROR_DEVICE_LOST:
      os << "VK_ERROR_DEVICE_LOST The logical or physical device has been "
            "lost. See [Lost "
            "Device](https://registry.khronos.org/vulkan/specs/1.3-extensions/"
            "html/vkspec.html#devsandqueues-lost-device)";
      break;
    case VK_ERROR_MEMORY_MAP_FAILED:
      os << "VK_ERROR_MEMORY_MAP_FAILED Mapping of a memory object has failed";
      break;
    case VK_ERROR_LAYER_NOT_PRESENT:
      os << "VK_ERROR_LAYER_NOT_PRESENT A requested layer is not present or "
            "could not be loaded";
      break;
    case VK_ERROR_EXTENSION_NOT_PRESENT:
      os << "VK_ERROR_EXTENSION_NOT_PRESENT A requested extension is not "
            "supported";
      break;
    case VK_ERROR_FEATURE_NOT_PRESENT:
      os << "VK_ERROR_FEATURE_NOT_PRESENT A requested feature is not supported";
      break;
    case VK_ERROR_INCOMPATIBLE_DRIVER:
      os << "VK_ERROR_INCOMPATIBLE_DRIVER The requested version of Vulkan is "
            "not supported by the driver or is otherwise incompatible for "
            "implementation-specific reasons";
      break;
    case VK_ERROR_TOO_MANY_OBJECTS:
      os << "VK_ERROR_TOO_MANY_OBJECTS Too many objects of the type have "
            "already been created";
      break;
    case VK_ERROR_FORMAT_NOT_SUPPORTED:
      os << "VK_ERROR_FORMAT_NOT_SUPPORTED A requested format is not supported "
            "on this device.";
      break;
    case VK_ERROR_FRAGMENTED_POOL:
      os << "VK_ERROR_FRAGMENTED_POOL A pool allocation has failed due to "
            "fragmentation of the pool’s memory. This must only be returned if "
            "no attempt to allocate host or device memory was made to "
            "accommodate the new allocation. This should be returned in "
            "preference to VK_ERROR_OUT_OF_POOL_MEMORY, but only if the "
            "implementation is certain that the pool allocation failure was "
            "due to fragmentation";
      break;
    case VK_ERROR_SURFACE_LOST_KHR:
      os << "VK_ERROR_SURFACE_LOST_KHR A surface is no longer available";
      break;
    case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
      os << "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR The requested window is already "
            "in use by Vulkan or another API in a manner which prevents it "
            "from being used again";
      break;
    case VK_ERROR_OUT_OF_DATE_KHR:
      os << "VK_ERROR_OUT_OF_DATE_KHR A surface has changed in such a way that "
            "it is no longer compatible with the swapchain, and further "
            "presentation requests using the swapchain will fail. Applications "
            "must query the new surface properties and recreate their "
            "swapchain if they wish to continue presenting to the surface";
      break;
    case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
      os << "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR The display used by a swapchain "
            "does not use the same presentable image layout, or is "
            "incompatible in a way that prevents sharing an image";
      break;
    case VK_ERROR_INVALID_SHADER_NV:
      os << "VK_ERROR_INVALID_SHADER_NV One or more shaders failed to compile "
            "or link. More details are reported back to the application via "
            "VK_EXT_debug_report if enabled";
      break;
    case VK_ERROR_OUT_OF_POOL_MEMORY:
      os << "VK_ERROR_OUT_OF_POOL_MEMORY A pool memory allocation has failed. "
            "This must only be returned if no attempt to allocate host or "
            "device memory was made to accommodate the new allocation. If the "
            "failure was definitely due to fragmentation of the pool, "
            "VK_ERROR_FRAGMENTED_POOL should be returned instead";
      break;
    case VK_ERROR_INVALID_EXTERNAL_HANDLE:
      os << "VK_ERROR_INVALID_EXTERNAL_HANDLE An external handle is not a "
            "valid handle of the specified type";
      break;
    case VK_ERROR_FRAGMENTATION:
      os << "VK_ERROR_FRAGMENTATION A descriptor pool creation has failed due "
            "to fragmentation";
      break;
    case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS:
      os << "VK_ERROR_INVALID_DEVICE_ADDRESS_EXT A buffer creation failed "
            "because the requested address is not available";
      os << ", or ";
      os << "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS A buffer creation or "
            "memory allocation failed because the requested address is not "
            "available. A shader group handle assignment failed because the "
            "requested shader group handle information is no longer valid";
      break;
    case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT:
      os << "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT An operation on a "
            "swapchain created with "
            "VK_FULL_SCREEN_EXCLUSIVE_APPLICATION_CONTROLLED_EXT failed as it "
            "did not have exclusive full-screen access. This may occur due to "
            "implementation-dependent reasons, outside of the application’s "
            "control";
      break;
#ifdef VK_EXT_IMAGE_COMPRESSION_CONTROL_EXTENSION_NAME
    case VK_ERROR_COMPRESSION_EXHAUSTED_EXT:
      os << "VK_ERROR_COMPRESSION_EXHAUSTED_EXT An image creation failed "
            "because internal resources required for compression are "
            "exhausted. This must only be returned when fixed-rate compression "
            "is requested";
      break;
#endif  // VK_EXT_IMAGE_COMPRESSION_CONTROL_EXTENSION_NAME
#ifdef VK_KHR_VIDEO_QUEUE_EXTENSION_NAME
    case VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR:
      os << "VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR The requested "
            "VkImageUsageFlags are not supported";
      break;
    case VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR:
      os << "VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR The requested "
            "video picture layout is not supported";
      break;
    case VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR:
      os << "VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR A video "
            "profile operation specified via "
            "VkVideoProfileInfoKHR::videoCodecOperation is not supported";
      break;
    case VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR:
      os << "VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR Format parameters "
            "in a requested VkVideoProfileInfoKHR chain are not supported";
      break;
    case VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR:
      os << "VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR Codec-specific "
            "parameters in a requested VkVideoProfileInfoKHR chain are not "
            "supported";
      break;
    case VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR:
      os << "VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR The specified video "
            "Std header version is not supported";
      break;
#endif  // VK_KHR_VIDEO_QUEUE_EXTENSION_NAME
#ifdef VK_ENABLE_BETA_EXTENSIONS
    case VK_ERROR_INVALID_VIDEO_STD_PARAMETERS_KHR:
      os << "VK_ERROR_INVALID_VIDEO_STD_PARAMETERS_KHR The specified Video Std "
            "parameters do not adhere to the syntactic or semantic "
            "requirements of the used video compression standard, or values "
            "derived from parameters according to the rules defined by the "
            "used video compression standard do not adhere to the capabilities "
            "of the video compression standard or the implementation";
      break;
#endif  // VK_ENABLE_BETA_EXTENSIONS
#ifdef VK_EXT_SHADER_OBJECT_EXTENSION_NAME
    case VK_ERROR_INCOMPATIBLE_SHADER_BINARY_EXT:
      os << "VK_ERROR_INCOMPATIBLE_SHADER_BINARY_EXT The provided binary "
            "shader code is not compatible with this device";
      break;
#endif  // VK_EXT_SHADER_OBJECT_EXTENSION_NAME
    case VK_ERROR_UNKNOWN:
      os << "VK_ERROR_UNKNOWN An unknown error has occurred; either the "
            "application has provided invalid input, or an implementation "
            "failure has occurred";
      break;
    default:
      os << "unknown code (" << int(res) << ") :/";
  }

  return os;
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
        memory_(),
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
        objects_(),
        objects_instances_(),
        camera_(),
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
        application_version_(0),
        rtx_enabled_(false),
        rtx_(),
        rt_properties_{},
        rt_descriptor_pool_(),
        rt_descriptor_set_(),
        rt_descriptor_layout_(),
        rt_storage_image_(),
        rt_constants_(),
        rt_shader_groups_(),
        rt_pipeline_(),
        rt_pipeline_layout_(),
        rt_shader_binding_table_()
  //
  {
    std::cout << "Engine: Hello World." << std::endl;
  }

  bool init(const std::string &application_name, uint32_t application_version,
            int width, int height, const std::string &title, bool rtx_enabled) {
    application_name_ = application_name;
    application_version_ = application_version;
    rtx_enabled_ = rtx_enabled;

    if (!init_glfw(width, height, title)) {
      std::cerr << "init_glfw() failed" << std::endl;
      return false;
    }

    if (!init_global_layer_properties()) {
      std::cerr << "init_global_layer_properties() failed:" << std::endl;
      return false;
    }

    if (!init_instance_extension_names(rtx_enabled)) {
      std::cerr << "init_instance_extension_names() failed." << std::endl;
      return false;
    }

    if (!init_device_extension_names(rtx_enabled)) {
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

    if (!init_memory()) {
      std::cerr << "init_memory() failed." << std::endl;
      return false;
    }

    if (!init_device_queue()) {
      std::cerr << "init_device_queue() failed." << std::endl;
      return false;
    }

    if (rtx_enabled) {
      if (!init_ray_tracing()) {
        std::cerr << "init_ray_tracing() failed." << std::endl;
        return false;
      }
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

    if (!load_scene()) {
      std::cerr << "load_scene() failed." << std::endl;
      return false;
    }

    if (!create_texture_sampler()) {
      std::cerr << "create_texture_sampler() failed." << std::endl;
      return false;
    }

    bool rtx_on = false;
    if (!create_swap_chain(rtx_on)) {
      std::cerr << "create_swap_chain() failed." << std::endl;
      return false;
    }

    return true;
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
    surface_ = VK_NULL_HANDLE;

    fini_glfw();

    fini_instance();
  }

  bool draw() {
    bool show_demo_window = false;
    bool show_settings = true;
    bool show_status = true;
    bool rtx_on = false;
    bool prev_rtx_status = rtx_on;
    bool force_recreate_swap_chain = false;

    int ray_samples = rt_constants_.samples;
    int ray_max_iterations = rt_constants_.max_iterations;
    bool profile_temperature = rt_constants_.temperature;

    float light_position[3] = {7.0f, 5.0f, -8.0f};
    float light_intensity = 1.0f;
    enum light_mode { light_mode_point = 0, light_mode_directional = 1 };
    int light_type = 1;  // point = 0, directional = 1;

    while (!platform_.should_close_window()) {
      platform_.poll_events();

      handle_mouse_drag();
      handle_mouse_scroll();
      handle_wasd();

      if (camera_.is_updated()) {
        // std::cout << "Updating uniform buffer due to camera movement."
        //           << std::endl;
        update_uniform_buffer();

        reset_ray_tracing_frame_counter();
      }

      // Start the Dear ImGui frame.
      ImGui_ImplVulkan_NewFrame();
      ImGui_ImplGlfw_NewFrame();
      ImGui::NewFrame();

      if (show_demo_window) {
        ImGui::ShowDemoWindow(&show_demo_window);
      }

      if (show_settings) {
        const float DISTANCE = 10.0f;
        ImVec2 window_pos = ImVec2(DISTANCE, DISTANCE);
        ImVec2 window_pos_pivot = ImVec2(0.0f, 0.0f);
        ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_AlwaysAutoResize;

        ImGui::Begin("Settings", nullptr, window_flags);

        ImGui::Text("Ray Tracing");
        if (rtx_enabled_) {
          ImGui::Checkbox("RTX", &rtx_on);
          ImGui::SliderInt("Samples", &ray_samples, 1, 32);
          ImGui::SliderInt("Depth", &ray_max_iterations, 1, 32);

          // Light  options
          if (ImGui::CollapsingHeader("Light")) {
            ImGui::DragFloat3("Position", light_position, 0.1f, -40, 40);
            ImGui::SliderFloat("Intensity", &light_intensity, 0.0f, 1000.0f);
            // Light Type
            if (ImGui::RadioButton("Point", light_type == light_mode_point)) {
              light_type = light_mode_point;
            }
            ImGui::SameLine();
            if (ImGui::RadioButton("Directional",
                                   light_type == light_mode_directional)) {
              light_type = light_mode_directional;
            }
          }

          // Debug
          if (ImGui::CollapsingHeader("Debug")) {
            ImGui::Checkbox("Pixel temperature", &profile_temperature);
          }
        } else {
          ImGui::Text("RTX not supported");
        }

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
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoMouseInputs;

        ImGui::Begin("Stats", &show_status, window_flags);

        ImGui::Text("GPU: %s", gpu_properties_.deviceName);

        ImGui::Separator();

        ImGui::Text("Window size: %.0f x %.0f", io.DisplaySize.x,
                    io.DisplaySize.y);
        ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
        ImGui::Text("%.3f ms/frame", 1000.0f / ImGui::GetIO().Framerate);

        if (rtx_enabled_) {
          ImGui::Separator();
          ImGui::Text("Ray tracing");
          ImGui::Text("Accumulated frames: %d",
                      rtx_on ? rt_constants_.frame : 0);
        }

        ImGui::End();
      }

      // Check for changes on settings options.
      // TODO: Refactor.
      if (rtx_on != prev_rtx_status) {
        prev_rtx_status = rtx_on;
        force_recreate_swap_chain = true;
        std::cout << "RTX " << (rtx_on ? "ON" : "OFF") << "." << std::endl;
      }
      if (ray_samples != rt_constants_.samples) {
        rt_constants_.samples = ray_samples;
        reset_ray_tracing_frame_counter();
      }
      if (ray_max_iterations != rt_constants_.max_iterations) {
        rt_constants_.max_iterations = ray_max_iterations;
        reset_ray_tracing_frame_counter();
      }
      if (profile_temperature != rt_constants_.temperature) {
        rt_constants_.temperature = profile_temperature;
        reset_ray_tracing_frame_counter();
      }

      if (!render_frame(force_recreate_swap_chain, rtx_on)) {
        std::cerr << "Rendering frame failed." << std::endl;
        break;
      }
      if (rt_constants_.light_position !=
          glm::vec3(light_position[0], light_position[1], light_position[2])) {
        rt_constants_.light_position =
            glm::vec3(light_position[0], light_position[1], light_position[2]);
        reset_ray_tracing_frame_counter();
      }
      if (rt_constants_.light_intensity != light_intensity) {
        rt_constants_.light_intensity = light_intensity;
        reset_ray_tracing_frame_counter();
      }
      if (rt_constants_.light_type != light_type) {
        rt_constants_.light_type = light_type;
        reset_ray_tracing_frame_counter();
      }

      if (force_recreate_swap_chain) {
        force_recreate_swap_chain = false;
      }
    }
    std::cout << "Closing window." << std::endl;

    vkDeviceWaitIdle(device_);

    return true;
  }

 private:
  bool render_frame(bool force_recreate_swap_chain, bool rtx_on) {
    VkResult res;

    static constexpr VkBool32 wait_all = VK_TRUE;
    static constexpr uint64_t timeout = UINT64_MAX;
    res = vkWaitForFences(device_, 1, &in_flight_fences_[current_frame_],
                          wait_all, timeout);
    if (VK_SUCCESS != res) {
      std::cerr << "Failed to wait for in flight fence: " << res << std::endl;
      return false;
    }

    if (force_recreate_swap_chain) {
      if (!recreate_swap_chain(rtx_on)) {
        std::cerr << "Failed to force recreate swap chain before acquiring "
                     "next image."
                  << std::endl;
        return false;
      }
      return true;
    }

    if (platform_.is_window_resized()) {
      platform_.set_already_resized();
      if (!recreate_swap_chain(rtx_on)) {
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
    } else if (VK_SUCCESS != res && VK_SUBOPTIMAL_KHR != res) {
      std::cerr << "Failed to acquire next swap chain image. Current buffer = "
                << current_buffer_ << ": " << res << std::endl;
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

    if (rtx_on) {
      ray_trace(command_buffers_[current_buffer_]);

      copy_ray_tracing_output_to_swap_chain(command_buffers_[current_buffer_],
                                            buffers_[current_buffer_].image);

      //  ImGui::Render();
    }
    //} else {
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
    // render_pass_begin_info.clearValueCount = 0;
    // render_pass_begin_info.pClearValues = nullptr;

    vkCmdBeginRenderPass(command_buffers_[current_buffer_],
                         &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

    if (!rtx_on) {
      // Bind the graphic pipeline.
      //
      vkCmdBindPipeline(command_buffers_[current_buffer_],
                        VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_);
      static constexpr uint32_t first_set = 0;
      static constexpr uint32_t dynamic_offset_count = 0;
      static constexpr uint32_t *dynamic_offsets = nullptr;
      vkCmdBindDescriptorSets(
          command_buffers_[current_buffer_], VK_PIPELINE_BIND_POINT_GRAPHICS,
          pipeline_layout_, first_set, constants::NUM_DESCRIPTOR_SETS,
          descriptor_set_.data(), dynamic_offset_count, dynamic_offsets);

      // Bind the vertex buffer.
      //
      const VkDeviceSize offsets[1] = {objects_[0].vertex_offset};
      static constexpr uint32_t first_binding = 0;
      static constexpr uint32_t binding_count = 1;

      vkCmdBindVertexBuffers(command_buffers_[current_buffer_], first_binding,
                             binding_count, &objects_[0].vertex_buf, offsets);

      // Bind the index vertex buffer.
      //
      vkCmdBindIndexBuffer(command_buffers_[current_buffer_],
                           objects_[0].index_buf, objects_[0].index_offset,
                           VK_INDEX_TYPE_UINT32);

      // Set the viewport and the scissors rectangle.
      //
      init_viewports();
      init_scissors();

      // Draw the vertices.
      //

      uint32_t index_count = static_cast<uint32_t>(objects_[0].indices.size());
      uint32_t instance_count = 1;
      uint32_t first_vertex = 0;
      uint32_t vertex_offset = 0;
      uint32_t first_instance = 0;
      vkCmdDrawIndexed(command_buffers_[current_buffer_], index_count,
                       instance_count, first_vertex, vertex_offset,
                       first_instance);
    }

    // Record dear imgui primitives into command buffer.
    //
    ImGui::Render();
    ImDrawData *imgui_draw_data = ImGui::GetDrawData();
    ImGui_ImplVulkan_RenderDrawData(imgui_draw_data,
                                    command_buffers_[current_buffer_]);

    vkCmdEndRenderPass(
        command_buffers_[current_buffer_]);  // End of render pass.
    //}

    // Submit the command buffer.
    //
    res = vkEndCommandBuffer(command_buffers_[current_buffer_]);
    if (VK_SUCCESS != res) {
      std::cerr << "Failed to complete recording of command buffer: " << res
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
      std::cerr << "Failed to submit command buffer to graphics queue: " << res
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
    } else if (VK_SUCCESS != res) {
      std::cerr << "Failed to present image: " << res << std::endl;
      return false;
    }

    current_frame_ = (current_frame_ + 1) % constants::MAX_FRAMES_IN_FLIGHT;

    return true;
  }

  void handle_mouse_drag() {
    double drag_x;
    double drag_y;
    platform_.get_mouse_drag_movement(drag_x, drag_y);
    ImGuiIO &io = ImGui::GetIO();
    if (!io.WantCaptureMouse) {
      camera_.rotate_with_mouse_drag(drag_x, drag_y);
    }
  }

  void handle_mouse_scroll() {
    double scroll_y;
    platform_.get_mouse_scroll_input(scroll_y);
    ImGuiIO &io = ImGui::GetIO();
    if (!io.WantCaptureMouse) {
      camera_.zoom_with_mouse_wheel(scroll_y);
    }
  }

  void handle_wasd() {
    double key_x;
    double key_y;
    platform_.get_wasd(key_x, key_y);
    ImGuiIO &io = ImGui::GetIO();
    if (!io.WantCaptureKeyboard) {
      if (0.0 != key_x || 0.0 != key_y) {
        camera_.wasd(key_x, key_y);
      }
    }
  }

  bool init_glfw(int width, int height, const std::string &title) {
    platform_.fini();
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
        std::cerr << "VkEnumerateInstanceLayerProperties failed: " << res
                  << std::endl;
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

  bool init_instance_extension_names(bool rtx_enabled) {
    instance_extension_names_.clear();

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

    // Ray tracing
    //
    if (rtx_enabled) {
      instance_extension_names_.push_back(
          VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
    }

    // Since VulkanSDK 1.3.216, the Vulkan Loader is strictly enforcing the new
    // VK_KHR_PORTABILITY_subset extension. MoltenVK is currently not fully
    // conformant so the VK_KHR_portability_enumeration device extension is now
    // required on macOS.
    // See the "Encountered VK_ERROR_INCOMPATIBLE_DRIVER" section at
    // https://vulkan.lunarg.com/doc/sdk/1.3.216.0/mac/getting_started.html
    // and https://vulkan.lunarg.com/issue/view/62d328555df112a15deabfdd
    //
    // VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME is only required
    // for Vulkan version 1.0.
    // instance_extension_names_.push_back(
    //    VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
    instance_extension_names_.push_back(
        VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);

    std::cout << "Required instance extensions:" << std::endl;
    for (auto &e : instance_extension_names_) {
      std::cout << "- " << e << std::endl;
    }

    return true;
  }

  bool init_device_extension_names(bool rtx_enabled) {
    device_extension_names_.clear();

    device_extension_names_.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

    // Ray Tracing
    if (rtx_enabled) {
      device_extension_names_.push_back(
          VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME);
      device_extension_names_.push_back(VK_NV_RAY_TRACING_EXTENSION_NAME);
    }

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
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
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
      std::cerr << "Failed to setup debug messenger: " << res << std::endl;
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

    // Since VulkanSDK 1.3.216, the Vulkan Loader is strictly enforcing the new
    // VK_KHR_PORTABILITY_subset extension. MoltenVK is currently not fully
    // conformant so the VK_KHR_portability_enumeration device extension is now
    // required on macOS.
    // See the "Encountered VK_ERROR_INCOMPATIBLE_DRIVER" section at
    // https://vulkan.lunarg.com/doc/sdk/1.3.216.0/mac/getting_started.html
    // and https://vulkan.lunarg.com/issue/view/62d328555df112a15deabfdd
    instance_create_info.flags =
        VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;

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
      std::cerr << "Failed to create vulkan instance: " << res << std::endl;
      return false;
    }

    return true;
  }

  void fini_instance() {
    std::cout << "fini_instance." << std::endl;
    vkDestroyInstance(instance_, allocation_callbacks_);
    instance_ = VK_NULL_HANDLE;
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
      std::cerr << "Failed to enumerate GPUs. gpu_count=" << gpu_count << ": "
                << res << std::endl;
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
      std::cerr << "GPU 0 doesn't support all required features." << std::endl;
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
                  << layer_properties.properties.layerName << "." << std::endl;
        return false;
      }
    }

    return true;
  }

  bool init_memory() {
    if (VK_NULL_HANDLE == device_) {
      std::cerr << "Device handle is null." << std::endl;
      return false;
    }
    memory temp(device_, memory_properties_, allocation_callbacks_);
    memory_ = std::move(temp);

    return true;
  }

  bool init_ray_tracing() {
    rt_properties_.sType =
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PROPERTIES_NV;

    VkPhysicalDeviceProperties2 properties2{};
    properties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    properties2.pNext = &rt_properties_;

    vkGetPhysicalDeviceProperties2(gpus_[0], &properties2);

    print_ray_tracing_properties();

    if (!ray_tracing_extensions::load(device_)) {
      std::cerr << "Failed to load ray tracing functions." << std::endl;
      return false;
    }

    // TODO: Move to better place.
    rt_constants_.samples = 8;
    rt_constants_.max_iterations = 8;
    rt_constants_.temperature = false;

    return true;
  }

  void print_ray_tracing_properties() {
    std::cout << "Ray Tracing properties:" << std::endl;
    std::cout << " - Shader header size: "
              << rt_properties_.shaderGroupHandleSize << " bytes." << std::endl;
    std::cout << " - Max recursion depth: " << rt_properties_.maxRecursionDepth
              << std::endl;
    std::cout << " - Max stride between shader groups in the SBT: "
              << rt_properties_.maxShaderGroupStride << " bytes." << std::endl;
    std::cout << " - Alignment for the base of the SBTs: "
              << rt_properties_.shaderGroupBaseAlignment << " bytes."
              << std::endl;
    std::cout << " - Max geometries in the BLAS: "
              << rt_properties_.maxGeometryCount << std::endl;
    std::cout << " - Max instances in the BLAS: "
              << rt_properties_.maxInstanceCount << std::endl;
    std::cout << " - Max triangles in the BLAS: "
              << rt_properties_.maxTriangleCount << std::endl;
    std::cout << " - Max acceleration structures in a descriptor set: "
              << rt_properties_.maxDescriptorSetAccelerationStructures
              << std::endl;
  }

  bool init_device_extension_properties(layer_properties_t &layer_properties) {
    char *layer_name = layer_properties.properties.layerName;
    uint32_t device_extension_count;
    VkResult res;

    do {
      res = vkEnumerateDeviceExtensionProperties(
          gpus_[0], layer_name, &device_extension_count, nullptr);
      if (res) {
        std::cerr << "Enumeration of device extension properties for layer "
                  << layer_name << " failed: " << res << std::endl;
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

    std::cout << "Layer " << layer_name
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
    VkResult res = vkGetPhysicalDeviceSurfaceFormatsKHR(gpus_[0], surface_,
                                                        &format_count, nullptr);
    if (res != VK_SUCCESS) {
      std::cerr << "Failed to get count of surface formats: " << res
                << std::endl;
      return false;
    }

    VkSurfaceFormatKHR *surface_formats =
        (VkSurfaceFormatKHR *)malloc(format_count * sizeof(VkSurfaceFormatKHR));
    if (!surface_formats) {
      std::cerr << "Failed to reserve memory for surface formats." << std::endl;
      return false;
    }
    res = vkGetPhysicalDeviceSurfaceFormatsKHR(gpus_[0], surface_,
                                               &format_count, surface_formats);
    if (res != VK_SUCCESS) {
      std::cerr << "Failed to get surface formats: " << res << std::endl;
      free(surface_formats);
      return false;
    }

    const VkFormat desired_format = VK_FORMAT_B8G8R8A8_UNORM;

    format_ = VK_FORMAT_UNDEFINED;
    for (uint32_t i = 0; i < format_count; ++i) {
      if (desired_format == surface_formats[i].format) {
        format_ = desired_format;
        break;
      }
    }
    if (1 == format_count && VK_FORMAT_UNDEFINED == surface_formats[0].format) {
      format_ = desired_format;
    }
    if (desired_format != format_) {
      std::cerr << "Unsupported surface format VK_FORMAT_B8G8R8A8_UNORM."
                << std::endl;
      return false;
    }

    free(surface_formats);

    return true;
  }

  bool init_device() {
    VkDeviceQueueCreateInfo device_queue_create_info = {};
    float queue_priorities[1] = {0.0};
    device_queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
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
      std::cerr << "Failed to create device: " << res << std::endl;
      return false;
    }

    return true;
  }

  void fini_device() {
    std::cout << "fini_device." << std::endl;
    vkDeviceWaitIdle(device_);
    vkDestroyDevice(device_, allocation_callbacks_);
    device_ = VK_NULL_HANDLE;
  }

  bool init_command_pool() {
    VkCommandPoolCreateInfo command_pool_create_info = {};
    command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    command_pool_create_info.pNext = nullptr;
    command_pool_create_info.queueFamilyIndex = graphics_queue_family_index_;
    command_pool_create_info.flags =
        VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    VkResult res = vkCreateCommandPool(device_, &command_pool_create_info,
                                       allocation_callbacks_, &command_pool_);
    if (VK_SUCCESS != res) {
      std::cerr << "Failed to create command pool: " << res << std::endl;
      return false;
    }

    return true;
  }

  void fini_command_pool() {
    std::cout << "fini_command_pool." << std::endl;
    vkDestroyCommandPool(device_, command_pool_, allocation_callbacks_);
    command_pool_ = VK_NULL_HANDLE;
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
      std::cerr << "Failed to create command buffer: " << res << std::endl;
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
    image_acquire_semaphores_.resize(constants::MAX_FRAMES_IN_FLIGHT);
    render_finished_semaphores_.resize(constants::MAX_FRAMES_IN_FLIGHT);
    in_flight_fences_.resize(constants::MAX_FRAMES_IN_FLIGHT);
    images_in_flight_.resize(swap_chain_image_count_);

    VkSemaphoreCreateInfo semaphore_create_info = {};
    semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphore_create_info.pNext = nullptr;
    semaphore_create_info.flags = 0;

    VkFenceCreateInfo fence_create_info;
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_create_info.pNext = nullptr;
    fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (uint32_t i = 0; i < constants::MAX_FRAMES_IN_FLIGHT; ++i) {
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
    for (uint32_t i = 0; i < constants::MAX_FRAMES_IN_FLIGHT; ++i) {
      vkDestroySemaphore(device_, image_acquire_semaphores_[i],
                         allocation_callbacks_);
      vkDestroySemaphore(device_, render_finished_semaphores_[i],
                         allocation_callbacks_);
      vkDestroyFence(device_, in_flight_fences_[i], allocation_callbacks_);
    }
    image_acquire_semaphores_.clear();
    render_finished_semaphores_.clear();
    in_flight_fences_.clear();
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
    // io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Enable gamepad support.
    //
    // io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

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
    if (!begin_single_time_commands(command_buffer, device_, command_pool_)) {
      std::cerr << "imgui: begin of single time command failed." << std::endl;
      return false;
    }

    if (!ImGui_ImplVulkan_CreateFontsTexture(command_buffer)) {
      std::cerr << "Failed to create imgui fonts." << std::endl;
      return false;
    }

    if (!end_single_time_commands(command_buffer, device_, command_pool_,
                                  graphics_queue_)) {
      std::cerr << "imgui: end of single time command failed." << std::endl;
      return false;
    }

    ImGui_ImplVulkan_DestroyFontUploadObjects();

    return true;
  }

  void fini_imgui() {
    std::cout << "fini_imgui." << std::endl;
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
  }

  // TODO: Return command_buffers_[current_buffer_]?
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
      std::cerr << "Failed to begin command buffer: " << res << std::endl;
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

  bool create_swap_chain(bool rtx_on) {
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

    if (!init_render_pass(rtx_on)) {
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

    if (rtx_on) {
      if (!create_ray_tracing()) {
        std::cerr << "create_ray_tracing() failed." << std::endl;
        return false;
      }
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
    if (rtx_enabled_) {
      fini_ray_tracing();
    }
    fini_descriptor_pool();
    fini_framebuffers();
    fini_pipeline();
    fini_shaders();
    fini_render_pass();
    fini_descriptor_layout();
    fini_uniform_buffer();
    fini_depth_buffer();
    fini_swap_chain();
  }

  bool recreate_swap_chain(bool rtx_on) {
    std::cout << "Recreating swap chain." << std::endl;

    VkExtent2D window = platform_.window_size();
    while (0 == window.width || 0 == window.height) {
      window = platform_.window_size();
      platform_.wait_events();
    }

    VkResult res = vkDeviceWaitIdle(device_);
    if (VK_SUCCESS != res) {
      std::cerr << "recreate swap chain: Failed to wait for device: " << res
                << std::endl;
      return false;
    }

    cleanup_swap_chain();

    if (!create_swap_chain(rtx_on)) {
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
      std::cerr << "Failed to get surface capabilities: " << res << std::endl;
      return false;
    }

    uint32_t present_mode_count;
    res = vkGetPhysicalDeviceSurfacePresentModesKHR(
        gpus_[0], surface_, &present_mode_count, nullptr);
    if (VK_SUCCESS != res) {
      std::cerr << "Failed to get surface present modes count: " << res
                << std::endl;
      return false;
    }
    std::cout << present_mode_count << " present modes available." << std::endl;
    VkPresentModeKHR *present_modes = (VkPresentModeKHR *)malloc(
        present_mode_count * sizeof(VkPresentModeKHR));
    if (!present_modes) {
      std::cerr << "Failed to reserve memory fro present modes." << std ::endl;
      return false;
    }
    res = vkGetPhysicalDeviceSurfacePresentModesKHR(
        gpus_[0], surface_, &present_mode_count, present_modes);
    if (VK_SUCCESS != res) {
      std::cerr << "Failed to get surface present modes: " << res << std::endl;
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

      swap_chain_extent.height = std::max(
          swap_chain_extent.height, surface_capabilities.minImageExtent.height);
      swap_chain_extent.height = std::min(
          swap_chain_extent.height, surface_capabilities.maxImageExtent.height);
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
    swap_chain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
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
    swap_chain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                                        VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                                        VK_IMAGE_USAGE_TRANSFER_DST_BIT;

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
      std::cerr << "Failed to create swap chain: " << res << std::endl;
      return false;
    }

    // Retrieve the created swap chain images.
    //
    res = vkGetSwapchainImagesKHR(device_, swap_chain_,
                                  &swap_chain_image_count_, nullptr);
    if (VK_SUCCESS != res) {
      std::cerr << "Failed to get swap chain image count: " << res << std::endl;
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
    res = vkGetSwapchainImagesKHR(device_, swap_chain_,
                                  &swap_chain_image_count_, swap_chain_images);
    if (VK_SUCCESS != res) {
      std::cerr << "Failed to get swap chain images: " << res << std::endl;
      return false;
    }

    for (uint32_t i = 0; i < swap_chain_image_count_; ++i) {
      swap_chain_buffer_t swap_chain_buffer;

      swap_chain_buffer.image = swap_chain_images[i];
      if (!helpers::create_image_view(memory_, swap_chain_buffer.image, format_,
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

    // Swap chain images will be destroyed when destroying the swap chain.
    // Just clear the vector.
    buffers_.clear();

    vkDestroySwapchainKHR(device_, swap_chain_, allocation_callbacks_);
    swap_chain_ = VK_NULL_HANDLE;
  }

  bool find_depth_format(VkFormat &depth_format) {
    return helpers::find_supported_format(
        gpus_[0],
        {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT,
         VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D16_UNORM},
        VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT,
        depth_format);
  }

  bool has_stencil_component(VkFormat format) {
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
           format == VK_FORMAT_D24_UNORM_S8_UINT;
  }

  bool init_depth_buffer() {
    VkFormat depth_format{};
    if (!find_depth_format(depth_format)) {
      std::cerr << "Failed to find asupported depth format." << std::endl;
      return false;
    }

    VkExtent2D window_size = platform_.window_size();

    if (!helpers::create_image(memory_, window_size.width, window_size.height,
                               depth_format, VK_IMAGE_TILING_OPTIMAL,
                               VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                               depth_buffer_.image, depth_buffer_.mem)) {
      std::cerr << "Failed to create depth buffer image." << std::endl;
      return false;
    }

    if (!helpers::create_image_view(memory_, depth_buffer_.image, depth_format,
                                    VK_IMAGE_ASPECT_DEPTH_BIT,
                                    depth_buffer_.view)) {
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
    depth_buffer_.view = VK_NULL_HANDLE;

    vkDestroyImage(device_, depth_buffer_.image, allocation_callbacks_);
    depth_buffer_.image = VK_NULL_HANDLE;

    vkFreeMemory(device_, depth_buffer_.mem, allocation_callbacks_);
    depth_buffer_.mem = VK_NULL_HANDLE;
  }

  bool init_model_view_projection() {
    VkExtent2D window = platform_.window_size();
    camera_.update_window_size(window.width, window.height);

    return true;
  }

  bool init_uniform_buffer() {
    VkBufferCreateInfo buffer_create_info = {};
    buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_create_info.pNext = nullptr;
    buffer_create_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    buffer_create_info.size = sizeof(uniform_data_.data);
    buffer_create_info.queueFamilyIndexCount = 0;
    buffer_create_info.pQueueFamilyIndices = nullptr;
    buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    buffer_create_info.flags = 0;

    VkResult res = vkCreateBuffer(device_, &buffer_create_info,
                                  allocation_callbacks_, &uniform_data_.buf);
    if (VK_SUCCESS != res) {
      std::cerr << "Failed to create uniform buffer: " << res << std::endl;
      return false;
    }

    VkMemoryRequirements memory_requirements;
    vkGetBufferMemoryRequirements(device_, uniform_data_.buf,
                                  &memory_requirements);

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
    //
    VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                       VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    if (!memory_.allocate_memory(memory_requirements, properties,
                                 uniform_data_.mem)) {
      std::cerr << "Failed to allocate memory for uniform buffer." << std::endl;
      return false;
    }

    VkDeviceSize memory_offset = 0;
    res = vkBindBufferMemory(device_, uniform_data_.buf, uniform_data_.mem,
                             memory_offset);
    if (VK_SUCCESS != res) {
      std::cerr << "Failed to bind uniform buffer memory: " << res << std::endl;
      return false;
    }

    uniform_data_.buffer_info.buffer = uniform_data_.buf;
    uniform_data_.buffer_info.offset = 0;
    uniform_data_.buffer_info.range = sizeof(uniform_data_.data);

    uniform_data_.size = memory_requirements.size;
    if (!update_uniform_buffer()) {
      std::cerr << "Failed to update uniform buffer." << std::endl;
      return false;
    }

    return true;
  }

  bool update_uniform_buffer() {
    // Get new values.
    //
    uniform_data_.data.mvp = camera_.mvp();
    uniform_data_.data.inverse_view = camera_.inverse_view();
    uniform_data_.data.inverse_projection = camera_.inverse_projection();

    if (!memory_.copy_to_buffer(uniform_data_.mem, uniform_data_.size,
                                &uniform_data_.data)) {
      std::cerr << "Failed to map uniform buffer to CPU memory." << std::endl;
      return false;
    }

    return true;
  }

  void fini_uniform_buffer() {
    std::cout << "fini_uniform_buffer." << std::endl;
    vkDestroyBuffer(device_, uniform_data_.buf, allocation_callbacks_);
    uniform_data_.buf = VK_NULL_HANDLE;

    vkFreeMemory(device_, uniform_data_.mem, allocation_callbacks_);
    uniform_data_.mem = VK_NULL_HANDLE;
  }

  bool init_descriptor_layout() {
    VkDescriptorSetLayoutBinding layout_bindings[2];

    // Vertex shader.
    layout_bindings[0].binding = 0;
    layout_bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    layout_bindings[0].descriptorCount = 1;
    layout_bindings[0].stageFlags =
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_RAYGEN_BIT_NV;
    layout_bindings[0].pImmutableSamplers = nullptr;

    // Fragment shader.
    layout_bindings[1].binding = 1;
    layout_bindings[1].descriptorType =
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    layout_bindings[1].descriptorCount = 1;
    layout_bindings[1].stageFlags =
        VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV;
    layout_bindings[1].pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo descriptor_layout = {};
    descriptor_layout.sType =
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptor_layout.pNext = nullptr;
    descriptor_layout.flags = 0;
    descriptor_layout.bindingCount = 2;
    descriptor_layout.pBindings = layout_bindings;

    descriptor_layout_.resize(constants::NUM_DESCRIPTOR_SETS);

    VkResult res = vkCreateDescriptorSetLayout(device_, &descriptor_layout,
                                               allocation_callbacks_,
                                               descriptor_layout_.data());
    if (VK_SUCCESS != res) {
      std::cerr << "Failed to create descriptor set layout: " << res
                << std::endl;
      return false;
    }

    VkPipelineLayoutCreateInfo pipeline_layout_create_info = {};
    pipeline_layout_create_info.sType =
        VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_create_info.pNext = nullptr;
    pipeline_layout_create_info.pushConstantRangeCount = 0;
    pipeline_layout_create_info.pPushConstantRanges = nullptr;
    pipeline_layout_create_info.setLayoutCount = constants::NUM_DESCRIPTOR_SETS;
    pipeline_layout_create_info.pSetLayouts = descriptor_layout_.data();

    res = vkCreatePipelineLayout(device_, &pipeline_layout_create_info,
                                 allocation_callbacks_, &pipeline_layout_);
    if (VK_SUCCESS != res) {
      std::cerr << "Failed to create pipeline layout: " << res << std::endl;
      return false;
    }

    return true;
  }

  void fini_descriptor_layout() {
    std::cout << "fini_descriptor_layout." << std::endl;
    for (int i = 0; i < constants::NUM_DESCRIPTOR_SETS; i++) {
      vkDestroyDescriptorSetLayout(device_, descriptor_layout_[i],
                                   allocation_callbacks_);
    }
    descriptor_layout_.clear();

    vkDestroyPipelineLayout(device_, pipeline_layout_, allocation_callbacks_);
    pipeline_layout_ = VK_NULL_HANDLE;
  }

  bool init_render_pass(bool rtx_on) {
    VkAttachmentDescription attachments[2];
    attachments[0].format = format_;
    attachments[0].samples = constants::NUM_SAMPLES;
    if (rtx_on) {
      attachments[0].initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
      attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    } else {
      attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    }
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    attachments[0].flags = 0;

    // Depth buffer attachment.
    //
    attachments[1].format = depth_buffer_.format;
    attachments[1].samples = constants::NUM_SAMPLES;
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
      std::cerr << "Failed to create render pass: " << res << std::endl;
      return false;
    }

    return true;
  }

  void fini_render_pass() {
    std::cout << "fini_render_pass." << std::endl;
    vkDestroyRenderPass(device_, render_pass_, allocation_callbacks_);
    render_pass_ = VK_NULL_HANDLE;
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

    VkResult res = vkCreateShaderModule(device_, &draw_cube_vert_create_info,
                                        allocation_callbacks_,
                                        &shader_stages_create_info_[0].module);
    if (VK_SUCCESS != res) {
      std::cerr << "Failed to create cube vertex shader module: " << res
                << std::endl;
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
      std::cerr << "Failed to create cube fragment shader module: " << res
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
      shader_stages_create_info_[i].module = VK_NULL_HANDLE;
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
        std::cerr << "Failed to create framebuffer " << i << ": " << res
                  << std::endl;
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

  bool init_vertex_buffer(object_model_t &object) {
    // Map GPU memory and copy vertex data.
    //

    const void *vertex_data = object.vertices.data();
    uint32_t vertex_data_size = sizeof(Vertex) * object.vertices.size();

    VkBufferUsageFlags usage =
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;  // Ray tracing needs to use  the
                                             // buffer as storage.

    VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                       VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    if (!memory_.create_buffer_and_copy(vertex_data_size, usage, properties,
                                        object.vertex_buf, object.vertex_mem,
                                        vertex_data)) {
      std::cerr << "Failed to create vertex buffer and copy vertex data."
                << std::endl;
      return false;
    }

    return true;
  }

  bool init_vertex_index_buffer(object_model_t &object) {
    VkDeviceSize buffer_size =
        sizeof(object.indices[0]) * object.indices.size();

    VkBuffer staging_buffer;
    VkDeviceMemory staging_buffer_memory;

    VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                       VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    if (!memory_.create_buffer_and_copy(buffer_size, usage, properties,
                                        staging_buffer, staging_buffer_memory,
                                        object.indices.data())) {
      std::cerr << "Failed to create vertex index staging buffer." << std::endl;
      return false;
    }

    usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT |
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;  // Ray tracing needs to use
                                                 // the buffer as storage.

    properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    if (!memory_.create_buffer(buffer_size, usage, properties, object.index_buf,
                               object.index_mem)) {
      std::cerr << "Failed to create vertex index buffer." << std::endl;
      return false;
    }

    // Set offset of index data.
    object.index_offset = 0;

    if (!copy_buffer(staging_buffer, object.index_buf, buffer_size)) {
      std::cerr << "Failed to copy vertex index staging buffer to vertex "
                   "index buffer."
                << std::endl;
      return false;
    }

    vkDestroyBuffer(device_, staging_buffer, allocation_callbacks_);
    vkFreeMemory(device_, staging_buffer_memory, allocation_callbacks_);

    return true;
  }

  void fini_vertex_buffer() {
    std::cout << "fini_vertex_buffer." << std::endl;

    for (auto &object : objects_) {
      vkDestroyBuffer(device_, object.index_buf, allocation_callbacks_);
      vkFreeMemory(device_, object.index_mem, allocation_callbacks_);

      vkDestroyBuffer(device_, object.vertex_buf, allocation_callbacks_);
      vkFreeMemory(device_, object.vertex_mem, allocation_callbacks_);
    }
    objects_.clear();
  }

  bool load_scene() {
    // Example of translation + scaling + rotation.
    //
    // glm::mat4 transform = glm::mat4(1.0f);
    // glm::vec3 translation = glm::vec3(-5.0, 0, 0);
    // transform = glm::translate(transform, translation);
    // float scale_factor = 1.0f;
    // transform = glm::scale(transform, glm::vec3(scale_factor));
    // float rotation_degrees = glm::radians(0.0f);
    // glm::vec3 rotation_axis = glm::vec3(0, 1, 0);
    // transform = glm::rotate(transform, rotation_degrees, rotation_axis);

    // Viking room
    //
    glm::mat4 viking_room_transform = glm::mat4(1.0f);
    std::string viking_room_model_path("assets/models/viking_room.obj");
    std::string viking_room_texture_path("assets/textures/viking_room.png");
    if (!load_model(viking_room_model_path, viking_room_transform)) {
      return false;
    }
    if (!load_texture(viking_room_texture_path)) {
      return false;
    }

    // Venus
    //
    // glm::vec3 venus_translation = glm::vec3(0, 0.1, -1.0);
    // glm::mat4 venus_transform =
    //    glm::translate(glm::mat4(1.0f), venus_translation);
    // std::string venus_model_path("assets/models/venus.obj");
    // if (!load_model(venus_model_path, venus_transform)) {
    //  return false;
    //}

    // Lucy
    //
    // glm::mat4 lucy_translation = glm::vec3(5.0, 0, 0);
    // glm::mat4 lucy_transform = glm::translate(glm::mat4(1.0f),
    // lucy_translation); std::string
    // lucy_model_path("assets/models/lucy.obj"); if
    // (!load_model(venus_model_path, venus_transform)) {
    //  return false;
    //}

    // Rungholt city
    //
    // glm::mat4 rungholt_transform = glm::mat4(1.0f);
    // std::string rungholt_model_path("assets/models/rungholt.obj");
    // std::string rungholt_texture_path("assets/textures/rungholt-RGBA.png");
    // if (!load_model(rungholt_model_path, rungholt_transform)) {
    //   return false;
    // }
    // if (!load_texture(rungholt_texture_path)) {
    //   return false;
    // }

    return true;
  }

  bool load_model(const std::string &model_path) {
    std::vector<glm::mat4> instances_transformation;
    instances_transformation.emplace_back(1.0f);
    return load_model(model_path, instances_transformation);
  }

  bool load_model(const std::string &model_path,
                  const glm::mat4 &transformation) {
    std::vector<glm::mat4> instances_transformation;
    instances_transformation.push_back(transformation);
    return load_model(model_path, instances_transformation);
  }

  bool load_model(const std::string &model_path,
                  const std::vector<glm::mat4> &instances_transformation) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    std::cout << "Loading model " << model_path << "... " << std::flush;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err,
                          model_path.c_str())) {
      std::cerr << "Failed to load model: " << warn << ", " << err << "."
                << std::endl;
      return false;
    }

    objects_.emplace_back();

    uint32_t index = objects_.size() - 1;
    for (auto &transformation : instances_transformation) {
      // Test
      objects_instances_.emplace_back();
      objects_instances_.back().index = index;
      objects_instances_.back().transform = transformation;
      objects_.back().transforms.push_back(transformation);
    }

    std::unordered_map<Vertex, uint32_t> unique_vertices;

    for (const auto &shape : shapes) {
      for (const auto &index : shape.mesh.indices) {
        Vertex vertex{};

        vertex.pos = {attrib.vertices[3 * index.vertex_index + 0],
                      attrib.vertices[3 * index.vertex_index + 1],
                      attrib.vertices[3 * index.vertex_index + 2]};

        if (!attrib.normals.empty()) {
          vertex.normal = {attrib.normals[3 * index.normal_index + 0],
                           attrib.normals[3 * index.normal_index + 1],
                           attrib.normals[3 * index.normal_index + 2]};
        }

        if (!attrib.texcoords.empty()) {
          vertex.tex_coord = {
              attrib.texcoords[2 * index.texcoord_index + 0],
              1.0f - attrib.texcoords[2 * index.texcoord_index + 1]};
        }

        if (unique_vertices.count(vertex) == 0) {
          unique_vertices[vertex] =
              static_cast<uint32_t>(objects_.back().vertices.size());
          objects_.back().vertices.push_back(vertex);
        }
        objects_.back().indices.push_back(unique_vertices[vertex]);
      }
    }

    if (!init_vertex_buffer(objects_.back())) {
      std::cerr << "init_vertex_buffer() failed." << std::endl;
      return false;
    }

    if (!init_vertex_index_buffer(objects_.back())) {
      std::cerr << "init_vertex_index_buffer() failed." << std::endl;
      return false;
    }

    std::cout << " Loaded " << objects_.back().vertices.size()
              << " vertices and " << objects_.back().indices.size()
              << " indices." << std::endl;

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
      std::cerr << "Failed to create descriptor pool: " << res << std::endl;
      return false;
    }

    return true;
  }

  void fini_descriptor_pool() {
    std::cout << "fini_descriptor_pool." << std::endl;
    vkDestroyDescriptorPool(device_, descriptor_pool_, allocation_callbacks_);
    descriptor_pool_ = VK_NULL_HANDLE;
  }

  bool init_descriptor_set() {
    VkDescriptorSetAllocateInfo descriptor_set_allocate_info = {};
    descriptor_set_allocate_info.sType =
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptor_set_allocate_info.pNext = nullptr;
    descriptor_set_allocate_info.descriptorPool = descriptor_pool_;
    descriptor_set_allocate_info.descriptorSetCount =
        constants::NUM_DESCRIPTOR_SETS;
    descriptor_set_allocate_info.pSetLayouts = descriptor_layout_.data();

    descriptor_set_.resize(constants::NUM_DESCRIPTOR_SETS);
    VkResult res = vkAllocateDescriptorSets(
        device_, &descriptor_set_allocate_info, descriptor_set_.data());
    if (VK_SUCCESS != res) {
      std::cerr << "Failed to allocate descriptor set: " << res << std::endl;
      return false;
    }

    VkWriteDescriptorSet write_descriptor_set[2];

    // Binding 0: Uniform buffer
    //
    write_descriptor_set[0] = {};
    write_descriptor_set[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_descriptor_set[0].pNext = nullptr;
    write_descriptor_set[0].dstSet = descriptor_set_[0];
    write_descriptor_set[0].descriptorCount = 1;
    write_descriptor_set[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write_descriptor_set[0].pBufferInfo = &uniform_data_.buffer_info;
    write_descriptor_set[0].pImageInfo = nullptr;
    write_descriptor_set[0].dstArrayElement = 0;
    write_descriptor_set[0].dstBinding = 0;

    // Binding 1: Texture
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
      std::cerr << "Failed to create pipeline cache: " << res << std::endl;
      return false;
    }

    return true;
  }

  void fini_pipeline_cache() {
    std::cout << "fini_pipeline_cache." << std::endl;
    vkDestroyPipelineCache(device_, pipeline_cache_, allocation_callbacks_);
    pipeline_cache_ = VK_NULL_HANDLE;
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

    VkVertexInputBindingDescription vertex_input_binding =
        Vertex::get_binding_description();
    vi.vertexBindingDescriptionCount = 1;
    vi.pVertexBindingDescriptions = &vertex_input_binding;

    const std::vector<VkVertexInputAttributeDescription>
        vertex_input_attributes = Vertex::get_attribute_descriptions();
    vi.vertexAttributeDescriptionCount =
        static_cast<uint32_t>(vertex_input_attributes.size());
    vi.pVertexAttributeDescriptions = vertex_input_attributes.data();

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
    vp.viewportCount = constants::NUM_VIEWPORTS_AND_SCISSORS;
    dynamic_states[pipeline_dynamic_state_create_info.dynamicStateCount++] =
        VK_DYNAMIC_STATE_VIEWPORT;
    vp.scissorCount = constants::NUM_VIEWPORTS_AND_SCISSORS;
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
    ms.rasterizationSamples = constants::NUM_SAMPLES;
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
    VkResult res =
        vkCreateGraphicsPipelines(device_, pipeline_cache_, create_info_count,
                                  &pipeline, allocation_callbacks_, &pipeline_);
    if (VK_SUCCESS != res) {
      std::cerr << "Failed to create pipeline: " << res << std::endl;
      return false;
    }

    return true;
  }

  void fini_pipeline() {
    std::cout << "fini_pipeline." << std::endl;
    std::cout << "Bye pipeline." << std::endl;
    vkDestroyPipeline(device_, pipeline_, allocation_callbacks_);
    pipeline_ = VK_NULL_HANDLE;
  }

  bool create_ray_tracing() {
    // TODO: BLAS, TLAS and more don't need to be rebuilt on window resize.
    // Generate ray tracing structures.
    bool update = false;
    if (!rtx_.build_acceleration_structures(
            memory_, command_pool_, graphics_queue_, objects_, update)) {
      std::cerr << "Failed to generate ray tracing structures." << std::endl;
      return false;
    }

    if (!rt_descriptor_pool_.init(memory_)) {
      std::cerr << "Failed to create ray tracing descriptor pool." << std::endl;
      return false;
    }

    if (!init_ray_tracing_descriptor_layout()) {
      std::cerr << "Failed to create ray tracing descriptor layout."
                << std::endl;
      return false;
    }

    if (!init_ray_tracing_storage_image()) {
      std::cerr << "Failed to create ray tracing storage image." << std::endl;
      return false;
    }

    if (!init_ray_tracing_descriptor_set()) {
      std::cerr << "Failed to create ray tracing descriptor set." << std::endl;
      return false;
    }

    if (!init_ray_tracing_pipeline()) {
      std::cerr << "Failed to create ray tracing pipeline." << std::endl;
      return false;
    }

    if (!init_ray_tracing_shader_binding_table()) {
      std::cerr << "Failed to create ray tracing shader binding table."
                << std::endl;
      return false;
    }

    reset_ray_tracing_frame_counter();

    return true;
  }

  bool init_ray_tracing_descriptor_layout() {
    // TLAS descriptor layout.
    //
    // Usable by camera rays (raygen) and bouncing rays on closest-hit.
    VkDescriptorSetLayoutBinding acceleration_structure_layout_binding{};
    acceleration_structure_layout_binding.binding = 0;
    acceleration_structure_layout_binding.descriptorType =
        VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV;
    acceleration_structure_layout_binding.descriptorCount = 1;
    acceleration_structure_layout_binding.stageFlags =
        VK_SHADER_STAGE_RAYGEN_BIT_NV | VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV;

    // Storage image descriptor layout.
    //
    VkDescriptorSetLayoutBinding output_image_layout_binding{};
    output_image_layout_binding.binding = 1;
    output_image_layout_binding.descriptorType =
        VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    output_image_layout_binding.descriptorCount = 1;
    output_image_layout_binding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_NV;

    // Vertices descriptor layout.
    //
    VkDescriptorSetLayoutBinding vertices_layout_binding{};
    vertices_layout_binding.binding = 2;
    vertices_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    vertices_layout_binding.descriptorCount = 1;
    vertices_layout_binding.stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV;

    // Indices descriptor layout.
    //
    VkDescriptorSetLayoutBinding indices_layout_binding{};
    indices_layout_binding.binding = 3;
    indices_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    indices_layout_binding.descriptorCount = 1;
    indices_layout_binding.stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV;

    std::vector<VkDescriptorSetLayoutBinding> layout_bindings(
        {acceleration_structure_layout_binding, output_image_layout_binding,
         vertices_layout_binding, indices_layout_binding});

    VkDescriptorSetLayoutCreateInfo descriptor_layout_create_info{};
    descriptor_layout_create_info.sType =
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptor_layout_create_info.bindingCount =
        static_cast<uint32_t>(layout_bindings.size());
    descriptor_layout_create_info.pBindings = layout_bindings.data();

    VkResult res = vkCreateDescriptorSetLayout(
        device_, &descriptor_layout_create_info, allocation_callbacks_,
        &rt_descriptor_layout_);
    if (VK_SUCCESS != res) {
      std::cerr << "Failed to create ray tracing descriptor set layout: " << res
                << std::endl;
      return false;
    }

    return true;
  }

  void fini_ray_tracing_descriptor_layout() {
    vkDestroyDescriptorSetLayout(device_, rt_descriptor_layout_,
                                 allocation_callbacks_);
    rt_descriptor_layout_ = VK_NULL_HANDLE;
  }

  bool find_ray_tracing_storage_image_format(VkFormat &format) {
    // TODO: Get proper format. Imgui seems to like this format better. Maybe
    // convert textures from VK_FORMAT_R8G8B8A8_SRGB to this format.
    return helpers::find_supported_format(
        gpus_[0],
        {VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM,
         VK_FORMAT_R8G8B8A8_SRGB, VK_FORMAT_B8G8R8A8_UNORM},
        VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT, format);
  }

  bool init_ray_tracing_storage_image() {
    // Create the storage image to where the ray tracing shaders will write.
    //

    VkExtent2D window_size = platform_.window_size();

    VkFormat color_format = VK_FORMAT_B8G8R8A8_UNORM;
    if (!find_ray_tracing_storage_image_format(color_format)) {
      std::cerr << "Failed to find a storage image format." << std::endl;
      return false;
    }
    rt_storage_image_.format = color_format;  // TODO: refactor.

    if (!helpers::create_image(
            memory_, window_size.width, window_size.height, color_format,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                VK_IMAGE_USAGE_STORAGE_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, rt_storage_image_.image,
            rt_storage_image_.mem)) {
      std::cerr << "Failed to create ray tracing storage image." << std::endl;
      return false;
    }

    if (!helpers::create_image_view(memory_, rt_storage_image_.image,
                                    color_format, VK_IMAGE_ASPECT_COLOR_BIT,
                                    rt_storage_image_.view)) {
      std::cerr << "Failed to create ray tracing storage image view."
                << std::endl;
      return false;
    }

    if (!transition_image_layout(rt_storage_image_.image, color_format,
                                 VK_IMAGE_LAYOUT_UNDEFINED,
                                 VK_IMAGE_LAYOUT_GENERAL)) {
      std::cerr << "Failed to transit ray tracing storage image." << std::endl;
      return false;
    }

    return true;
  }

  void fini_ray_tracing_storage_image() {
    vkDestroyImageView(device_, rt_storage_image_.view, allocation_callbacks_);
    rt_storage_image_.view = VK_NULL_HANDLE;

    vkDestroyImage(device_, rt_storage_image_.image, allocation_callbacks_);
    rt_storage_image_.image = VK_NULL_HANDLE;

    vkFreeMemory(device_, rt_storage_image_.mem, allocation_callbacks_);
    rt_storage_image_.mem = VK_NULL_HANDLE;
  }

  bool init_ray_tracing_descriptor_set() {
    VkDescriptorSetAllocateInfo descriptor_set_allocate_info{};
    descriptor_set_allocate_info.sType =
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptor_set_allocate_info.descriptorPool = rt_descriptor_pool_.pool();
    descriptor_set_allocate_info.descriptorSetCount = 1;
    descriptor_set_allocate_info.pSetLayouts = &rt_descriptor_layout_;

    VkResult res = vkAllocateDescriptorSets(
        device_, &descriptor_set_allocate_info, &rt_descriptor_set_);
    if (VK_SUCCESS != res) {
      std::cerr << "Failed to allocate ray tracing descriptor set: " << res
                << std::endl;
      return false;
    }

    // TLAS descriptor.
    //
    VkWriteDescriptorSetAccelerationStructureNV write_descriptor_set_as_info{};
    write_descriptor_set_as_info.sType =
        VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_NV;
    write_descriptor_set_as_info.accelerationStructureCount = 1;
    write_descriptor_set_as_info.pAccelerationStructures = &rtx_.get_tlas();

    VkWriteDescriptorSet write_descriptor_set_as{};
    write_descriptor_set_as.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    // The specialized acceleration structure descriptor has to be chained.
    write_descriptor_set_as.pNext = &write_descriptor_set_as_info;
    write_descriptor_set_as.dstSet = rt_descriptor_set_;
    write_descriptor_set_as.dstBinding = 0;
    write_descriptor_set_as.descriptorCount = 1;
    write_descriptor_set_as.descriptorType =
        VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV;

    //  Storage image descriptor.
    //
    VkDescriptorImageInfo output_image_descriptor_info{};
    output_image_descriptor_info.imageView = rt_storage_image_.view;
    output_image_descriptor_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

    VkWriteDescriptorSet write_descriptor_set_storage_image{};
    write_descriptor_set_storage_image.sType =
        VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_descriptor_set_storage_image.dstSet = rt_descriptor_set_;
    write_descriptor_set_storage_image.dstBinding = 1;
    write_descriptor_set_storage_image.descriptorCount = 1;
    write_descriptor_set_storage_image.descriptorType =
        VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    write_descriptor_set_storage_image.pImageInfo =
        &output_image_descriptor_info;

    // Vertices descriptor.
    //
    // TODO: Add more objects.
    VkDescriptorBufferInfo vertices_descriptor_info{};
    vertices_descriptor_info.buffer = objects_[0].vertex_buf;
    vertices_descriptor_info.range = VK_WHOLE_SIZE;

    VkWriteDescriptorSet write_descriptor_set_vertices{};
    write_descriptor_set_vertices.sType =
        VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_descriptor_set_vertices.dstSet = rt_descriptor_set_;
    write_descriptor_set_vertices.dstBinding = 2;
    write_descriptor_set_vertices.descriptorCount = 1;
    write_descriptor_set_vertices.descriptorType =
        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    write_descriptor_set_vertices.pBufferInfo = &vertices_descriptor_info;

    // Indices descriptor.
    //
    VkDescriptorBufferInfo indices_descriptor_info{};
    indices_descriptor_info.buffer = objects_[0].index_buf;
    indices_descriptor_info.range = VK_WHOLE_SIZE;

    VkWriteDescriptorSet write_descriptor_set_indices{};
    write_descriptor_set_indices.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_descriptor_set_indices.dstSet = rt_descriptor_set_;
    write_descriptor_set_indices.dstBinding = 3;
    write_descriptor_set_indices.descriptorCount = 1;
    write_descriptor_set_indices.descriptorType =
        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    write_descriptor_set_indices.pBufferInfo = &indices_descriptor_info;

    std::vector<VkWriteDescriptorSet> write_descriptor_sets(
        {write_descriptor_set_as, write_descriptor_set_storage_image,
         write_descriptor_set_vertices, write_descriptor_set_indices});

    uint32_t descriptor_copy_count = 0;
    const VkCopyDescriptorSet *descriptor_copies = nullptr;
    vkUpdateDescriptorSets(
        device_, static_cast<uint32_t>(write_descriptor_sets.size()),
        write_descriptor_sets.data(), descriptor_copy_count, descriptor_copies);

    return true;
  }

  bool load_shader(
      const uint32_t *code, size_t code_size,
      VkPipelineShaderStageCreateInfo &pipeline_shader_stage_create_info,
      VkShaderStageFlagBits stage) {
    VkShaderModuleCreateInfo shader_module_create_info{};
    shader_module_create_info.sType =
        VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shader_module_create_info.pNext = nullptr;
    shader_module_create_info.flags = 0;
    shader_module_create_info.codeSize = code_size;
    shader_module_create_info.pCode = code;

    pipeline_shader_stage_create_info.sType =
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipeline_shader_stage_create_info.pNext = nullptr;
    pipeline_shader_stage_create_info.pSpecializationInfo = nullptr;
    pipeline_shader_stage_create_info.flags = 0;
    pipeline_shader_stage_create_info.stage = stage;
    pipeline_shader_stage_create_info.pName = "main";

    VkResult res = vkCreateShaderModule(
        device_, &shader_module_create_info, allocation_callbacks_,
        &pipeline_shader_stage_create_info.module);
    if (VK_SUCCESS != res) {
      std::cerr << "Failed to create shader module: " << res << std::endl;
      return false;
    }

    return true;
  }

  bool init_ray_tracing_pipeline_layout() {
    VkPipelineLayoutCreateInfo pipeline_layout_create_info{};
    pipeline_layout_create_info.sType =
        VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

    // Ray tracing descriptor layout + shared-with-rasterization descriptor
    // layout.
    std::vector<VkDescriptorSetLayout> layouts(
        {rt_descriptor_layout_, descriptor_layout_[0]});
    pipeline_layout_create_info.setLayoutCount =
        static_cast<uint32_t>(layouts.size());
    pipeline_layout_create_info.pSetLayouts = layouts.data();

    VkPushConstantRange push_constant{};
    push_constant.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_NV |
                               VK_SHADER_STAGE_MISS_BIT_NV |
                               VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV;
    push_constant.offset = 0;
    push_constant.size = sizeof(rt_constants_);

    pipeline_layout_create_info.pushConstantRangeCount = 1;
    pipeline_layout_create_info.pPushConstantRanges = &push_constant;

    VkResult res =
        vkCreatePipelineLayout(device_, &pipeline_layout_create_info,
                               allocation_callbacks_, &rt_pipeline_layout_);
    if (VK_SUCCESS != res) {
      std::cerr << "Failed to create ray tracing pipeline layout: " << res
                << std::endl;
      return false;
    }

    return true;
  }

  void fini_ray_tracing_pipeline_layout() {
    vkDestroyPipelineLayout(device_, rt_pipeline_layout_,
                            allocation_callbacks_);
    rt_pipeline_layout_ = VK_NULL_HANDLE;
  }

  bool init_ray_tracing_shaders(
      std::vector<VkRayTracingShaderGroupCreateInfoNV> &groups) {
    // Load shaders.
    //

    rt_shader_groups_.emplace_back();
    if (!load_shader(raytrace_rgen, sizeof(raytrace_rgen),
                     rt_shader_groups_.back(), VK_SHADER_STAGE_RAYGEN_BIT_NV)) {
      std::cerr << "Failed to load ray tracing raygen shader." << std::endl;
      return false;
    }

    rt_shader_groups_.emplace_back();
    if (!load_shader(raytrace_rmiss, sizeof(raytrace_rmiss),
                     rt_shader_groups_.back(), VK_SHADER_STAGE_MISS_BIT_NV)) {
      std::cerr << "Failed to load ray tracing miss shader." << std::endl;
      return false;
    }

    rt_shader_groups_.emplace_back();
    if (!load_shader(raytrace_shadow_rmiss, sizeof(raytrace_shadow_rmiss),
                     rt_shader_groups_.back(), VK_SHADER_STAGE_MISS_BIT_NV)) {
      std::cerr << "Failed to load ray tracing shadow miss shader."
                << std::endl;
      return false;
    }

    rt_shader_groups_.emplace_back();
    if (!load_shader(raytrace_rchit, sizeof(raytrace_rchit),
                     rt_shader_groups_.back(),
                     VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV)) {
      std::cerr << "Failed to load ray tracing closes hit shader." << std::endl;
      return false;
    }

    // Create ray tracing shader groups.
    //

    for (uint32_t i = 0; i < rt_shader_groups_.size(); ++i) {
      VkRayTracingShaderGroupCreateInfoNV group{};
      group.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_NV;
      group.generalShader = VK_SHADER_UNUSED_NV;
      group.closestHitShader = VK_SHADER_UNUSED_NV;
      group.anyHitShader = VK_SHADER_UNUSED_NV;
      group.intersectionShader = VK_SHADER_UNUSED_NV;

      if (rt_shader_groups_[i].stage == VK_SHADER_STAGE_RAYGEN_BIT_NV) {
        group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_NV;
        group.generalShader = i;
      } else if (rt_shader_groups_[i].stage == VK_SHADER_STAGE_MISS_BIT_NV) {
        group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_NV;
        group.generalShader = i;
      } else if (rt_shader_groups_[i].stage ==
                 VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV) {
        group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_NV;
        group.closestHitShader = i;
      } else {
        std::cerr << "Unknown shader stage bit " << rt_shader_groups_[i].stage
                  << "." << std::endl;
        return false;
      }

      groups.push_back(group);
    }

    return true;
  }

  void fini_ray_tracing_shaders() {
    for (uint32_t i = 0; i < rt_shader_groups_.size(); ++i) {
      vkDestroyShaderModule(device_, rt_shader_groups_[i].module,
                            allocation_callbacks_);
    }
    rt_shader_groups_.clear();
  }

  bool init_ray_tracing_pipeline() {
    if (!init_ray_tracing_pipeline_layout()) {
      std::cerr << "Failed to init ray tracing pipeline layout." << std::endl;
      return false;
    }

    std::vector<VkRayTracingShaderGroupCreateInfoNV> groups{};
    if (!init_ray_tracing_shaders(groups)) {
      std::cerr << "Failed to init ray tracing shaders." << std::endl;
      return false;
    }

    VkRayTracingPipelineCreateInfoNV rt_pipeline_create_info{};
    rt_pipeline_create_info.sType =
        VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_NV;
    rt_pipeline_create_info.stageCount =
        static_cast<uint32_t>(rt_shader_groups_.size());
    rt_pipeline_create_info.pStages = rt_shader_groups_.data();
    rt_pipeline_create_info.groupCount = static_cast<uint32_t>(groups.size());
    rt_pipeline_create_info.pGroups = groups.data();
    rt_pipeline_create_info.maxRecursionDepth = 2;  // Normal ray + shadow ray.
    rt_pipeline_create_info.layout = rt_pipeline_layout_;

    // TODO: rt_pipeline_cache_ or re-use pipeline_cache_?
    VkPipelineCache pipeline_cache = VK_NULL_HANDLE;
    static constexpr uint32_t create_info_count = 1;

    VkResult res = vkCreateRayTracingPipelinesNV(
        device_, pipeline_cache, create_info_count, &rt_pipeline_create_info,
        allocation_callbacks_, &rt_pipeline_);
    if (VK_SUCCESS != res) {
      std::cerr << "Failed to create ray tracing pipeline: " << res
                << std::endl;
      return false;
    }

    return true;
  }

  void fini_ray_tracing_pipeline() {
    vkDestroyPipeline(device_, rt_pipeline_, allocation_callbacks_);
    rt_pipeline_ = VK_NULL_HANDLE;

    fini_ray_tracing_shaders();
    fini_ray_tracing_pipeline_layout();
  }

  bool init_ray_tracing_shader_binding_table() {
    VkDeviceSize sbt_size =
        rt_properties_.shaderGroupHandleSize * rt_shader_groups_.size();

    if (!memory_.create_buffer(sbt_size, VK_BUFFER_USAGE_RAY_TRACING_BIT_NV,
                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                               rt_shader_binding_table_.buffer,
                               rt_shader_binding_table_.mem)) {
      std::cerr << "Failed to create ray tracing shader binding table buffer."
                << std::endl;
      return false;
    }

    // Recover shader handles and copy the to SBT buffer respecting
    // shaderGroupHandleSize alingment.
    //
    void *sbt_data;
    VkDeviceSize offset = 0;
    VkMemoryMapFlags flags = 0;
    vkMapMemory(device_, rt_shader_binding_table_.mem, offset, sbt_size, flags,
                &sbt_data);
    for (uint32_t i = 0; i < rt_shader_groups_.size(); ++i) {
      uint32_t first_group = i;
      static constexpr uint32_t group_count = 1;
      uint8_t *sbt_ptr = static_cast<uint8_t *>(sbt_data) +
                         i * rt_properties_.shaderGroupHandleSize;
      VkResult res = vkGetRayTracingShaderGroupHandlesNV(
          device_, rt_pipeline_, first_group, group_count,
          rt_properties_.shaderGroupHandleSize, static_cast<void *>(sbt_ptr));
      if (VK_SUCCESS != res) {
        std::cerr << "Failed to bind shader handle #" << i << " to SBT: " << res
                  << std::endl;
        return false;
      }
    }
    vkUnmapMemory(device_, rt_shader_binding_table_.mem);

    return true;
  }

  void fini_ray_tracing_shader_binding_table() {
    vkFreeMemory(device_, rt_shader_binding_table_.mem, allocation_callbacks_);
    rt_shader_binding_table_.mem = VK_NULL_HANDLE;

    vkDestroyBuffer(device_, rt_shader_binding_table_.buffer,
                    allocation_callbacks_);
    rt_shader_binding_table_.buffer = VK_NULL_HANDLE;
  }

  void reset_ray_tracing_frame_counter() { rt_constants_.frame = -1; }

  void update_ray_tracing_frame_counter() {
    rt_constants_.frame =
        std::min(MAX_ACCUMULATED_FRAMES, rt_constants_.frame + 1);
  }

  void ray_trace(VkCommandBuffer cmd_buf) {
    update_ray_tracing_frame_counter();

    if (rt_constants_.frame >= MAX_ACCUMULATED_FRAMES) {
      return;
    }

    // Bind pipeline.
    //
    vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_RAY_TRACING_NV,
                      rt_pipeline_);

    // Bind descriptor sets.
    //
    uint32_t first_set = 0;
    uint32_t dynamic_offset_count = 0;
    const uint32_t *dynamic_offsets = nullptr;

    // TODO: Move rt_descriptor_set_ to descriptor_set_[1].
    std::vector<VkDescriptorSet> sets({rt_descriptor_set_, descriptor_set_[0]});
    vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_RAY_TRACING_NV,
                            rt_pipeline_layout_, first_set,
                            static_cast<uint32_t>(sets.size()), sets.data(),
                            dynamic_offset_count, dynamic_offsets);

    // Push constants.
    //
    rt_constants_.clear_color = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    // rt_constants_.light_position = glm::vec3(-5.0f, 0.0f, -5.0f);
    // rt_constants_.light_intensity = 1.0f;
    // rt_constants_.light_type = 1;  // point = 0, directional = 1;

    uint32_t offset = 0;
    vkCmdPushConstants(
        cmd_buf, rt_pipeline_layout_,
        VK_SHADER_STAGE_RAYGEN_BIT_NV | VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV |
            VK_SHADER_STAGE_MISS_BIT_NV,
        offset, static_cast<uint32_t>(sizeof(rt_constants_)), &rt_constants_);

    VkDeviceSize raygen_shader_binding_offset =
        0 * rt_properties_.shaderGroupHandleSize;
    VkDeviceSize miss_shader_binding_offset =
        1 * rt_properties_.shaderGroupHandleSize;
    // 2 miss shaders.
    VkDeviceSize hit_shader_binding_offset =
        3 * rt_properties_.shaderGroupHandleSize;

    VkDeviceSize binding_stride = rt_properties_.shaderGroupHandleSize;

    VkBuffer callable_sbt_buffer = VK_NULL_HANDLE;
    VkDeviceSize callable_sb_offset = 0;
    VkDeviceSize callable_sb_stride = 0;

    uint32_t depth = 1;  // depth of the ray trace query dimensions.

    vkCmdTraceRaysNV(
        cmd_buf,
        // raygen
        rt_shader_binding_table_.buffer, raygen_shader_binding_offset,
        // miss
        rt_shader_binding_table_.buffer, miss_shader_binding_offset,
        binding_stride,
        // hit
        rt_shader_binding_table_.buffer, hit_shader_binding_offset,
        binding_stride,
        // callable
        callable_sbt_buffer, callable_sb_offset, callable_sb_stride,
        // dimensions
        window_size_.width, window_size_.height, depth);
  }

  void copy_ray_tracing_output_to_swap_chain(VkCommandBuffer cmd_buf,
                                             VkImage swap_chain_image) {
    // Copy ray tracing output image to swap chain image.
    //

    helpers::transition_image_layout(cmd_buf, swap_chain_image, format_,
                                     VK_IMAGE_LAYOUT_UNDEFINED,
                                     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    helpers::transition_image_layout(
        cmd_buf, rt_storage_image_.image, rt_storage_image_.format,
        VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

    // VkImageSubresourceRange subresource_range{};
    // subresource_range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    // subresource_range.baseMipLevel = 0;
    // subresource_range.levelCount = 1;
    // subresource_range.baseArrayLayer = 0;
    // subresource_range.layerCount = 1;

    VkOffset3D offset_3d{};
    offset_3d.x = 0;
    offset_3d.y = 0;
    offset_3d.z = 0;

    VkExtent3D extent_3d{};
    extent_3d.width = window_size_.width;
    extent_3d.height = window_size_.height;
    extent_3d.depth = 1;

    VkImageCopy image_copy{};
    image_copy.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_copy.srcSubresource.mipLevel = 0;
    image_copy.srcSubresource.baseArrayLayer = 0;
    image_copy.srcSubresource.layerCount = 1;
    image_copy.srcOffset = offset_3d;
    image_copy.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_copy.dstSubresource.mipLevel = 0;
    image_copy.dstSubresource.baseArrayLayer = 0;
    image_copy.dstSubresource.layerCount = 1;
    image_copy.dstOffset = offset_3d;
    image_copy.extent = extent_3d;

    uint32_t region_count = 1;
    vkCmdCopyImage(cmd_buf, rt_storage_image_.image,
                   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, swap_chain_image,
                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, region_count,
                   &image_copy);

    helpers::transition_image_layout(cmd_buf, swap_chain_image, format_,
                                     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                     VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    helpers::transition_image_layout(
        cmd_buf, rt_storage_image_.image, rt_storage_image_.format,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);
  }

  void fini_ray_tracing() {
    rtx_.destroy(memory_);
    rt_descriptor_pool_.fini(memory_);
    fini_ray_tracing_descriptor_layout();
    fini_ray_tracing_storage_image();
    fini_ray_tracing_pipeline();
    fini_ray_tracing_shader_binding_table();
  }

  // Attributes
  //

  platform platform_;
  VkInstance instance_;
  VkDebugUtilsMessengerEXT debug_messenger_;
  std::vector<VkPhysicalDevice> gpus_;
  std::vector<VkQueueFamilyProperties> queue_props_;
  VkPhysicalDeviceMemoryProperties memory_properties_;
  VkDevice device_;
  memory memory_;
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

  std::vector<object_model_t> objects_;
  std::vector<object_instance_t> objects_instances_;

  camera camera_;

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

  // Ray Tracing stuff.
  //
  bool rtx_enabled_;
  ray_tracer rtx_;
  VkPhysicalDeviceRayTracingPropertiesNV rt_properties_;
  rt_descriptor_pool rt_descriptor_pool_;
  VkDescriptorSet rt_descriptor_set_;
  VkDescriptorSetLayout rt_descriptor_layout_;
  storage_image_t rt_storage_image_;
  ray_tracing_constants_t rt_constants_;
  std::vector<VkPipelineShaderStageCreateInfo> rt_shader_groups_;
  VkPipeline rt_pipeline_;
  VkPipelineLayout rt_pipeline_layout_;
  shader_binding_table_t rt_shader_binding_table_;
  static constexpr int MAX_ACCUMULATED_FRAMES = 1000;
  //
  // End of Ray Tracing stuff.

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
                  << layer_name << " failed: " << res << std::endl;
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
    if (!memory_.create_buffer_and_copy(image_size, usage, properties,
                                        staging_buffer, staging_buffer_memory,
                                        pixels)) {
      std::cerr << "Failed to create texture buffer." << std::endl;
      return false;
    }

    stbi_image_free(pixels);

    // VkFormat texture_format = VK_FORMAT_R8G8B8A8_SRGB;
    VkFormat texture_format = VK_FORMAT_R8G8B8A8_UNORM;

    if (!helpers::create_image(
            memory_, static_cast<uint32_t>(texture_width),
            static_cast<uint32_t>(texture_height), texture_format,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, texture_image_,
            texture_image_memory_)) {
      std::cerr << "Failed to create texture image." << std::endl;
      return false;
    }

    if (!transition_image_layout(texture_image_, texture_format,
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

    if (!transition_image_layout(texture_image_, texture_format,
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
    texture_image_ = VK_NULL_HANDLE;

    vkFreeMemory(device_, texture_image_memory_, allocation_callbacks_);
    texture_image_memory_ = VK_NULL_HANDLE;
  }

  bool create_texture_image_view() {
    // VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;
    VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
    if (!helpers::create_image_view(memory_, texture_image_, format,
                                    VK_IMAGE_ASPECT_COLOR_BIT,
                                    texture_image_view_)) {
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
    texture_image_view_ = VK_NULL_HANDLE;
  }

  bool load_texture(const std::string &texture_path) {
    if (!create_texture_image(texture_path)) {
      std::cerr << "create_texture_image() failed." << std::endl;
      return false;
    }
    if (!create_texture_image_view()) {
      std::cerr << "create_texture_image_view() failed." << std::endl;
      return false;
    }
    return true;
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
      std::cerr << "Failed to create texture sampler: " << res << std::endl;
      return false;
    }

    return true;
  }

  void cleanup_texture_sampler() {
    vkDestroySampler(device_, texture_sampler_, allocation_callbacks_);
    texture_sampler_ = VK_NULL_HANDLE;
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
                     constants::NUM_VIEWPORTS_AND_SCISSORS, &viewport_);
  }

  void init_scissors() {
    VkExtent2D window_size = platform_.window_size();
    scissor_.extent.height = window_size.height;
    scissor_.extent.width = window_size.width;
    scissor_.offset.x = 0;
    scissor_.offset.y = 0;

    static constexpr uint32_t first_scissor = 0;
    vkCmdSetScissor(command_buffers_[current_buffer_], first_scissor,
                    constants::NUM_VIEWPORTS_AND_SCISSORS, &scissor_);
  }

  bool copy_buffer(VkBuffer src_buffer, VkBuffer dst_buffer,
                   VkDeviceSize size) {
    VkCommandBuffer command_buffer;
    if (!begin_single_time_commands(command_buffer, device_, command_pool_)) {
      std::cerr << "copy buffer: begin of single time command failed."
                << std::endl;
      return false;
    }

    VkBufferCopy copy_region{};
    copy_region.size = size;

    static constexpr uint32_t region_count = 1;
    vkCmdCopyBuffer(command_buffer, src_buffer, dst_buffer, region_count,
                    &copy_region);

    if (!end_single_time_commands(command_buffer, device_, command_pool_,
                                  graphics_queue_)) {
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
    if (!begin_single_time_commands(command_buffer, device_, command_pool_)) {
      std::cerr
          << "transition image layout: begin of single time command failed."
          << std::endl;
      return false;
    }

    if (!helpers::transition_image_layout(command_buffer, image, format,
                                          old_layout, new_layout)) {
      return false;
    }

    if (!end_single_time_commands(command_buffer, device_, command_pool_,
                                  graphics_queue_)) {
      std::cerr << "transition image layout: end of single time command failed."
                << std::endl;
      return false;
    }

    return true;
  }

  bool copy_buffer_to_image(VkBuffer buffer, VkImage image, uint32_t width,
                            uint32_t height) {
    VkCommandBuffer command_buffer;
    if (!begin_single_time_commands(command_buffer, device_, command_pool_)) {
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

    if (!end_single_time_commands(command_buffer, device_, command_pool_,
                                  graphics_queue_)) {
      std::cerr << "transition image layout: end of single time command failed."
                << std::endl;
      return false;
    }

    return true;
  }
};

}  // namespace rtx
