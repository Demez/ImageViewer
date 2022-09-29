#pragma once

#include <string>
#include "util.h"

struct ImageInfo;


enum ImageZoomType
{
	ImageZoom_Default,
	// ImageZoom_Fit,
	// ImageZoom_FitScaleUp,
	ImageZoom_FitInView,
	ImageZoom_Free,
};


enum ImageLoadState : char
{
	ImageLoadState_Error = -1,
	ImageLoadState_NotLoaded,
	ImageLoadState_Loading,
	ImageLoadState_Finished,
};


struct ImageLoadThreadData_t
{
	fs::path            aPath;
	ImageInfo*          aInfo = nullptr;  // check to see if this is valid for if it's done loading
	std::vector< char > aData;
	ImageLoadState      aState = ImageLoadState_NotLoaded;
};


// TODO:
// - create an image thumbnail cache
// - store data for up to X amount of images to stay loaded in memory when switching
// - allow images to move off canvas, and be able to be grabbed anywhere
// - implement better scaling of images at lower resolutions
// - maybe for higher resolutions with an option of disabling some filter above a certain zoom level

void            ImageLoadThread_AddTask( ImageLoadThreadData_t* srData );
void            ImageLoadThread_RemoveTask( ImageLoadThreadData_t* srData );

void            ImageView_Update();
void            ImageView_Draw();
void            ImageView_Shutdown();

void            ImageView_SetImage( const fs::path& path );
void            ImageView_RemoveImage();
bool            ImageView_HasImage();
void            ImageView_DeleteImage();
const fs::path& ImageView_GetImagePath();

void            ImageView_FitInView( bool sScaleUp = false );
double          ImageView_GetZoomLevel();
void            ImageView_SetZoomLevel( double level );
void            ImageView_ResetZoom();

void            ImageView_ResetRotation();
float           ImageView_GetRotation();
void            ImageView_SetRotation( float rotation );

// ImageZoomType ImageView_GetZoomType();
// ImageZoomType ImageView_SetZoomType();

