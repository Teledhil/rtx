#include <iostream>

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

  rtx::render_engine r;

  if (!r.init("RTX", 1, width, height, title)) {
    std::cerr << "Render init failed." << std::endl;
    return -1;
  }

  return 0;
}
