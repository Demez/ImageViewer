#include "render_sdl/render_sdl.h"
#include "ui/imageview.h"

#include <assert.h>
#include <stdio.h>
#include <vector>

#define SDL_MAIN_HANDLED

#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_sdlrenderer.h"

#include <SDL.h>

int main( int argc, char* argv[] )
{
	ImGui::CreateContext();

	if ( !Render_Init() )
	{
		printf( "Failed to init Renderer!!\n" );
		ImGui::DestroyContext();
		return 1;
	}

	// enable drag and drop in SDL2 (not very good as it just accepts everything, hmm)
	SDL_EventState( SDL_DROPFILE, SDL_ENABLE );

	bool                     running = true;
	std::vector< SDL_Event > aEvents;

	ImGuiIO&                 io = ImGui::GetIO();

	if ( argc > 1 )
	{
		ImageView_SetImage( argv[ 1 ] );
	}

	while ( running )
	{
		SDL_PumpEvents();

		// get total event count first
		int eventCount = SDL_PeepEvents( nullptr, 0, SDL_PEEKEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT );

		// resize event vector with events found (probably bad to do every frame?)
		aEvents.resize( eventCount );

		// fill event vector with events found
		SDL_PeepEvents( aEvents.data(), eventCount, SDL_GETEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT );

		for ( int i = 0; i < eventCount; i++ )
		{
			SDL_Event& aEvent = aEvents[ i ];
			ImGui_ImplSDL2_ProcessEvent( &aEvent );
			ImageView_HandleEvent( aEvent );

			switch ( aEvent.type )
			{
				case SDL_QUIT:
				{
					running = false;
					break;
				}

				case SDL_DROPBEGIN:
				{
					printf( "drop begin\n" );
					break;
				}

				case SDL_DROPFILE:
				{
					ImageView_SetImage( aEvent.drop.file );
					printf( "drop file\n" );
					break;
				}
			}
		}

		Render_NewFrame();
		
		ImageView_Draw();

		// ----------------------------------------------------------------------
		// UI Building

		ImGui::NewFrame();

		// Zoom Display
		char buf[ 16 ] = { '\0' };
		snprintf( buf, 16, "%.0f%%\0", ImageView_GetZoomLevel() * 100 );

		ImGui::Begin( "Zoom Level", nullptr, ImGuiWindowFlags_NoTitleBar );
		ImGui::TextUnformatted( buf );

		if ( ImGui::Button( "Fit" ) )
		{
			ImageView_FitInView();
		}

		if ( ImGui::Button( "100%" ) )
		{
			ImageView_ResetZoom();
		}

		ImGui::End();

		// Temp
		// ImGui::ShowDemoWindow();

		// ----------------------------------------------------------------------
		// Rendering

		Render_Draw();

		SDL_Delay( 1 );
	}

	Render_Shutdown();

	ImGui::DestroyContext();

	return 0;
}
