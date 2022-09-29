#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : enable

#include "image_base.glsl"
#include "filters.glsl"

precision highp float;

layout(set = 0, binding = 0) uniform sampler2D[] texSamplers;
layout(set = 1, binding = 0, rgba8) uniform readonly image2D[] img;

layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;


void main()
{
    if ( push.filterType == ImageFilter_Cubic )
	{
		// outColor = BiCubic( texSamplers[push.texIndex], fragTexCoord, push.textureSize, push.drawSize );
		// outColor = DoPixelBinning( texSamplers[push.texIndex], fragTexCoord, push.textureSize, push.drawSize );
		outColor = DoPixelBinning( texSamplers[push.texIndex], fragTexCoord, textureSize( texSamplers[push.texIndex], 0 ), push.drawSize );
	}
    else if ( push.filterType == ImageFilter_Gaussian )
	{
        outColor = texture( texSamplers[push.texIndex+1], fragTexCoord );
        // outColor = Lanczos( texSamplers[push.texIndex], fragTexCoord );
	}
    else
    {
        outColor = texture( texSamplers[push.texIndex], fragTexCoord );
    }
}

