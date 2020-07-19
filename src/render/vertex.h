#pragma once

#include "glm.h"

namespace rtx {

struct Vertex {
  glm::vec3 pos;
  glm::vec3 normal;
  glm::vec2 tex_coord;

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

    // return (hash<glm::vec3>()(vertex.pos) ^
    //        (hash<glm::vec2>()(vertex.tex_coord) << 1));
  }

 private:
  // From boost::hash_combine
  static size_t hash_combine(size_t h0, size_t h1) {
    return h0 ^ (h1 + 0x9e3779b9 + (h0 << 6) + (h0 >> 2));
  }
};

}  // namespace std
