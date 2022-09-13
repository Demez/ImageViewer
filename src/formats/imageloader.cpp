#include "imageloader.h"
#include "../util.h"


// TODO: maybe create some kind of plugin system for image loaders?
// and create some sort of image loader priority system that users can reorder if custom plugins are used

std::vector< IImageFormat* > gFormats;


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
		if ( !format->CheckExt( fileExt.c_str() ) )
			continue;

		if ( auto image = format->LoadImage( srPath, srData ) )
		{
			wprintf( L"[ImageLoader] Loaded Image: \"%s\"\n", srPath.c_str() );
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

