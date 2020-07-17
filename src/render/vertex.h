#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

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
