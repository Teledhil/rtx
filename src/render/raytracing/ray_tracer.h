#pragma once

#include <iostream>

#include <vulkan/vulkan.h>

#include "raytracing/acceleration_structure.h"

namespace rtx {

class ray_tracer {
 public:
  ray_tracer() = default;

  bool build_acceleration_structures(memory &mem,
                                     VkCommandPool &command_pool,
                                     VkQueue &graphics_queue,
                                     std::vector<object_model_t> &objects,
                                     bool update) {
    // Add each object into its own BLAS.
    for (auto &object : objects) {
      if (!acceleration_structure_.add_object(object)) {
        std::cerr << "Failed to add ray tracing object." << std::endl;
        return false;
      }
    }

    if (!acceleration_structure_.generate(mem.get_device(), mem, command_pool,
                                          graphics_queue, update)) {
      std::cerr << "Failed to generate acceleration strucutures." << std::endl;
      return false;
    }

    return true;
  }

  const VkAccelerationStructureNV &get_tlas() const {
    return acceleration_structure_.get_tlas();
  }

  void destroy(memory &mem) { acceleration_structure_.destroy(mem); }

 private:
  acceleration_structure acceleration_structure_;
};
}  // namespace rtx
