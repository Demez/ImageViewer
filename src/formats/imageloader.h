#pragma once

#include <string>
#include <vector>


enum PixelFormat
{
	FMT_NONE,

	FMT_RGB8,
	FMT_RGB16,

	FMT_RGBA8,
	FMT_RGBA16,

	COUNT,
};


struct ImageData
{
	std::vector< char > aData;
	int                 aWidth;
	int                 aHeight;
	int                 aBitDepth;
	// int                 aPitch;
	PixelFormat         aFormat = FMT_NONE;
};


class IImageFormat
{
public:
	virtual ImageData* LoadImage( const std::string& path ) = 0;
	virtual bool       CheckExt( const std::string& ext )   = 0;
};


class ImageLoader
{
public:
	ImageData*                   LoadImage( const std::string& path );
	bool                         CheckExt( const std::string& ext );
	void                         RegisterFormat( IImageFormat* spFormat );

	std::vector< IImageFormat* > aFormats;
};


ImageLoader& GetImageLoader();

