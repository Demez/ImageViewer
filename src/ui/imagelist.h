#pragma once

#include <string>
#include "util.h"

union SDL_Event;

void ImageList_HandleEvent( SDL_Event& srEvent );
void ImageList_Update();

void ImageList_SetPathFromFile( const fs::path& srFile );
void ImageList_SetPath( const fs::path& srPath );
void ImageList_LoadFiles();

void ImageList_LoadPrevImage();
void ImageList_LoadNextImage();

