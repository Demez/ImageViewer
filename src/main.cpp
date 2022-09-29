#include <memory>

size_t gTotalAllocated = 0;

// replace operator new and delete to log allocations
void*  operator new( size_t n )
{
	size_t* p = (size_t*)malloc( n + sizeof( size_t ) );
	*p++      = n;
	gTotalAllocated += n;
	return p;
}

void operator delete( void* p ) throw()
{
	size_t* sp = (size_t*)p;
	*--sp;
	gTotalAllocated -= *sp;
	free( sp );
}

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
#include "misc/freetype/imgui_freetype.h"


// TODO LIST:
// 
// - be able to hold control and drag the image out as a drop source
// - add an image gallery view with different view types
// - for the image list, be able to hover over the image and get a larger preview
// 

// temp
#define UN_SUNGLASSES "\U0001F60E"
#define UN_POINT_UP "\u261D"

std::filesystem::path      gFontPath      = _T("CascadiaMono.ttf");
std::filesystem::path      gFontEmojiPath = _T("seguiemj.ttf");

ImFont*                    gpFont         = nullptr;
ImFont*                    gpFontEmoji    = nullptr;

static ImFontConfig        gFontConfig;
static ImFontConfig        gFontEmojiConfig;


Module gRenderer = 0;


Render_Init_t              Render_Init              = 0;
Render_Shutdown_t          Render_Shutdown          = 0;

Render_NewFrame_t          Render_NewFrame          = 0;
Render_Reset_t             Render_Reset             = 0;
Render_Present_t           Render_Present           = 0;

Render_SetResolution_t     Render_SetResolution     = 0;
Render_SetClearColor_t     Render_SetClearColor     = 0;
Render_GetClearColor_t     Render_GetClearColor     = 0;

Render_LoadImage_t         Render_LoadImage         = 0;
Render_FreeImage_t         Render_FreeImage         = 0;
Render_DrawImage_t         Render_DrawImage         = 0;
Render_DownscaleImage_t    Render_DownscaleImage    = 0;
Render_AddTextureToImGui_t Render_AddTextureToImGui = 0;

Render_AddFont_t           Render_AddFont           = 0;
Render_BuildFonts_t        Render_BuildFonts        = 0;


#define LOAD_RENDER_FUNC( name ) \
	if ( !(name = (##name##_t)Plat_LoadFunc( gRenderer, #name )) ) \
	{                                                                \
		printf( "Failed To Load Render Function: \"%s\"\n", #name );\
		return false;\
	}


bool LoadRenderer()
{
	if ( Args_Has( _T("-sdl") ) )
	{
		if ( !( gRenderer = Plat_LoadLibrary( _T("render_sdl") EXT_DLL ) ) )
		{
			printf( "Failed to Load Renderer!!\n" );
			return false;
		}
	}
	else
	{
		if ( !( gRenderer = Plat_LoadLibrary( _T("render_vk") EXT_DLL ) ) )
		{
			printf( "Failed to Load Renderer!!\n" );
			return false;
		}
	}

	LOAD_RENDER_FUNC( Render_Init );
	LOAD_RENDER_FUNC( Render_Shutdown );

	LOAD_RENDER_FUNC( Render_NewFrame );
	LOAD_RENDER_FUNC( Render_Reset );
	LOAD_RENDER_FUNC( Render_Present );

	LOAD_RENDER_FUNC( Render_SetResolution );
	LOAD_RENDER_FUNC( Render_SetClearColor );
	LOAD_RENDER_FUNC( Render_GetClearColor );

	LOAD_RENDER_FUNC( Render_LoadImage );
	LOAD_RENDER_FUNC( Render_FreeImage );
	LOAD_RENDER_FUNC( Render_DrawImage );
	LOAD_RENDER_FUNC( Render_DownscaleImage );
	LOAD_RENDER_FUNC( Render_AddTextureToImGui );

	LOAD_RENDER_FUNC( Render_AddFont );
	LOAD_RENDER_FUNC( Render_BuildFonts );

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
static bool gShowImGuiDemo           = false;


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

	if ( ImGui::MenuItem( "Undo", nullptr, false, UndoSys_CanUndo() ) )
	{
		UndoSys_Undo();
	}

	if ( ImGui::MenuItem( "Redo", nullptr, false, UndoSys_CanRedo() ) )
	{
		UndoSys_Redo();
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
		auto sortMode = ImageList_GetSortMode();

		if ( ImGui::MenuItem( "File Name - A to Z", nullptr, sortMode == FileSort_AZ, false ) )
			ImageList_SetSortMode( FileSort_AZ );

		if ( ImGui::MenuItem( "File Name - Z to A", nullptr, sortMode == FileSort_ZA, false ) )
			ImageList_SetSortMode( FileSort_ZA );

		if ( ImGui::MenuItem( "Date Modified - Newest First", nullptr, sortMode == FileSort_DateModNewest, ImageList_InFolder() ) )
			ImageList_SetSortMode( FileSort_DateModNewest );

		if( ImGui::MenuItem( "Date Modified - Oldest First", nullptr, sortMode == FileSort_DateModOldest, ImageList_InFolder() ) )
			ImageList_SetSortMode( FileSort_DateModOldest );

		if ( ImGui::MenuItem( "Date Created - Newest First", nullptr, sortMode == FileSort_DateCreatedNewest, ImageList_InFolder() ) )
			ImageList_SetSortMode( FileSort_DateCreatedNewest );

		if ( ImGui::MenuItem( "Date Created - Oldest First", nullptr, sortMode == FileSort_DateCreatedOldest, ImageList_InFolder() ) )
			ImageList_SetSortMode( FileSort_DateCreatedOldest );

		if ( ImGui::MenuItem( "File Size - Largest First", nullptr, sortMode == FileSort_SizeLargest, false ) )
			ImageList_SetSortMode( FileSort_SizeLargest );

		if ( ImGui::MenuItem( "File Size - Smallest First", nullptr, sortMode == FileSort_SizeSmallest, false ) )
			ImageList_SetSortMode( FileSort_SizeSmallest );

		ImGui::EndMenu();
	}

	if ( ImGui::MenuItem( "Reload Folder", nullptr, false, ImageList_InFolder() ) )
	{
		ImageList_LoadFiles();
	}

	ImGui::Separator();

	if ( ImGui::MenuItem( "Settings", nullptr, false, false ) )
	{
	}

	if ( ImGui::MenuItem( "Show ImGui Demo", nullptr, gShowImGuiDemo ) )
	{
		gShowImGuiDemo = !gShowImGuiDemo;
	}

	ImGui::EndPopup();
}


void Main_WindowDraw()
{
	if ( !gCanDraw )
		return;

	gCanDraw = false;
	gShouldDraw = false;

	Render_NewFrame();

	// ----------------------------------------------------------------------
	// UI Building

	ImGui::NewFrame();

	// if ( gpFont )
	//  	ImGui::PushFont( gpFont );

	auto& io = ImGui::GetIO();

	// TEMP
	// ImGui::Text( "Unicode Test: —😎—◘☝—" );
	//ImGui::Text( "Unicode Test: —" UN_SUNGLASSES "â€”â€”◘" UN_POINT_UP "—" );
	ImGui::Text( "Unicode Test: â€”ðŸ˜Žâ€”â—˜â˜â€”" );

	static char bufTemp[ 256 ] = { '\0' };
	ImGui::InputText( "test: ", bufTemp, 256 );

	ImageView_Draw();
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

	if ( gShowImGuiDemo )
		ImGui::ShowDemoWindow();

	// if ( gpFont )
	//  	ImGui::PopFont();

	// ----------------------------------------------------------------------
	// Rendering

	if ( !Plat_WindowOpen() )
	{ 
		// wtf
		return;
	}

	Render_Present();

	gCanDraw = Plat_WindowOpen();
}


int entry()
{
	if ( !LoadRenderer() )
		return 1;

	ImGui::CreateContext();

	if ( !Plat_Init() )
	{
		printf( "Failed to init platform!!\n" );
		ImGui::DestroyContext();
		return 1;
	}

	Plat_SetMinWindowSize( 320, 240 );

	// StyleImGui();

	if ( !Render_Init( Plat_GetWindow() ) )
	{
		printf( "Failed to Init Renderer!!\n" );
		ImGui::DestroyContext();
		Plat_Shutdown();
		return 1;
	}

	{
		int width, height;
		Plat_GetWindowSize( width, height );
		Render_SetResolution( width, height );
	}

	std::filesystem::path entryPath = Args_Get( 0 );
	entryPath = entryPath.parent_path();

	// gFontConfig.FontBuilderFlags |= ImGuiFreeTypeBuilderFlags_LoadColor | ImGuiFreeTypeBuilderFlags_Bitmap;
	gFontConfig.FontBuilderFlags |= ImGuiFreeTypeBuilderFlags_Bitmap;
	// gFontConfig.OversampleH = gFontConfig.OversampleV = 6;
	gFontConfig.GlyphOffset.y = -2;
	// gFontConfig.MergeMode = true;

	gpFont = Render_AddFont( entryPath / gFontPath, 13, &gFontConfig );

	gFontEmojiConfig.FontBuilderFlags |= ImGuiFreeTypeBuilderFlags_LoadColor | ImGuiFreeTypeBuilderFlags_Bitmap;
	gFontEmojiConfig.MergeMode = true;
	
	gpFontEmoji = Render_AddFont( entryPath / gFontEmojiPath, 13, &gFontEmojiConfig );

	auto& io = ImGui::GetIO();

	if ( Render_BuildFonts() )
	{
		if ( gpFont )
		{
			io.FontDefault = gpFont;
		}
	}
	else
	{
		printf( "Failed to build fonts!\n" );
		ImGui::DestroyContext();
		Plat_Shutdown();
		return 1;
	}

	if ( Args_Count() > 1 )
	{
		// still kinda shit, hmm
		for ( int i = 1; i < Args_Count(); i++ )
		{
			if ( fs_is_file( Args_Get( i ).data() ) )
			{
				ImageView_SetImage( Args_Get( i ) );
				break;
			}
		}
	}

	Render_SetClearColor( 48, 48, 48 );
	
	gCanDraw = Plat_WindowOpen();
	gRunning = gCanDraw;

	while ( gCanDraw )
	{
		Plat_Update();

		ImageList_Update();
		ImageView_Update();

		if ( Plat_WindowShown() && gShouldDraw )
		// if ( Plat_WindowShown() )
			Main_WindowDraw();

		if ( Plat_WindowFocused() )
			Plat_Sleep( 0.5 );

		else
			Plat_Sleep( 20 );

		gCanDraw = Plat_WindowOpen();
	}

	Render_Shutdown();

	ImageView_Shutdown();

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
