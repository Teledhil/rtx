#include <iostream>
#include <thread>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include <vulkan/vulkan.h>

#include "render/engine.h"

int main(int argc, char *argv[]) {
  int width = 1280;
  int height = 720;
  std::string title = "RTX";

  std::cout << "RTX off" << std::endl;

  bool debug = true;
  rtx::render_engine r(debug);

  bool rtx_enabled = true;
  if (!r.init(title, 1, width, height, title, rtx_enabled)) {
    std::cerr << "Render init failed with ray tracing." << std::endl;

    rtx_enabled = false;
    if (!r.init(title, 1, width, height, title, rtx_enabled)) {
      std::cerr << "Render init without ray tracing failed." << std::endl;
      return -1;
    }
  }
  std::cout << "Render ready." << std::endl;

  if (!r.draw()) {
    std::cerr << "Render draw failed." << std::endl;
    return -1;
  }

  r.fini();

  std::cout << "Bye." << std::endl;

  return 0;
}
