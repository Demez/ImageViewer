#include "imageloader.h"
#include "../util.h"

#include <chrono>


// TODO: maybe create some kind of plugin system for image loaders?
// and create some sort of image loader priority system that users can reorder if custom plugins are used

std::vector< IImageFormat* > gFormats;


void ConvertRGB8ToBGR8( std::vector< char >& srData )
{
	char temp;
	for ( int i = 0; i < srData.size(); i += 3 )
	{
		// swap R and B; raw_image[i + 1] is G, so it stays where it is.
		temp            = srData[ i + 0 ];
		srData[ i + 0 ] = srData[ i + 2 ];
		srData[ i + 2 ] = temp;
	}
}


void ConvertRGBA8ToBGRA8( std::vector< char >& srData )
{
	char temp;
	for ( int i = 0; i < srData.size(); i += 4 )
	{
		// swap R and B; raw_image[i + 1] is G, so it stays where it is.
		temp            = srData[ i + 0 ];
		srData[ i + 0 ] = srData[ i + 2 ];
		srData[ i + 2 ] = temp;
	}
}


void ImageLoader_RegisterFormat( IImageFormat* spFormat )
{
	gFormats.push_back( spFormat );
}


ImageInfo* ImageLoader_LoadImage( const fs::path& srPath, std::vector< char >& srData )
{
	if ( !fs_file_exists( srPath.c_str() ) )
	{
		wprintf( L"[ImageLoader] Image does not exist: \"%s\"\n", srPath.c_str() );
		return nullptr;
	}

	std::wstring fileExt = fs_get_file_ext( srPath );
	if ( fileExt.empty() )
	{
		wprintf( L"[ImageLoader] Failed to get file extension: - %s\n", srPath.c_str() );
		return nullptr;
	}

	// fs::path fileExt = srPath.extension();

	for ( auto format: gFormats )
	{
		// if ( !format->CheckExt( fileExt.c_str() ) )
		// 	continue;

		if ( !format->CheckHeader( srPath ) )
			continue;

		auto startTime = std::chrono::high_resolution_clock::now();

		if ( auto image = format->LoadImage( srPath, srData ) )
		{
			wprintf( L"[ImageLoader] Loaded Image: \"%s\"\n", srPath.c_str() );

			// BGRA is faster than RGBA on GPUs
			if ( image->aFormat == FMT_RGBA8 )
			{
				// ConvertRGBA8ToBGRA8( srData );
				// image->aFormat = FMT_BGRA8;
			}

			auto  currentTime = std::chrono::high_resolution_clock::now();
			float time = std::chrono::duration< float, std::chrono::seconds::period >( currentTime - startTime ).count();

			printf( "ImageLoader_LoadImage(): Time taken to load image: %.6f\n", time );
			return image;
		}
	}

	wprintf( L"[ImageLoader] Failed to load image: \"%s\"\n", srPath.c_str() );
	return nullptr;
}


bool ImageLoader_SupportsImage( const fs::path& path )
{
	std::wstring ext = fs_get_file_ext( path );
	// if ( !path.has_extension() )
	// 	return false;
	// 
	// fs::path ext = path.extension();

	if ( ext.empty() )
		return false;

	return ImageLoader_SupportsImageExt( ext );
}


bool ImageLoader_SupportsImageExt( const fs::path& ext )
{
	for ( auto format: gFormats )
	{
		if ( format->CheckExt( ext.wstring() ) )
			return true;
	}

	return false;
}


bool ImageLoader_ConvertFormat( std::vector< char >& srData, PixelFormat srcFmt, PixelFormat dstFmt )
{
	// lazy
	if ( srcFmt == FMT_RGB8 && dstFmt == FMT_BGR8 )
	{
		ConvertRGB8ToBGR8( srData );
		return true;
	}
	else if ( srcFmt == FMT_RGBA8 && dstFmt == FMT_BGRA8 )
	{
		ConvertRGBA8ToBGRA8( srData );
		return true;
	}

	return false;
}

