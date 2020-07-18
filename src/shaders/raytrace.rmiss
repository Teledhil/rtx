#version 460
#extension GL_NV_ray_tracing : require
#extension GL_GOOGLE_include_directive : enable

#include "ray_common.glsl"

//layout(location = 0) rayPayloadInNV vec3 hitValue;

//Result image from raygen shader.
layout(location = 0) rayPayloadInNV hitPayload prd;

// RT constants.
layout(push_constant) uniform Constants {
  vec4 clear_color;
  vec3 light_position;
  float light_intensity;
  int light_type;
};

void main()
{
  // hitValue = vec3(0.5, 0.5, 0.5);

  prd.hit_value = clear_color.xyz * 0.8;
}
