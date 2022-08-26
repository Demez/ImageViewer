#include "util.h"
#include "render_sdl/render_sdl.h"
#include "ui/imageview.h"
#include "ui/imagelist.h"

#include <assert.h>
#include <stdio.h>
#include <vector>

#define SDL_MAIN_HANDLED

#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_sdlrenderer.h"

#include <SDL.h>


void StyleImGui()
{
	auto&   io                               = ImGui::GetIO();

	// gBuiltInFont = BuildFont( "fonts/CascadiaMono.ttf" );

	// Classic VGUI2 Style Color Scheme
	ImVec4* colors                           = ImGui::GetStyle().Colors;

	colors[ ImGuiCol_Text ]                  = ImVec4( 1.00f, 1.00f, 1.00f, 1.00f );
	colors[ ImGuiCol_TextDisabled ]          = ImVec4( 0.50f, 0.50f, 0.50f, 1.00f );
	colors[ ImGuiCol_WindowBg ]              = ImVec4( 0.29f, 0.34f, 0.26f, 1.00f );
	colors[ ImGuiCol_ChildBg ]               = ImVec4( 0.29f, 0.34f, 0.26f, 1.00f );
	colors[ ImGuiCol_PopupBg ]               = ImVec4( 0.24f, 0.27f, 0.20f, 1.00f );
	colors[ ImGuiCol_Border ]                = ImVec4( 0.54f, 0.57f, 0.51f, 0.50f );
	colors[ ImGuiCol_BorderShadow ]          = ImVec4( 0.14f, 0.16f, 0.11f, 0.52f );
	colors[ ImGuiCol_FrameBg ]               = ImVec4( 0.24f, 0.27f, 0.20f, 1.00f );
	colors[ ImGuiCol_FrameBgHovered ]        = ImVec4( 0.27f, 0.30f, 0.23f, 1.00f );
	colors[ ImGuiCol_FrameBgActive ]         = ImVec4( 0.30f, 0.34f, 0.26f, 1.00f );
	colors[ ImGuiCol_TitleBg ]               = ImVec4( 0.24f, 0.27f, 0.20f, 1.00f );
	colors[ ImGuiCol_TitleBgActive ]         = ImVec4( 0.29f, 0.34f, 0.26f, 1.00f );
	colors[ ImGuiCol_TitleBgCollapsed ]      = ImVec4( 0.00f, 0.00f, 0.00f, 0.51f );
	colors[ ImGuiCol_MenuBarBg ]             = ImVec4( 0.24f, 0.27f, 0.20f, 1.00f );
	colors[ ImGuiCol_ScrollbarBg ]           = ImVec4( 0.35f, 0.42f, 0.31f, 1.00f );
	colors[ ImGuiCol_ScrollbarGrab ]         = ImVec4( 0.28f, 0.32f, 0.24f, 1.00f );
	colors[ ImGuiCol_ScrollbarGrabHovered ]  = ImVec4( 0.25f, 0.30f, 0.22f, 1.00f );
	colors[ ImGuiCol_ScrollbarGrabActive ]   = ImVec4( 0.23f, 0.27f, 0.21f, 1.00f );
	colors[ ImGuiCol_CheckMark ]             = ImVec4( 0.59f, 0.54f, 0.18f, 1.00f );
	colors[ ImGuiCol_SliderGrab ]            = ImVec4( 0.35f, 0.42f, 0.31f, 1.00f );
	colors[ ImGuiCol_SliderGrabActive ]      = ImVec4( 0.54f, 0.57f, 0.51f, 0.50f );
	colors[ ImGuiCol_Button ]                = ImVec4( 0.29f, 0.34f, 0.26f, 0.40f );
	colors[ ImGuiCol_ButtonHovered ]         = ImVec4( 0.35f, 0.42f, 0.31f, 1.00f );
	colors[ ImGuiCol_ButtonActive ]          = ImVec4( 0.54f, 0.57f, 0.51f, 0.50f );
	colors[ ImGuiCol_Header ]                = ImVec4( 0.35f, 0.42f, 0.31f, 1.00f );
	colors[ ImGuiCol_HeaderHovered ]         = ImVec4( 0.35f, 0.42f, 0.31f, 0.6f );
	colors[ ImGuiCol_HeaderActive ]          = ImVec4( 0.54f, 0.57f, 0.51f, 0.50f );
	colors[ ImGuiCol_Separator ]             = ImVec4( 0.14f, 0.16f, 0.11f, 1.00f );
	colors[ ImGuiCol_SeparatorHovered ]      = ImVec4( 0.54f, 0.57f, 0.51f, 1.00f );
	colors[ ImGuiCol_SeparatorActive ]       = ImVec4( 0.59f, 0.54f, 0.18f, 1.00f );
	colors[ ImGuiCol_ResizeGrip ]            = ImVec4( 0.19f, 0.23f, 0.18f, 0.00f );  // grip invis
	colors[ ImGuiCol_ResizeGripHovered ]     = ImVec4( 0.54f, 0.57f, 0.51f, 1.00f );
	colors[ ImGuiCol_ResizeGripActive ]      = ImVec4( 0.59f, 0.54f, 0.18f, 1.00f );
	colors[ ImGuiCol_Tab ]                   = ImVec4( 0.35f, 0.42f, 0.31f, 1.00f );
	colors[ ImGuiCol_TabHovered ]            = ImVec4( 0.54f, 0.57f, 0.51f, 0.78f );
	colors[ ImGuiCol_TabActive ]             = ImVec4( 0.59f, 0.54f, 0.18f, 1.00f );
	colors[ ImGuiCol_TabUnfocused ]          = ImVec4( 0.24f, 0.27f, 0.20f, 1.00f );
	colors[ ImGuiCol_TabUnfocusedActive ]    = ImVec4( 0.35f, 0.42f, 0.31f, 1.00f );
	colors[ ImGuiCol_DockingPreview ]        = ImVec4( 0.59f, 0.54f, 0.18f, 1.00f );
	colors[ ImGuiCol_DockingEmptyBg ]        = ImVec4( 0.20f, 0.20f, 0.20f, 1.00f );
	colors[ ImGuiCol_PlotLines ]             = ImVec4( 0.61f, 0.61f, 0.61f, 1.00f );
	colors[ ImGuiCol_PlotLinesHovered ]      = ImVec4( 0.59f, 0.54f, 0.18f, 1.00f );
	colors[ ImGuiCol_PlotHistogram ]         = ImVec4( 1.00f, 0.78f, 0.28f, 1.00f );
	colors[ ImGuiCol_PlotHistogramHovered ]  = ImVec4( 1.00f, 0.60f, 0.00f, 1.00f );
	colors[ ImGuiCol_TextSelectedBg ]        = ImVec4( 0.59f, 0.54f, 0.18f, 1.00f );
	colors[ ImGuiCol_DragDropTarget ]        = ImVec4( 0.73f, 0.67f, 0.24f, 1.00f );
	colors[ ImGuiCol_NavHighlight ]          = ImVec4( 0.59f, 0.54f, 0.18f, 1.00f );
	colors[ ImGuiCol_NavWindowingHighlight ] = ImVec4( 1.00f, 1.00f, 1.00f, 0.70f );
	colors[ ImGuiCol_NavWindowingDimBg ]     = ImVec4( 0.80f, 0.80f, 0.80f, 0.20f );
	colors[ ImGuiCol_ModalWindowDimBg ]      = ImVec4( 0.80f, 0.80f, 0.80f, 0.35f );

	ImGuiStyle& style                        = ImGui::GetStyle();
	style.FrameBorderSize                    = 1.0f;
	style.WindowRounding                     = 0.0f;
	style.ChildRounding                      = 0.0f;
	style.FrameRounding                      = 0.0f;
	style.PopupRounding                      = 0.0f;
	style.ScrollbarRounding                  = 0.0f;
	style.GrabRounding                       = 0.0f;
	style.TabRounding                        = 0.0f;
}


int main( int argc, char* argv[] )
{
	ImGui::CreateContext();

	StyleImGui();

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
			ImageList_HandleEvent( aEvent );

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
					ImageView_SetImage( sys_to_wstr( aEvent.drop.file ) );
					printf( "drop file\n" );
					break;
				}
			}
		}

		Render_NewFrame();
		
		ImageList_Update();
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

		// Context Menu
		if ( ImGui::BeginPopupContextVoid( "main ctx menu" ) )
		{
			if ( ImGui::MenuItem( "Open File Location", nullptr, false, ImageView_HasImage() ) )
			{
				sys_browse_to_file( ImageView_GetImagePath() );
			}

			ImGui::EndPopup();
		}

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
