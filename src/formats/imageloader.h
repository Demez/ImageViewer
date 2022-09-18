#pragma once

#include <string>
#include <vector>
#include "util.h"


enum PixelFormat
{
	FMT_NONE,

	FMT_RGB8,
	FMT_RGBA8,

	FMT_BGR8,
	FMT_BGRA8,

	COUNT,
};


struct ImageInfo
{
	int                 aWidth;
	int                 aHeight;
	int                 aBitDepth;
	// int                 aPitch;
	PixelFormat         aFormat = FMT_NONE;
};


struct ImageData
{
	std::vector< std::vector< char > > aData;
};


// TODO:
// - add a header check function
// - add a struct for image data to support animations
class IImageFormat
{
public:
	virtual ImageInfo* LoadImage( const fs::path& path, std::vector< char >& srData ) = 0;
	virtual bool       CheckExt( std::wstring_view ext )   = 0;
	// virtual bool       CheckHeader( const fs::path& path )   = 0;
};


void       ImageLoader_RegisterFormat( IImageFormat* spFormat );
ImageInfo* ImageLoader_LoadImage( const fs::path& path, std::vector< char >& srData );
bool       ImageLoader_SupportsImage( const fs::path& path );
bool       ImageLoader_SupportsImageExt( const fs::path& ext );
bool       ImageLoader_ConvertFormat( std::vector< char >& srData, PixelFormat srcFmt, PixelFormat dstFmt );

// ideas
// 
// using DImageHandle = size_t;
// 
// void       ImageLoader_GetDimensions( DImageHandle img, int& srWidth, int& srHeight );
// void       ImageLoader_GetPitch( DImageHandle img );
// void       ImageLoader_GetColorType( DImageHandle img );

