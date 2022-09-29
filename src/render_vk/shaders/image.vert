#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : require

#include "image_base.glsl"

precision highp float;

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inTexCoord;

layout(location = 0) out vec2 fragTexCoord;

mat2 RotateMat2( float angle )
{
	float s = sin( angle );
	float c = cos( angle );

	return mat2( c, -s, s, c );
}

void main()
{
    vec2 scale = 2.0f / push.viewport;
    vec2 translate = push.translate * scale;

    // create a rotation matrix
    mat2 rotMat = RotateMat2( push.rotation );

    vec2 finalRotMat = ( inPosition * ( push.drawSize * 0.5 ) ) * rotMat;

    gl_Position = vec4( finalRotMat * scale + translate, 0.0, 1.0 );

    fragTexCoord = inTexCoord;
}

