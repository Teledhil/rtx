#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
//layout (location = 0) in vec4 color;
layout(binding = 1) uniform sampler2D texSampler;
layout (location = 0) in vec2 inTexCoord;

layout (location = 0) out vec4 outColor;

void main() {
    //outColor = color;
    //outColor = vec4(inTexCoord, 0.0, 1.0);
    outColor = texture(texSampler, inTexCoord);
}
