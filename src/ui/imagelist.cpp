#include "imagelist.h"
#include "util.h"
#include "formats/imageloader.h"
#include "ui/imageview.h"

#include <SDL.h>


static fs::path                gCurrentDir;
static std::vector< fs::path > gImages;
static size_t                  gIndex;
static FileSort                gSortMode;


struct ImageListElement
{
};



void ImageList_Update()
{
}


void ImageList_SetSortMode( FileSort sortMode )
{
	if ( sortMode > FileSort_Count )
	{
		printf( "ImageList: Warning - tried to set an invalid file sort mode\n" );
		return;
	}

	gSortMode = sortMode;
}


FileSort ImageList_GetSortMode()
{
	return gSortMode;
}


void ImageList_SetPathFromFile( const fs::path& srFile )
{
	// ImageList_SetPath( fs_get_dir_name( srFile ) );
	ImageList_SetPath( srFile.parent_path() );

	// find index of the image we have loaded
	gIndex = vec_index( gImages, srFile );

	if ( gIndex == SIZE_MAX )
	{
		ImageList_LoadFiles();
		gIndex = vec_index( gImages, srFile );
	}
}


void ImageList_SetPath( const fs::path& srPath )
{
	fs::path path = fs_clean_path( srPath );

	if ( gCurrentDir.empty() )
		gCurrentDir = path;

	else if ( gCurrentDir == path )
		return;

	gCurrentDir = path;

	ImageList_LoadFiles();
}


bool ImageList_InFolder()
{
	return !gCurrentDir.empty();
}


void ImageList_LoadFiles()
{
	if ( gCurrentDir.empty() )
		return;

	wprintf( L"ImageList: Loading files in directory: %s\n", gCurrentDir.c_str() );

	gImages.clear();

	// TODO: maybe move this somewhere else later, for win32 folder view sorting support
	fs::path    file;
	DirHandle_t dir = fs_read_first( gCurrentDir, file, ReadDir_AbsPaths );

	if ( dir == nullptr )
	{
		printf( "uh\n" );
	}

	bool search = true;

	while ( search )
	{
		if ( fs_is_file( file.c_str() ) && ImageLoader_SupportsImage( file ) )
		{
			gImages.push_back( file );
		}

		search = fs_read_next( dir, file );
	}

	fs_read_close( dir );

	wprintf( L"ImageList: Finished Loading files in directory: %s\n", gCurrentDir.c_str() );
}


void ImageList_SortFiles()
{
}


bool ImageList_LoadPrevImage()
{
	if ( gImages.empty() )
		return false;

	if ( gImages.size() == 1 )
		return false;

	if ( gIndex == 0 )
		gIndex = gImages.size()-1;
	else
		gIndex--;

	ImageView_SetImage( gImages[ gIndex ] );
	return true;
}


bool ImageList_LoadNextImage()
{
	if ( gImages.empty() )
		return false;

	if ( gImages.size() == 1 )
		return false;

	gIndex++;

	if ( gIndex >= gImages.size() )
		gIndex = 0;

	ImageView_SetImage( gImages[ gIndex ] );
	return true;
}


void ImageList_RemoveItem( const fs::path& srFile )
{
	size_t index = vec_index( gImages, srFile );

	if ( index == SIZE_MAX )
	{
		wprintf( L"ImageList: File not in image list: %s\n", srFile.c_str() );
		return;
	}

	vec_remove_index( gImages, index );
}

