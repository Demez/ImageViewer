#include "render.h"
#include "platform.h"
#include "formats/imageloader.h"

#include "imgui_impl_win32.h"
#include "imgui_impl_sdlrenderer.h"

#include <SDL.h>

#include <unordered_map>


SDL_Renderer* gRenderer = nullptr;
SDL_Window*   gWindow   = nullptr;

// std::unordered_map< ImageInfo*, SDL_Surface* > gImageTextures;
std::unordered_map< ImageInfo*, SDL_Texture* > gImageTextures;


bool Render_Init()
{
	SDL_Init( SDL_INIT_VIDEO );

	void* window = Plat_GetWindow();
	if ( window == nullptr )
	{
		printf( "Plat_GetWindow returned nullptr?\n" );
		return false;
	}

	if ( !( gWindow = SDL_CreateWindowFrom( window ) ) )
	{
		printf( "Failed to create SDL_Window from HWDN: %s\n", SDL_GetError() );
		return false;
	}
	
	if ( !(gRenderer = SDL_CreateRenderer( gWindow, -1, SDL_RENDERER_ACCELERATED )) )
	{
		printf( "Failed to create SDL_Renderer: %s\n", SDL_GetError() );
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
	// SDL_DestroyWindow( gWindow );
}


void Render_NewFrame()
{
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


bool Render_LoadImage( ImageInfo* spInfo, std::vector< char >& srData )
{
	auto find = gImageTextures.find( spInfo );
	
	// Image is already loaded
	if ( find != gImageTextures.end() )
		return true;

	int                 pitch  = spInfo->aWidth;
	SDL_PixelFormatEnum sdlFmt = SDL_PIXELFORMAT_ABGR8888;

	if ( spInfo->aFormat == FMT_RGB8 )
	{
		sdlFmt = SDL_PIXELFORMAT_RGB24;
		pitch *= 3;
	}
	else if ( spInfo->aFormat == FMT_BGR8 )
	{
		sdlFmt = SDL_PIXELFORMAT_BGR24;
		pitch *= 3;
	}
	else
	{
		pitch *= 4;
	}

	SDL_Surface* surf = SDL_CreateRGBSurfaceWithFormatFrom(
		srData.data(), 
		spInfo->aWidth,
		spInfo->aHeight,
		spInfo->aBitDepth,
		pitch,
		sdlFmt
	);

	if ( surf == nullptr )
		return false;

	SDL_Texture* tex = SDL_CreateTextureFromSurface( gRenderer, surf );
	SDL_FreeSurface( surf );

	if ( tex == nullptr )
	{
		printf( "[Render] Failed to create texture from surface: %s\n", SDL_GetError() );
		return false;
	}

	// gImageTextures[ spInfo ] = surf;
	gImageTextures[ spInfo ] = tex;

	return true;
}


void Render_FreeImage( ImageInfo* spInfo )
{
	auto find = gImageTextures.find( spInfo );

	// Image is not loaded
	if ( find == gImageTextures.end() )
		return;

	SDL_DestroyTexture( find->second );

	gImageTextures.erase( spInfo );
}


void Render_DrawImage( ImageInfo* spInfo, const ImageDrawInfo& srDrawInfo )
{
	auto find = gImageTextures.find( spInfo );

	// Image is already loaded
	if ( find == gImageTextures.end() )
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
}

