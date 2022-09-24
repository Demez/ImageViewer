#pragma once

#include <vector>
#include <string>

#include "platform.h"

struct ImageInfo;
struct ImDrawData;
class ivec2;
typedef void* ImTextureID;


enum ImageFilter : unsigned char
{
	ImageFilter_Nearest,
	ImageFilter_Linear,
	ImageFilter_Cubic,
	ImageFilter_Gaussian,

	ImageFilter_Count,
	ImageFilter_VulkanCount = ImageFilter_Linear+1,
};


struct ImageDrawInfo
{
	double aX;
	double aY;
	int aWidth;
	int aHeight;
	ImageFilter aFilter = ImageFilter_Cubic;
};


#ifdef RENDER_DLL
  #define RENDER_DLL_FUNC( type, name, ... ) \
	DLL_EXPORT type name( __VA_ARGS__ )

extern "C"
{
#else
  #define RENDER_DLL_FUNC( type, name, ... ) \
	typedef type ( *##name##_t )( __VA_ARGS__ );  \
	extern name##_t name

#endif

RENDER_DLL_FUNC( bool, Render_Init, void* spWindow );
RENDER_DLL_FUNC( void, Render_Shutdown, void );

RENDER_DLL_FUNC( void, Render_NewFrame, void );
RENDER_DLL_FUNC( void, Render_Reset, void );
RENDER_DLL_FUNC( void, Render_Present, void );

RENDER_DLL_FUNC( void, Render_SetResolution, int sWidth, int sHeight );
RENDER_DLL_FUNC( void, Render_SetClearColor, int r, int g, int b );
RENDER_DLL_FUNC( void, Render_GetClearColor, int& r, int& g, int& b );

RENDER_DLL_FUNC( bool, Render_LoadImage, ImageInfo* spInfo, std::vector< char >& srData );
RENDER_DLL_FUNC( void, Render_FreeImage, ImageInfo* spInfo );
RENDER_DLL_FUNC( void, Render_DrawImage, ImageInfo* spInfo, const ImageDrawInfo& srDrawInfo );
RENDER_DLL_FUNC( void, Render_DownscaleImage, ImageInfo* spInfo, const ivec2& srDestSize );
RENDER_DLL_FUNC( ImTextureID, Render_AddTextureToImGui, ImageInfo* spInfo );

#ifdef RENDER_DLL
}
#endif

#undef RENDER_DLL_FUNC
