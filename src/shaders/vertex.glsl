struct Vertex {
  vec3 position;
  vec3 normal;
  vec2 texture_coord;  // uv.
};

Vertex unpack(uint index) {
  const uint offset = 8 * index;

  Vertex v;
  v.position = vec3(vertices.v[offset + 0], vertices.v[offset + 1],
                    vertices.v[offset + 2]);
  v.normal = vec3(vertices.v[offset + 3], vertices.v[offset + 4],
                  vertices.v[offset + 5]);
  v.texture_coord = vec2(vertices.v[offset + 6], vertices.v[offset + 7]);

  return v;
}
