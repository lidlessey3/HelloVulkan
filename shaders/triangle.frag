#version 450
#extension GL_ARB_separate_shader_objects : enable

//layout(binding = 1) uniform samplerCube cubeTextSampler;
layout (binding = 1) uniform  sampler2D textSampler;


layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragTextCoord;

layout(location = 0) out vec4 outColor;

void main() {
    //outColor = texture(cubeTextSampler, fragTextCoord);
    // outColor = vec4(fragColor, 1.0);
    if(fragTextCoord.z == 0) {
        vec2 textCoord;
        textCoord.x = fragTextCoord.x;
        textCoord.y = fragTextCoord.y;

        outColor = texture(textSampler, textCoord);
    }
    else
        outColor = vec4(1.0,1.0,1.0,1.0);
}
