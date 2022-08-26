#pragma once

#include <string>
#include "util.h"

struct ImageInfo;
typedef union SDL_Event;


enum ImageZoomType
{
	ImageZoom_Default,
	ImageZoom_Fit,
	ImageZoom_FitScaleUp,
	ImageZoom_Free,
};


// TODO:
// - create an image thumbnail cache
// - store data for up to X amount of images to stay loaded in memory when switching
// - allow images to move off canvas, and be able to be grabbed anywhere
// - implement better scaling of images at lower resolutions
// - maybe for higher resolutions with an option of disabling some filter above a certain zoom level

// might remove this HandleEvent later if i decide to try win32 api
void            ImageView_HandleEvent( SDL_Event& srEvent );
void            ImageView_Draw();

bool            ImageView_SetImage( const fs::path& path );
void            ImageView_RemoveImage();
bool            ImageView_HasImage();
const fs::path& ImageView_GetImagePath();

void            ImageView_FitInView( bool sScaleUp = false );
double          ImageView_GetZoomLevel();
void            ImageView_SetZoomLevel( double level );
void            ImageView_ResetZoom();

// ImageZoomType ImageView_GetZoomType();
// ImageZoomType ImageView_SetZoomType();

