#version 460
#extension GL_NV_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_GOOGLE_include_directive : enable

#include "ray_common.glsl"

hitAttributeNV vec3 attribs;

// Result image from raygen shader.
layout(location = 0) rayPayloadInNV hitPayload hit_payload;

// Whether an occluder was found.
layout(location = 1) rayPayloadNV bool is_shadowed;

// RT constants.
// TODO: Move definition to ray_common.glsl
layout(push_constant) uniform Constants {
  vec4 clear_color;
  vec3 light_position;
  float light_intensity;
  int light_type;
  int frame;
  int samples;
  int max_iterations;
  bool temperature;
} constants;

// Top-Level Acceleration Structure.
layout(binding = 0, set = 0) uniform accelerationStructureNV tlas;

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

  // hit_payload.hit_value = vec3(0.0, 0.0, 1.0) * 0.8;


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
  fake_material.illumination = 2; // 3 to enable reflection
  fake_material.specular = vec3(0.8);

  // Diffuse
  //
  vec3 diffuse = compute_diffuse_lol(fake_material, light, normal);
  vec2 texture_coord = v0.texture_coord * barycenter_coordinates.x + v1.texture_coord * barycenter_coordinates.y + v2.texture_coord * barycenter_coordinates.z;
  diffuse *= texture(texture_sampler, texture_coord).xyz;




  // Shadow Ray. Ray that goes from hit point to light source.
  //
  float attenuation = 1;
  vec3 specular = vec3(0);

  // Trace shadow ray only if light is visible from the hit surface.
  if (dot(normal, light) > 0) {

    vec3 origin = gl_WorldRayOriginNV + gl_WorldRayDirectionNV * gl_HitTNV;
    vec3 direction = light;

    // Make all objects opaque, to skip c-hit shader and consider first hit as
    // valid.
    const uint ray_flags = gl_RayFlagsOpaqueNV | gl_RayFlagsSkipClosestHitShaderNV | gl_RayFlagsTerminateOnFirstHitNV;

    // Rendering distance range.
    float t_min     = 0.001;
    float t_max     = 10000.0;

    is_shadowed = true;

    traceNV(tlas,           // acceleration structure
            ray_flags,      // rayFlags
            0xFF,           // cullMask
            0,              // sbtRecordOffset
            0,              // sbtRecordStride
            1,              // missIndex (shadow miss).
            origin,         // ray origin
            t_min,          // ray min range
            direction,      // ray direction
            t_max,          // ray max range
            1               // payload (location = 1) is_shadowed.
    );

    if (is_shadowed) {
      attenuation = 0.3;
    } else {

      // Specular
      //
      specular = compute_specular_lol(fake_material, gl_WorldRayDirectionNV, light, normal);
    }
  }

  //vec3 pixel_color = vec3(light_intensity * attenuation * (diffuse + specular));
  // hit_payload.hit_value = pixel_color;

  // Reflection
  if (fake_material.illumination == 3) {
    hit_payload.attenuation *= fake_material.specular;
    hit_payload.done = 0;
    hit_payload.ray_origin = origin;
    hit_payload.ray_direction = reflect(gl_WorldRayDirectionNV, normal);
  }
  hit_payload.hit_value = vec3(attenuation * light_intensity * (diffuse + specular));


}
