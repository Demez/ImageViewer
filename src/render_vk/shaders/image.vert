#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(push_constant) uniform Push {
    vec2 scale;
    vec2 translate;
	int  texIndex;
} push;

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inTexCoord;

layout(location = 0) out vec2 fragTexCoord;

void main() {
    gl_Position = vec4(inPosition * push.scale + push.translate, 0.0, 1.0);
    // gl_Position = vec4(inPosition, 0.0, 1.0);
    // gl_Position = vec4(push.scale + push.translate, 0.0, 1.0);

    fragTexCoord = inTexCoord;
    // fragTexCoord = inTexCoord * push.scale + push.translate;
}
