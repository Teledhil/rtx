#pragma once

#include "glm.h"

namespace rtx {

struct Vertex {
  glm::vec3 pos;
  glm::vec3 normal;
  glm::vec2 tex_coord;

  static VkVertexInputBindingDescription get_binding_description() {
    VkVertexInputBindingDescription binding_description{};
    binding_description.binding = 0;
    binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    binding_description.stride = sizeof(Vertex);

    return binding_description;
  }

  static const std::vector<VkVertexInputAttributeDescription>
  get_attribute_descriptions() {
    std::vector<VkVertexInputAttributeDescription> attribute_descriptions;
    attribute_descriptions.resize(2);

    // Allowed formats are:
    // (from:
    // https://vulkan-tutorial.com/Vertex_buffers/Vertex_input_description#page_Attribute-descriptions)
    //
    // float: VK_FORMAT_R32_SFLOAT
    // vec2: VK_FORMAT_R32G32_SFLOAT
    // vec3: VK_FORMAT_R32G32B32_SFLOAT
    // vec4: VK_FORMAT_R32G32B32A32_SFLOAT

    // Location 0: Vertex position.
    //
    attribute_descriptions[0].binding = 0;
    attribute_descriptions[0].location = 0;
    attribute_descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;  // aka vec3
    attribute_descriptions[0].offset = offsetof(Vertex, pos);

    // TODO: normal?

    // Location 1: Texture coordinates.
    attribute_descriptions[1].binding = 0;
    attribute_descriptions[1].location = 1;
    attribute_descriptions[1].format =
        VK_FORMAT_R32G32_SFLOAT;  // texture coordinates are 2D.
    attribute_descriptions[1].offset = offsetof(Vertex, tex_coord);

    return attribute_descriptions;
  }

  bool operator==(const Vertex &other) const {
    return pos == other.pos && normal == other.normal &&
           tex_coord == other.tex_coord;
  }
};

}  // namespace rtx

namespace std {

template <>
struct hash<rtx::Vertex> {
  size_t operator()(rtx::Vertex const &vertex) const {
    return hash_combine(hash<glm::vec3>()(vertex.pos),
                        hash_combine(hash<glm::vec3>()(vertex.normal),
                                     hash<glm::vec2>()(vertex.tex_coord)));
  }

 private:
  // From boost::hash_combine
  static size_t hash_combine(size_t h0, size_t h1) {
    return h0 ^ (h1 + 0x9e3779b9 + (h0 << 6) + (h0 >> 2));
  }
};

}  // namespace std
