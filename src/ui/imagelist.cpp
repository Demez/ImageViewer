#include "imagelist.h"
#include "util.h"
#include "formats/imageloader.h"
#include "ui/imageview.h"

#include <SDL.h>


static fs::path                gCurrentDir;
static std::vector< fs::path > gImages;
static size_t                  gIndex;


struct ImageListElement
{
};


void ImageList_HandleEvent( SDL_Event& srEvent )
{
	switch ( srEvent.type )
	{
		case SDL_KEYDOWN:
		{
			if ( srEvent.key.keysym.sym == SDLK_RIGHT )
			{
				ImageList_LoadNextImage();
			}
			else if ( srEvent.key.keysym.sym == SDLK_LEFT )
			{
				ImageList_LoadPrevImage();
			}

			return;
		}
	}
}


void ImageList_Update()
{
	// const Uint8* keyboardState = SDL_GetKeyboardState( nullptr );
}


void ImageList_SetPathFromFile( const fs::path& srFile )
{
	// ImageList_SetPath( fs_get_dir_name( srFile ) );
	ImageList_SetPath( srFile.parent_path() );

	// find index of the image we have loaded
	gIndex = vec_index( gImages, srFile );

	if ( gIndex == SIZE_MAX )
	{
		printf( "what the FUCK\n" );
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


void ImageList_LoadFiles()
{
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
}


void ImageList_LoadPrevImage()
{
	if ( gImages.empty() )
		return;

	if ( gImages.size() == 1 )
		return;

	if ( gIndex == 0 )
		gIndex = gImages.size()-1;
	else
		gIndex--;

	ImageView_SetImage( gImages[ gIndex ] );
}


void ImageList_LoadNextImage()
{
	if ( gImages.empty() )
		return;

	if ( gImages.size() == 1 )
		return;

	gIndex++;

	if ( gIndex >= gImages.size() )
		gIndex = 0;

	ImageView_SetImage( gImages[ gIndex ] );
}


