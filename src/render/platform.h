#pragma once

#include <string>

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.h>

namespace rtx {

class platform {
 public:
  platform() : window_(nullptr), window_resized_(false) {}

  bool init(int width, int height, const std::string& title) {
    glfwSetErrorCallback(error_callback);

    if (GLFW_TRUE != glfwInit()) {
      std::cerr << "glfwInit() failed" << std::endl;
      return false;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    // Disable resize.
    //
    // glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    GLFWmonitor* monitor = nullptr;  // windowed mode.
    GLFWwindow* share = nullptr;     //  not share resources from window.

    window_ = glfwCreateWindow(width, height, title.c_str(), monitor, share);

    if (GLFW_TRUE != glfwVulkanSupported()) {
      std::cerr << "GLFW: Vulkan not supported." << std::endl;
      return false;
    }

    // Link this class to GLFWwindow.
    glfwSetWindowUserPointer(window_, this);

    return true;
  }

  void fini() { glfwDestroyWindow(window_); }

  static bool get_vulkan_extensions(uint32_t& num_extensions,
                                    const char**& extensions) {
    extensions = glfwGetRequiredInstanceExtensions(&num_extensions);
    if (!extensions) {
      std::cerr << "GLFW: No extensions." << std::endl;
      return false;
    }
    return true;
  }

  bool create_window_surface(VkInstance& instance,
                             const VkAllocationCallbacks* allocator,
                             VkSurfaceKHR& surface) {
    VkResult res =
        glfwCreateWindowSurface(instance, window_, allocator, &surface);
    if (VK_SUCCESS != res) {
      return false;
    }
    return true;
  }

  bool init_framebuffer() {
    int width;
    int height;
    glfwGetFramebufferSize(window_, &width, &height);
    glfwSetFramebufferSizeCallback(window_, resize_callback);

    return true;
  }

  VkExtent2D window_size() const {
    int width;
    int height;
    glfwGetFramebufferSize(window_, &width, &height);
    return VkExtent2D{static_cast<uint32_t>(width),
                      static_cast<uint32_t>(height)};
  }

  GLFWwindow* window() { return window_; }

  bool should_close_window() { return glfwWindowShouldClose(window_); }

  void poll_events() { glfwPollEvents(); }

  bool is_window_resized() const { return window_resized_; }
  void set_already_resized() { window_resized_ = false; }

  void wait_events() { glfwWaitEvents(); }

 private:
  GLFWwindow* window_;

  bool window_resized_;

  static void error_callback(int error, const char *description) {
    std::cerr << "GLFW_ERROR(" << error << "): " << description << std::endl;
  }

  static void resize_callback(GLFWwindow* window, int w, int h) {
    auto* const p = static_cast<platform*>(glfwGetWindowUserPointer(window));

    std::cout << "Windows resized to " << w << "x" << h << "." << std::endl;
    p->window_resized_ = true;
  }
};
}
