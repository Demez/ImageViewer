#include "main.h"
#include "util.h"
#include "args.h"
#include "render.h"
#include "ui/imageview.h"
#include "ui/imagelist.h"
// #include "ui/settings.h"

#include <assert.h>
#include <stdio.h>
#include <vector>

#include "imgui.h"


Module gRenderer = 0;


// uhh where tf do i put this
Render_Init_t          Render_Init          = 0;
Render_Shutdown_t      Render_Shutdown      = 0;

Render_NewFrame_t      Render_NewFrame      = 0;
Render_DrawImGui_t     Render_DrawImGui     = 0;
Render_Present_t       Render_Present       = 0;

Render_SetResolution_t Render_SetResolution = 0;
Render_SetClearColor_t Render_SetClearColor = 0;
Render_GetClearColor_t Render_GetClearColor = 0;

Render_LoadImage_t     Render_LoadImage     = 0;
Render_FreeImage_t     Render_FreeImage     = 0;
Render_DrawImage_t     Render_DrawImage     = 0;


#define LOAD_RENDER_FUNC( name ) \
	if ( !(name = (##name##_t)Plat_LoadFunc( gRenderer, #name )) ) \
	{                                                                \
		printf( "Failed To Load Render Function: \"%s\"\n", #name );\
		return false;\
	}


bool LoadRenderer()
{
	// printf( "Failed to Init Renderer!!\n" );

	if ( Args_Has( _T("-vk") ) )
	{
		if ( !( gRenderer = Plat_LoadLibrary( _T("render_vk") EXT_DLL ) ) )
		{
			printf( "Failed to Load Renderer!!\n" );
			return false;
		}
	}
	else
	{
		if ( !( gRenderer = Plat_LoadLibrary( _T("render_sdl") EXT_DLL ) ) )
		{
			printf( "Failed to Load Renderer!!\n" );
			return false;
		}
	}

	LOAD_RENDER_FUNC( Render_Init );
	LOAD_RENDER_FUNC( Render_Shutdown );

	LOAD_RENDER_FUNC( Render_NewFrame );
	LOAD_RENDER_FUNC( Render_DrawImGui );
	LOAD_RENDER_FUNC( Render_Present );

	LOAD_RENDER_FUNC( Render_SetResolution );
	LOAD_RENDER_FUNC( Render_SetClearColor );
	LOAD_RENDER_FUNC( Render_GetClearColor );

	LOAD_RENDER_FUNC( Render_LoadImage );
	LOAD_RENDER_FUNC( Render_FreeImage );
	LOAD_RENDER_FUNC( Render_DrawImage );

	return true;
}


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


bool gRunning                 = true;
bool gShouldDraw              = true;
bool gCanDraw                 = false;
bool gSettingsOpen            = false;
bool gFilePropertiesSupported = true;


void Main_ShouldDrawWindow( bool draw )
{
	gShouldDraw |= draw;
}


void Main_VoidContextMenu()
{
	if ( !ImGui::BeginPopupContextVoid( "main ctx menu" ) )
		return;

	if ( ImGui::MenuItem( "Open File Location", nullptr, false, ImageView_HasImage() ) )
	{
		Plat_BrowseToFile( ImageView_GetImagePath() );
	}

	if ( ImGui::MenuItem( "Copy Image", nullptr, false, false ) )
	{
	}

	if ( ImGui::MenuItem( "Copy Image Data", nullptr, false, false ) )
	{
	}

	if ( ImGui::MenuItem( "Set As Desktop Background", nullptr, false, false ) )
	{
	}

	if ( ImGui::MenuItem( "Undo", nullptr, false, Plat_CanUndo() ) )
	{
		Plat_Undo();
	}

	if ( ImGui::MenuItem( "Redo", nullptr, false, Plat_CanRedo() ) )
	{
		Plat_Redo();
	}

	if ( ImGui::MenuItem( "Delete", nullptr, false, ImageView_HasImage() ) )
	{
		ImageView_DeleteImage();
	}

	if ( ImGui::MenuItem( "File Info", nullptr, false, false ) )
	{
	}

	if ( ImGui::MenuItem( "File Properties", nullptr, false, ImageView_HasImage() ) )
	{
		// TODO: create our own imgui file properties for more info
		Plat_OpenFileProperties( ImageView_GetImagePath() );
	}

	ImGui::Separator();

	if ( ImGui::BeginMenu( "Sort Mode" ) )
	{
		ImGui::EndMenu();
	}

	if ( ImGui::MenuItem( "Reload Folder", nullptr, false, ImageList_InFolder() ) )
	{
		ImageList_LoadFiles();
	}

	ImGui::Separator();

	if ( ImGui::MenuItem( "Settings", nullptr, false ) )
	{
	}

	ImGui::EndPopup();
}


void Main_WindowDraw()
{
	if ( !gCanDraw )
		return;

	gCanDraw = false;

	Render_NewFrame();

	ImageView_Draw();

	// ----------------------------------------------------------------------
	// UI Building

	ImGui::NewFrame();

	ImageList_Draw();

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
	// if ( Plat_WindowFocused() )
	Main_VoidContextMenu();

	// if ( gSettingsOpen )
	// 	Settings_Draw();

	// Temp
	// ImGui::ShowDemoWindow();

	// ----------------------------------------------------------------------
	// Rendering

	Render_Present();

	gCanDraw = true;
}


int entry()
{
	if ( !LoadRenderer() )
		return 1;

	ImGui::CreateContext();

	auto imguiCtx = ImGui::GetCurrentContext();

	if ( !Plat_Init() )
	{
		printf( "Failed to init platform!!\n" );
		ImGui::DestroyContext();
		return 1;
	}

	StyleImGui();

	if ( !Render_Init( Plat_GetWindow() ) )
	{
		printf( "Failed to Init Renderer!!\n" );
		ImGui::DestroyContext();
		Plat_Shutdown();
		return 1;
	}

	if ( Args_Count() > 1 )
	{
		ImageView_SetImage( Args_Get( 1 ) );
	}

	Render_SetClearColor( 48, 48, 48 );
	
	gCanDraw = Plat_WindowOpen();
	gRunning = gCanDraw;

	while ( gCanDraw )
	{
		Plat_Update();

		if ( Plat_IsKeyDown( K_RIGHT ) )
		{
			ImageList_LoadNextImage();
			gShouldDraw = true;
		}

		else if ( Plat_IsKeyDown( K_LEFT ) )
		{
			ImageList_LoadPrevImage();
			gShouldDraw = true;
		}

		ImageList_Update();
		gShouldDraw |= ImageView_Update();

		// if ( gShouldDraw )
			Main_WindowDraw();

		Plat_Sleep( 5 );

		gShouldDraw = false;
		gCanDraw    = Plat_WindowOpen();
	}

	Render_Shutdown();

	ImGui::DestroyContext();

	Plat_Shutdown();

	return 0;
}


#if 0 //def _WIN32
#include <Windows.h>

int wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nShowCmd )
{
	int     argc;
	LPWSTR* argv = CommandLineToArgvW( pCmdLine, &argc );

	Args_Init( argc, argv );

	// Plat_Win32Init( hInstance, hPrevInstance, nShowCmd );

	return entry();
}
#elif defined( _WIN32 )

int wmain( int argc, wchar_t* argv[], wchar_t* envp[] )
{
	Args_Init( argc, argv );

	return entry();
}

#else

int main( int argc, char* argv[] )
{
	Args_Init( argc, argv );

	return entry();
}

#endif
