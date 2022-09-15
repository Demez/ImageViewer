#pragma once

#include <vector>
#include <string>

#include "platform.h"

struct ImageInfo;
struct ImDrawData;


struct ImageDrawInfo
{
	double aX;
	double aY;
	int aWidth;
	int aHeight;
};

#if 0
#ifdef RENDER_DLL

#define RENDER_DLL_FUNC( type, name ) \
	DLL_EXPORT type name()

#define RENDER_DLL_FUNC_ARGS( type, name, ... ) \
	DLL_EXPORT type name( __VA_ARGS__ )

extern "C"
{
	RENDER_DLL_FUNC_ARGS( bool, Render_Init, void* spWindow );

	// DLL_EXPORT bool Render_Init( void* spWindow );
	DLL_EXPORT void Render_Shutdown();

	DLL_EXPORT void Render_NewFrame();
	DLL_EXPORT void Render_DrawImGui( ImDrawData* spImDrawData );
	DLL_EXPORT void Render_Present();

	DLL_EXPORT void Render_SetClearColor( int r, int g, int b );
	DLL_EXPORT void Render_GetClearColor( int& r, int& g, int& b );

	DLL_EXPORT bool Render_LoadImage( ImageInfo* spInfo, std::vector< char >& srData );
	DLL_EXPORT void Render_FreeImage( ImageInfo* spInfo );
	DLL_EXPORT void Render_DrawImage( ImageInfo* spInfo, const ImageDrawInfo& srDrawInfo );
}

#else

// ---------------------------------------------------------------------------------------------------

typedef bool ( *Render_Init_t )( void* spWindow );
typedef void ( *Render_Shutdown_t )();

typedef void ( *Render_NewFrame_t )();
typedef void ( *Render_DrawImGui_t )( ImDrawData* spImDrawData );
typedef void ( *Render_Present_t )();

typedef void ( *Render_SetClearColor_t )( int r, int g, int b );
typedef void ( *Render_GetClearColor_t )( int& r, int& g, int& b );

typedef bool ( *Render_LoadImage_t )( ImageInfo* spInfo, std::vector< char >& srData );
typedef void ( *Render_FreeImage_t )( ImageInfo* spInfo );
typedef void ( *Render_DrawImage_t )( ImageInfo* spInfo, const ImageDrawInfo& srDrawInfo );

// ---------------------------------------------------------------------------------------------------

extern Render_Init_t          Render_Init;
extern Render_Shutdown_t      Render_Shutdown;

extern Render_NewFrame_t      Render_NewFrame;
extern Render_DrawImGui_t     Render_DrawImGui;
extern Render_Present_t       Render_Present;

extern Render_SetClearColor_t Render_SetClearColor;
extern Render_GetClearColor_t Render_GetClearColor;

extern Render_LoadImage_t     Render_LoadImage;
extern Render_FreeImage_t     Render_FreeImage;
extern Render_DrawImage_t     Render_DrawImage;

#endif
#endif


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
	RENDER_DLL_FUNC( void, Render_DrawImGui, ImDrawData* spImDrawData );
	RENDER_DLL_FUNC( void, Render_Present, void );

	RENDER_DLL_FUNC( void, Render_SetClearColor, int r, int g, int b );
	RENDER_DLL_FUNC( void, Render_GetClearColor, int& r, int& g, int& b );

	RENDER_DLL_FUNC( bool, Render_LoadImage, ImageInfo* spInfo, std::vector< char >& srData );
	RENDER_DLL_FUNC( void, Render_FreeImage, ImageInfo* spInfo );
	RENDER_DLL_FUNC( void, Render_DrawImage, ImageInfo* spInfo, const ImageDrawInfo& srDrawInfo );

#ifdef RENDER_DLL
}
#endif

#undef RENDER_DLL_FUNC

