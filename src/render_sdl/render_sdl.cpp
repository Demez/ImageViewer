#include "render_sdl.h"
#include "formats/imageloader.h"

#include "imgui_impl_sdl.h"
#include "imgui_impl_sdlrenderer.h"

#include <SDL.h>

#include <unordered_map>
#include "resources.hpp"


SDL_Window*   gWindow      = nullptr;
SDL_Renderer* gRenderer    = nullptr;
SDL_Surface*  gScreenSurface = nullptr;

// std::unordered_map< ImageData*, SDL_Surface* > gImages;
std::unordered_map< ImageData*, SDL_Texture* > gImages;


bool Render_Init()
{
	SDL_Init( SDL_INIT_VIDEO | SDL_INIT_EVENTS );

	gWindow = SDL_CreateWindow( "demez imgui image viewer", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1280, 720, SDL_WINDOW_RESIZABLE );
	if ( !gWindow )
	{
		printf( "Failed to create window: %s\n", SDL_GetError() );
		return false;
	}

	gScreenSurface = SDL_GetWindowSurface( gWindow );

	if ( !(gRenderer = SDL_CreateRenderer( gWindow, -1, SDL_RENDERER_ACCELERATED )) )
	{
		printf( "Failed to create SDL_Renderer: %s\n", SDL_GetError() );
		return false;
	}

	if ( !ImGui_ImplSDL2_InitForSDLRenderer( gWindow, gRenderer ) )
	{
		fputs( "Failed to init ImGui for SDL_Renderer\n", stderr );
		return false;
	}

	if ( !ImGui_ImplSDLRenderer_Init( gRenderer ) )
	{
		fputs( "Failed to init ImGui SDL_Renderer\n", stderr );
		return false;
	}

	return true;
}


void Render_Shutdown()
{
	SDL_DestroyRenderer( gRenderer );
	SDL_DestroyWindow( gWindow );
}


void Render_NewFrame()
{
	ImGui_ImplSDL2_NewFrame();
	ImGui_ImplSDLRenderer_NewFrame();

	SDL_SetRenderDrawColor( gRenderer, 48, 48, 48, 255 );
	SDL_RenderClear( gRenderer );
}


void Render_Draw()
{
	ImGui::Render();
	ImGui_ImplSDLRenderer_RenderDrawData( ImGui::GetDrawData() );

	SDL_RenderPresent( gRenderer );
}


bool Render_LoadImage( ImageData* spData )
{
	auto find = gImages.find( spData );
	
	// Image is already loaded
	if ( find != gImages.end() )
		return true;

	SDL_Surface* surf = SDL_CreateRGBSurfaceWithFormatFrom(
		spData->aData.data(), 
		spData->aWidth,
		spData->aHeight,
		spData->aBitDepth,

		// TODO: actually do this properly
		spData->aWidth * 4,
		SDL_PIXELFORMAT_ABGR8888
	);

	if ( surf == nullptr )
		return false;

	SDL_Texture* tex = SDL_CreateTextureFromSurface( gRenderer, surf );

	if ( tex == nullptr )
	{
		printf( "[Render] Failed to create texture from surface: %s\n", SDL_GetError() );
		SDL_FreeSurface( surf );
		return false;
	}

	// gImages[ spData ] = surf;
	gImages[ spData ] = tex;

	SDL_FreeSurface( surf );

	return true;
}


void Render_FreeImage( ImageData* spData )
{
	auto find = gImages.find( spData );

	// Image is not loaded
	if ( find == gImages.end() )
		return;

	// SDL_Surface* surf = find->second;

	// SDL_FreeSurface( surf );

	// uh remove from gImages???
}


void Render_DrawImage( ImageData* spData, const ImageDrawInfo& srDrawInfo )
{
	auto find = gImages.find( spData );

	// Image is already loaded
	if ( find == gImages.end() )
		return;

	// SDL_Surface* surf = find->second;
	SDL_Texture* tex = find->second;

	SDL_Rect dstRect{
		.x = (int)round(srDrawInfo.aX),
		.y = (int)round(srDrawInfo.aY),
		.w = srDrawInfo.aWidth,
		.h = srDrawInfo.aHeight
	};

	SDL_RenderCopy( gRenderer, tex, nullptr, &dstRect );

	// SDL_UpperBlit( surf, nullptr, gScreenSurface, nullptr );

	// SDL_UpdateWindowSurface( gWindow );
}


void Render_GetWindowSize( int& srWidth, int& srHeight )
{
	SDL_GetWindowSize( gWindow, &srWidth, &srHeight );
}

