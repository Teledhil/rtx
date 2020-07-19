#version 460
#extension GL_NV_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_GOOGLE_include_directive : enable

#include "ray_common.glsl"

hitAttributeNV vec3 attribs;

//Result image from raygen shader.
layout(location = 0) rayPayloadInNV hitPayload prd;

// RT constants.
layout(push_constant) uniform Constants {
  vec4 clear_color;
  vec3 light_position;
  float light_intensity;
  int light_type;
} constants;

// Vertices.
layout(binding = 2, set = 0) buffer Vertices {
  float v[];
} vertices;

// Indices.
layout(binding = 3, set = 0) buffer Indices {
  uint i[];
} indices;

// Texture.
// TODO: Textures.
layout(binding = 1, set = 1) uniform sampler2D texture_sampler;


#include "vertex.glsl"
#include "material.glsl"

void main()
{

  //prd.hit_value = vec3(0.0, 0.0, 1.0) * 0.8;


  // Object of the instance.
  //uint object_id = 0;

  // Indices of the triangle.
  ivec3 index = ivec3(indices.i[3 * gl_PrimitiveID], indices.i[3 * gl_PrimitiveID + 1], indices.i[3 * gl_PrimitiveID + 2]);

  // Vertices of the triangle.
  Vertex v0 = unpack(index.x);
  Vertex v1 = unpack(index.y);
  Vertex v2 = unpack(index.z);

  // Barycenter coordinates of the triangle.
  const vec3 barycenter_coordinates = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);

  // Normal at the hit position.
  vec3 normal = normalize(v0.normal * barycenter_coordinates.x + v1.normal * barycenter_coordinates.y + v2.normal * barycenter_coordinates.z);

  // vec3 origin = v0.position * barycenter_coordinates.x + v1.position * barycenter_coordinates.y + v2.position * barycenter_coordinates.z;
  vec3 origin = gl_WorldRayOriginNV + gl_WorldRayDirectionNV * gl_HitTNV;

  // Vector torward the light.
  vec3 light;
  float light_intensity = constants.light_intensity;
  float light_distance = 100000.0;

  if (constants.light_type == 0) { // point.
    vec3 light_direction = constants.light_position - origin;
    light_distance = length(light_direction);
    light_intensity = constants.light_intensity / (light_distance * light_distance);
    light = normalize(light_direction);
  } else { // directional.
    light = normalize(constants.light_position - vec3(0));
  }

  Material fake_material;

  // Diffuse
  //
  vec3 diffuse = compute_diffuse_lol(fake_material, light, normal);
  vec2 texture_coord = v0.texture_coord * barycenter_coordinates.x + v1.texture_coord * barycenter_coordinates.y + v2.texture_coord * barycenter_coordinates.z;
  diffuse *= texture(texture_sampler, texture_coord).xyz;

  // Specular
  //
  vec3 specular = compute_specular_lol(fake_material, gl_WorldRayDirectionNV, light, normal);

  vec3 pixel_color = vec3(light_intensity * (diffuse + specular));

  //float dot_nl = max(dot(normal, light), 0.2);
  //prd.hit_value = vec3(dot_nl);

  //pixel_color = pixel_color * vec3(1, 1, 0.8);

  prd.hit_value = pixel_color;
}
