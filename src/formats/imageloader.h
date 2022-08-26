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


class IImageFormat
{
public:
	virtual ImageInfo* LoadImage( const fs::path& path, std::vector< char >& srData ) = 0;
	virtual bool       CheckExt( std::wstring_view ext )   = 0;
};


ImageInfo* ImageLoader_LoadImage( const fs::path& path, std::vector< char >& srData );
bool       ImageLoader_SupportsImage( const fs::path& path );
bool       ImageLoader_SupportsImageExt( const fs::path& ext );
void       ImageLoader_RegisterFormat( IImageFormat* spFormat );



