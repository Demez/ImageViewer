#pragma once

#include <vector>
#include <string>

#include "platform.h"

struct ImageInfo;


struct ImageDrawInfo
{
	double aX;
	double aY;
	int aWidth;
	int aHeight;
};

#ifdef RENDER_DLL

extern "C"
{
	DLL_EXPORT bool Render_Init( void* spWindow );
	DLL_EXPORT void Render_Shutdown();

	DLL_EXPORT void Render_NewFrame();
	DLL_EXPORT void Render_Draw();

	DLL_EXPORT bool Render_LoadImage( ImageInfo* spInfo, std::vector< char >& srData );
	DLL_EXPORT void Render_FreeImage( ImageInfo* spInfo );
	DLL_EXPORT void Render_DrawImage( ImageInfo* spInfo, const ImageDrawInfo& srDrawInfo );
}

#else

// ---------------------------------------------------------------------------------------------------

typedef bool ( *Render_Init_t )( void* spWindow );

typedef void ( *Render_Shutdown_t )();

typedef void ( *Render_NewFrame_t )();
typedef void ( *Render_Draw_t )();

typedef bool ( *Render_LoadImage_t )( ImageInfo* spInfo, std::vector< char >& srData );
typedef void ( *Render_FreeImage_t )( ImageInfo* spInfo );
typedef void ( *Render_DrawImage_t )( ImageInfo* spInfo, const ImageDrawInfo& srDrawInfo );

// ---------------------------------------------------------------------------------------------------

extern Render_Init_t      Render_Init;
extern Render_Shutdown_t  Render_Shutdown;

extern Render_NewFrame_t  Render_NewFrame;
extern Render_Draw_t      Render_Draw;

extern Render_LoadImage_t Render_LoadImage;
extern Render_FreeImage_t Render_FreeImage;
extern Render_DrawImage_t Render_DrawImage;

#endif

