# RTX

[![Build Status](https://drone.teledhil.eu/api/badges/Teledhil/rtx/status.svg)](https://drone.teledhil.eu/Teledhil/rtx)

![RTX ON](/assets/screenshots/arch.png)

Welcome to RTX, a work-in-progress ray tracing engine built using Vulkan.

## Overview

The engine leverages the NVIDIA's [VK_NV_ray_tracing](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_NV_ray_tracing.html)
extension, which was the only available option when I started working on this.
Consecuently, an NVIDIA RTX GPU is currently required to utilize the ray tracing
pipeline. I plan to migrate the code to the cross-platform
[VK_KHR_ray_tracing_pipeline](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_ray_tracing_pipeline.html)
extension, now that I have a Steam Deck.

For those without an NVIDIA RTX GPU, the engine will gracefully fallback to a
minimal rasterizer pipeline.

![rasterization](/assets/screenshots/macOS.png)

## Features

* Utilizes the [tinyobjloader](https://github.com/tinyobjloader/tinyobjloader)
library to load textured Wavefront OBJ models, only one model can be loaded.
* Provides a simple UI with settings and stats using [Dear
ImGui](https://github.com/ocornut/imgui).
* A [timing heat
map](https://developer.nvidia.com/blog/profiling-dxr-shaders-with-timer-instrumentation/)
of the time spent to draw each pixel.

![heat map](/assets/screenshots/temperature.png)

* Light reflection on metal and lambertian materials.
* A single source of light. It's position and intensity can be adjusted in the
UI.
* Mouse support. Models can be rotated and the mouse wheel lets you zoom in or
out.
* Move around the scene using the WASD keys.


## Build instructions

RTX can be built and executed on Linux, macOS, and Windows (with Git Bash and
MSYS2). Ensure you have VulkanSDK 1.3 installed, although it should also work
with Vulkan 1.2 after making a few modifications.


### Linux
* On *Ubuntu*, refer to the `Dockerfile` or follow this steps:
  * Install the [VulkanSDK](https://vulkan.lunarg.com/sdk/home) Ubuntu package.
  * Install the following packages: `clang cmake curl git libgl-dev libx11-dev libxcursor-dev libxi-dev libxinerama-dev libxrandr-dev ninja-build`.
* On *Arch Linux*, install the following packages: `clang cmake curl git libglvnd libx11 libxcursor libxi libxinerama libxrandr ninja vulkan-devel`.


### macOS:
* Install the [VulkanSDK](https://vulkan.lunarg.com/sdk/home) Installer.
* Install [Hombrew](https://brew.sh/) and the required packages using the
  following command:
```
brew install cmake git ninja
```

To build and run RTX, follow these steps:
* Clone the repo and its submodules:
```
git clone --recursive https://github.com/Teledhil/rtx.git
```

* Build with:
```
./build.sh
```

* Launch the render with:
```
./_build/bin/rtx
```

## References

* The [Ray Tracing in One Weekend](https://raytracing.github.io/books/RayTracingInOneWeekend.html) books
* [Vulkan Tutorial](https://vulkan-tutorial.com/)
* Sascha Willems's [Vulkan C++ examples and
  demos](https://github.com/SaschaWillems/Vulkan)
* [NVIDIA Vulkan Ray Tracing
  Tutorials](https://github.com/nvpro-samples/vk_raytracing_tutorial_NV)
* GPSnoopy's
  [RayTracingInVulkan](https://github.com/GPSnoopy/RayTracingInVulkan)

