#version 450
#extension GL_ARB_separate_shader_objects : enable

// bindings: things in memory that are shared between the shader and the main program
// good to know they do exist

layout (binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

// getting the inpust from the program:
layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inColor;

layout(location = 0) out vec3 fragColor;

void main() {
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
    fragColor = inColor;
}