#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// In
layout(binding = 1) uniform sampler2D texSampler;
layout (location = 0) in vec2 inTexCoord;

// Out
layout (location = 0) out vec4 outColor;

void main() {
    outColor = texture(texSampler, inTexCoord);
}
