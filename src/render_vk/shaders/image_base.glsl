
layout(push_constant) uniform Push {
    vec2 scale;
    vec2 imageScale;
    vec2 bicubicScale;
    vec2 viewport;
    vec2 translate;
	vec2 textureSize;
	vec2 drawSize;
	int  texIndex;
    int  filterType;
} push;

#define ImageFilter_Nearest  0
#define ImageFilter_Linear   1
#define ImageFilter_Cubic    2
#define ImageFilter_Gaussian 3

#define RADIUS 1

