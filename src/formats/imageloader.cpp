#include "imageloader.h"
#include "../util.h"


// TODO: maybe create some kind of plugin system for image loaders?
// and create some sort of image loader priority system that users can reorder if custom plugins are used


ImageLoader& GetImageLoader()
{
	static ImageLoader loader;
	return loader;
}


ImageData* ImageLoader::LoadImage( const std::string& srPath )
{
	// HACK HACK
	std::string path = srPath;

	if ( srPath.starts_with( "file:///" ) )
		path = srPath.substr( 8, srPath.length() - 8 );

	if ( !fs_file_exists( path ) )
	{
		printf( "[ImageLoader] Image does not exist: \"%s\"\n", path.c_str() );
		return nullptr;
	}

	std::string fileExt = fs_get_file_ext( path );
	if ( fileExt.empty() )
	{
		printf( "[ImageLoader] Failed to get file extension: - %s\n", path.c_str() );
		return nullptr;
	}

	for ( auto format: aFormats )
	{
		if ( !format->CheckExt( fileExt ) )
			continue;

		if ( auto image = format->LoadImage( path ) )
		{
			printf( "[ImageLoader] Loaded Image: \"%s\"\n", path.c_str() );
			return image;
		}
	}

	printf( "[ImageLoader] Failed to load image: \"%s\"\n", path.c_str() );
	return nullptr;
}


bool ImageLoader::CheckExt( const std::string& ext )
{
	for ( auto format: aFormats )
	{
		if ( format->CheckExt( ext ) )
			return true;
	}

	return false;
}


void ImageLoader::RegisterFormat( IImageFormat* spFormat )
{
	aFormats.push_back( spFormat );
}


