#pragma once

#include <iostream>
#include <string>

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.h>

namespace rtx {

// Key null (265) pressed.
// Key null (265) released.
// Key w (87) pressed.
// Key w (87) released.
// Key null (264) pressed.
// Key null (264) released.
// Key s (83) pressed.
// Key s (83) released.
// Key null (263) pressed.
// Key null (263) released.
// Key a (65) pressed.
// Key a (65) released.
// Key null (262) pressed.
// Key null (262) released.
// Key d (68) pressed.
// Key d (68) released.
enum KEYS {
  W = 87,
  A = 65,
  S = 83,
  D = 68,
  UP = 265,
  DOWN = 264,
  LEFT = 263,
  RIGHT = 262,
};

class platform {
 public:
  platform()
      : window_(nullptr),
        window_resized_(false),
        left_mouse_clicked_(false),
        x_pos_(0),
        y_pos_(0),
        clicked_x_pos_(0),
        clicked_y_pos_(0),
        last_x_reported_(0),
        last_y_reported_(0),
        recorded_scroll_y_(0),
        key_x_pos_(0),
        key_y_pos_(0) {}

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
      std::cerr << "GLFW: Creation of window surface failed: " << res
                << std::endl;
      return false;
    }
    return true;
  }

  bool init_framebuffer() {
    int width;
    int height;
    glfwGetFramebufferSize(window_, &width, &height);
    glfwSetFramebufferSizeCallback(window_, resize_callback);

    // Keyboard callback
    //
    glfwSetKeyCallback(window_, key_callback);

    // Mouse callback
    //
    glfwSetCursorPosCallback(window_, cursor_position_callback);
    glfwSetCursorEnterCallback(window_, cursor_enter_callback);
    glfwSetMouseButtonCallback(window_, mouse_button_callback);
    glfwSetScrollCallback(window_, scroll_callback);

    // Mouse cursor normal.
    //
    glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    // Enable mouse raw mode if suported.
    if (glfwRawMouseMotionSupported()) {
      std::cout << "Mouse raw mode enabled." << std::endl;
      glfwSetInputMode(window_, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    } else {
      std::cout << "Mouse raw mode not supported." << std::endl;
    }

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

  void get_mouse_drag_movement(double& x, double& y) {
    double new_x_reported = clicked_x_pos_;
    double new_y_reported = clicked_y_pos_;

    x = last_x_reported_ - new_x_reported;
    y = last_y_reported_ - new_y_reported;

    last_x_reported_ = new_x_reported;
    last_y_reported_ = new_y_reported;

    // if (x != 0 || y != 0) {
    //  std::cout << "Mouse movement: " << x << " " << y << "." << std::endl;
    //}
  }

  void get_mouse_scroll_input(double& scroll_y) {
    scroll_y = recorded_scroll_y_;
    recorded_scroll_y_ = 0;
  }

  void get_wasd(double& key_x, double& key_y) {
    key_x = key_x_pos_;
    key_x_pos_ = 0;

    key_y = key_y_pos_;
    key_y_pos_ = 0;
  }

 private:
  GLFWwindow* window_;

  bool window_resized_;

  bool left_mouse_clicked_;
  double x_pos_;
  double y_pos_;
  double clicked_x_pos_;
  double clicked_y_pos_;
  double last_x_reported_;
  double last_y_reported_;
  double recorded_scroll_y_;

  double key_x_pos_;
  double key_y_pos_;

  static void error_callback(int error, const char* description) {
    std::cerr << "GLFW_ERROR(" << error << "): " << description << std::endl;
  }

  static void resize_callback(GLFWwindow* window, int w, int h) {
    auto* const p = static_cast<platform*>(glfwGetWindowUserPointer(window));

    std::cout << "Windows resized to " << w << "x" << h << "." << std::endl;
    p->window_resized_ = true;
  }

  static void key_callback(GLFWwindow* window, int key, int scancode,
                           int action, int mods) {
    auto* const p = static_cast<platform*>(glfwGetWindowUserPointer(window));

    const char* key_name = glfwGetKeyName(key, 0);
    if (!key_name) {
      key_name = "null";
    }

    switch (action) {
      case GLFW_PRESS:
        std::cout << "Key " << key_name << " (" << key << ") pressed."
                  << std::endl;
        p->key_pressed_or_repeated(key);
        break;
      case GLFW_RELEASE:
        std::cout << "Key " << key_name << " (" << key << ") released."
                  << std::endl;
        break;
      case GLFW_REPEAT:
        std::cout << "Key " << key_name << " (" << key << ") repeated."
                  << std::endl;
        p->key_pressed_or_repeated(key);
        break;
      default:
        std::cout << "Key " << key_name << " (" << key << ") did something ("
                  << action << ")." << std::endl;
    }
  }

  void key_pressed_or_repeated(int key) {
    switch (key) {
      case KEYS::W:
      case KEYS::UP:
        key_up();
        break;
      case KEYS::S:
      case KEYS::DOWN:
        key_down();
        break;
      case KEYS::A:
      case KEYS::LEFT:
        key_left();
        break;
      case KEYS::D:
      case KEYS::RIGHT:
        key_right();
        break;
      default:
        break;
    }
  }

  void key_up() { key_x_pos_ += 0.1; }
  void key_down() { key_x_pos_ -= 0.1; }
  void key_left() { key_y_pos_ += 0.1; }
  void key_right() { key_y_pos_ -= 0.1; }

  static void cursor_position_callback(GLFWwindow* window, double xpos,
                                       double ypos) {
    auto* const p = static_cast<platform*>(glfwGetWindowUserPointer(window));
    if (p->left_mouse_clicked_) {
      // std::cout << "Mouse: " << xpos << "x" << ypos << "." << std::endl;
      p->clicked_x_pos_ = xpos;
      p->clicked_y_pos_ = ypos;
    }
    p->x_pos_ = xpos;
    p->y_pos_ = ypos;
  }

  static void cursor_enter_callback(GLFWwindow* window, int entered) {
    if (entered) {
      std::cout << "Mouse entered window." << std::endl;
    } else {
      std::cout << "Mouse exited window." << std::endl;
    }
  }

  static const char* get_mouse_button_name(int button) {
    switch (button) {
      case GLFW_MOUSE_BUTTON_LEFT:
        return "left";
      case GLFW_MOUSE_BUTTON_RIGHT:
        return "right";
      case GLFW_MOUSE_BUTTON_MIDDLE:
        return "middle";
      case GLFW_MOUSE_BUTTON_LAST:
        return "last";
      case GLFW_MOUSE_BUTTON_4:
      case GLFW_MOUSE_BUTTON_5:
      case GLFW_MOUSE_BUTTON_6:
      case GLFW_MOUSE_BUTTON_7:
        return "other";

      default:
        return "unknown";
    }
  }

  static void mouse_button_callback(GLFWwindow* window, int button, int action,
                                    int mods) {
    auto* const p = static_cast<platform*>(glfwGetWindowUserPointer(window));

    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    const char* button_name = get_mouse_button_name(button);

    switch (action) {
      case GLFW_PRESS:
        if (GLFW_MOUSE_BUTTON_LEFT == button) {
          p->left_mouse_clicked_ = true;
          p->clicked_x_pos_ = xpos;
          p->clicked_y_pos_ = ypos;
          p->last_x_reported_ = xpos;
          p->last_y_reported_ = ypos;
        }
        std::cout << "Mouse button " << button_name << " pressed at " << xpos
                  << "x" << ypos << "." << std::endl;
        break;
      case GLFW_RELEASE:
        if (GLFW_MOUSE_BUTTON_LEFT == button) {
          p->left_mouse_clicked_ = false;
        }
        std::cout << "Mouse button " << button_name << " released at " << xpos
                  << "x" << ypos << "." << std::endl;
        break;
      case GLFW_REPEAT:
        std::cout << "Mouse button " << button_name << " repeated at " << xpos
                  << "x" << ypos << "." << std::endl;
        break;
      default:
        std::cout << "Mouse button " << button_name << " did something at "
                  << xpos << "x" << ypos << "." << std::endl;
    }
  }

  static void scroll_callback(GLFWwindow* window, double xoffset,
                              double yoffset) {
    auto* const p = static_cast<platform*>(glfwGetWindowUserPointer(window));
    p->recorded_scroll_y_ -= yoffset;
    std::cout << "Mouse scroll: " << xoffset << " " << yoffset << std::endl;
  }
};
}  // namespace rtx
