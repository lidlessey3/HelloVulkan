#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 1) uniform samplerCube cubeTextSampler;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragTextCoord;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = texture(cubeTextSampler, fragTextCoord);
    //outColor = vec4(fragTextCoord, 1.0);
}