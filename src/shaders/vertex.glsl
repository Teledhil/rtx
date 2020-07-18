struct Vertex {
  vec3 position;
  vec3 normal;
  vec3 color;
  vec2 tex_coord; // uv.
};

Vertex unpack(uint index) {

  vec4 d0 = vertices.v[3 * index + 0];
  vec4 d1 = vertices.v[3 * index + 1];
  vec4 d2 = vertices.v[3 * index + 2];

  Vertex v;
  v.position = d0.xyz;
  v.normal = vec3(d0.w, d1.x, d1.y);
  v.color = vec3(d1.z, d1.w, d2.x);

  return v;
}
