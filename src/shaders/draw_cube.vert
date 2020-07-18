#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// In
layout (std140, binding = 0) uniform bufferVals {
    mat4 mvp;
    mat4 inverse_view;
    mat4 inverse_projection;
} myBufferVals;
layout (location = 0) in vec4 pos;
layout (location = 1) in vec2 inTexCoord;

// Out
layout (location = 0) out vec2 outTexCoord;

void main() {
   gl_Position = myBufferVals.mvp * pos;
   outTexCoord = inTexCoord;
}
