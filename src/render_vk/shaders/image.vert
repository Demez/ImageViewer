#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : require

#include "image_base.glsl"

precision highp float;

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inTexCoord;

layout(location = 0) out vec2 fragTexCoord;

vec2 Rotate( vec2 pos, float angle )
{
	float s = sin( angle );
	float c = cos( angle );

    vec2 rotatedPosition = vec2(
     pos.x * c + pos.y * s,
     pos.y * c - pos.x * s );

	// mat2 rotMat = mat2( c, -s, s, c );
	// return rotMat * pos;
    return rotatedPosition;
}


mat2 Rotate2D(float _angle){
    return mat2(cos(_angle),-sin(_angle),
                sin(_angle),cos(_angle));
}

void main()
{


    // gl_Position = vec4( inPosition * push.scale + push.translate, 0.0, 1.0 );
    // gl_Position = vec4( inPosition + push.translate, 0.0, 1.0 );
    // gl_Position = vec4( Rotate( inPosition * push.scale, push.rotation ) + push.translate, 0.0, 1.0 );
    gl_Position = vec4( Rotate( inPosition, push.rotation ) * push.scale + push.translate, 0.0, 1.0 );
    // gl_Position = vec4( Rotate2( inPosition, push.rotation ) + push.translate, 0.0, 1.0 );
    //gl_Position = vec4( Rotate( inPosition, push.rotation ) * Rotate( push.scale, push.rotation ) + push.translate, 0.0, 1.0 );
    fragTexCoord = inTexCoord;
}

