#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (std140, binding = 0) uniform bufferVals {
    mat4 mvp;
} myBufferVals;
layout (location = 0) in vec4 pos;
//layout (location = 1) in vec4 inColor;
layout (location = 1) in vec2 inTexCoord;

//layout (location = 0) out vec4 outColor;
layout (location = 0) out vec2 outTexCoord;

void main() {
   //outColor = inColor;
   gl_Position = myBufferVals.mvp * pos;
   outTexCoord = inTexCoord;
}
