#include "imagelist.h"
#include "platform.h"
#include "util.h"
#include "formats/imageloader.h"
#include "ui/imageview.h"

#include <map>


static fs::path                gCurrentDir;
static std::vector< fs::path > gImages;
static size_t                  gIndex;

static FileSort                          gSortMode = FileSort_DateModNewest;
std::map< fs::path, fs::file_time_type > gDateMap;

extern fs::path                gImagePath;


struct ImageListElement
{
};



void ImageList_Update()
{
	// check folder monitor for if a change happened
	if ( !Plat_FolderMonitorChanged() )
		return;

	// something changed, reload files
	ImageList_LoadFiles();

	// OPTIONAL:
	// checks if the image currently loaded was deleted,
	// and automatically goes to the next image if so

	size_t newIndex = vec_index( gImages, gImagePath );

	// file we had loaded was deleted
	if ( newIndex == SIZE_MAX )
	{
		// are we now outside the total image count?
		if ( gIndex > gImages.size() )
		{
			gIndex = gImages.size()-1;
			ImageView_SetImage( gImages[ gIndex ] );
		}
		else
		{
			// nope, load whatever image fell into that slot now
			ImageView_SetImage( gImages[ gIndex ] );
		}
	}
	else
	{
		// update our image index
		gIndex = newIndex;
	}
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

	Plat_FolderMonitorSetPath( path );

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

	ImageList_SortFiles();
}


int qsort_date_mod_newest( const void* spLeft , const void* spRight )
{
	const auto* pathX = (fs::path*)spLeft;
	const auto* pathY = (fs::path*)spRight;

	const auto& x     = gDateMap.at( *pathX );
	const auto& y     = gDateMap.at( *pathY );

	if ( x > y )
		return -1;
	else if ( x < y )
		return 1;

	return 0;
}


int qsort_date_mod_oldest( const void* spLeft, const void* spRight )
{
	const auto* pathX = (fs::path*)spLeft;
	const auto* pathY = (fs::path*)spRight;

	const auto& x     = gDateMap.at( *pathX );
	const auto& y     = gDateMap.at( *pathY );

	if ( x > y )
		return 1;
	else if ( x < y )
		return -1;

	return 0;
}


void ImageList_SortFiles()
{
	switch ( gSortMode )
	{
		default:
		{
			printf( "Unimplemented Sort Mode!\n" );
			break;
		}

		case FileSort_None:
			return;

		case FileSort_DateModNewest:
		{
			for ( auto& file : gImages )
				gDateMap[ file ] = fs::last_write_time( file );

			std::qsort( gImages.data(), gImages.size(), sizeof( fs::path ), qsort_date_mod_newest );
			break;
		}

		case FileSort_DateModOldest:
		{
			for ( auto& file : gImages )
				gDateMap[ file ] = fs::last_write_time( file );

			std::qsort( gImages.data(), gImages.size(), sizeof( fs::path ), qsort_date_mod_oldest );
			break;
		}
	}
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

