#include <string>
#include <thread>
#include <condition_variable>

#include "args.h"
#include "main.h"
#include "imageview.h"
#include "platform.h"
#include "ui/imagelist.h"
#include "util.h"
#include "formats/imageloader.h"
#include "render.h"

#include "imgui.h"
//#include "async/async.h"


fs::path         gImagePath;
ImageInfo*       gpImageInfo = nullptr;
ImageDrawInfo    gDrawInfo;

ImageFilter      gZoomOutFilter = ImageFilter_Cubic;
ImageFilter      gZoomInFilter  = ImageFilter_Nearest;

size_t           gImageHandle   = 0;
double           gZoomLevel     = 1.0f;
bool             gGrabbed       = false;


// -------------------------------------------------------------------


constexpr double ZOOM_AMOUNT = 0.1;
constexpr double ZOOM_MIN = 0.01;


void UpdateZoom()
{
	// round it so we don't get something like 0.9999564598 or whatever instead of 1.0
	gZoomLevel        = std::max( ZOOM_MIN, round( gZoomLevel * 100 ) / 100 );

	// recalculate draw width and height
	gDrawInfo.aWidth  = (double)gpImageInfo->aWidth * gZoomLevel;
	gDrawInfo.aHeight = (double)gpImageInfo->aHeight * gZoomLevel;
}


// returns if there was any scroll change
void HandleWheelEvent( char scroll )
{
	if ( !gpImageInfo || scroll == 0 )
		return;

	double factor = 1.0;

	// Zoom in if scrolling up
	if ( scroll > 0 )
	{
		// max zoom level
		if ( gZoomLevel >= 100.0 )
			return;

		factor += ( ZOOM_AMOUNT * scroll );
	}
	else
	{
		// min zoom level
		if ( gZoomLevel <= 0.01 )
			return;

		factor -= ( ZOOM_AMOUNT * abs(scroll) );
	}

	// TODO: round zoom more the further you get in
	// ex. round by 100 at 200%, round by 1000 at 300%
	// at least i think that will work

	// also need to somehow do a zoom level check to fit inside the window
	// like fragment does

	// TODO: add zoom levels to snap to here
	// 100, 200, 400, 500, 50, 25, etc

	auto roundedZoom = std::max( 0.1f, roundf( gZoomLevel * factor * 10 ) / 10 );

	if ( fmod( roundedZoom, 1.0 ) == 0 )
		factor = roundedZoom / gZoomLevel;

	int mouseX, mouseY;
	Plat_GetMousePos( mouseX, mouseY );

	double oldZoom = gZoomLevel;

	// recalc gZoomLevel
	gZoomLevel = (double)(std::max(1, gDrawInfo.aWidth) * factor) / (double)gpImageInfo->aWidth;

	// round it so we don't get something like 0.9999564598 or whatever instead of 1.0
	gZoomLevel = std::max( ZOOM_MIN, round( gZoomLevel * 100 ) / 100 );

	// recalculate draw width and height
	gDrawInfo.aWidth  = (double)gpImageInfo->aWidth * gZoomLevel;
	gDrawInfo.aHeight = (double)gpImageInfo->aHeight * gZoomLevel;
	
	// recalculate image position to keep image where cursor is
	gDrawInfo.aX = mouseX - gZoomLevel / oldZoom * ( mouseX - gDrawInfo.aX );
	gDrawInfo.aY = mouseY - gZoomLevel / oldZoom * ( mouseY - gDrawInfo.aY );

	Main_ShouldDrawWindow();
}


// -------------------------------------------------------------------


fs::path            gNewImagePath;
ImageInfo*          gNewImageData = nullptr;
std::vector< char > gReadData;


void ImageView_SetImageInternal( const fs::path& path )
{
	gNewImagePath = fs_clean_path( path );

	// TODO: this is a slow blocking operation on the main thread
	// async or move this to a image loader thread and use a callback for when it's loaded
	if ( !( gNewImageData = ImageLoader_LoadImage( gNewImagePath, gReadData ) ) )
	{
		gNewImagePath.clear();
	}
}


// TODO: probably use a mutex lock or something to have this sleep while not used
void LoadImageFunc()
{
	while ( gRunning )
	{
		if ( !gNewImageData && !gNewImagePath.empty() )
			ImageView_SetImageInternal( gNewImagePath );

		Plat_Sleep( 100 );
	}
}


std::thread gLoadImageThread( LoadImageFunc );


void ImageView_Shutdown()
{
	gRunning = false;
	gLoadImageThread.join();
}


// blech
ImageFilter ImageView_GetImageFilter()
{
	if ( gZoomLevel >= 1.0 )
		return gZoomInFilter;

	return gZoomOutFilter;
}


void ImageView_SetImageFilter( ImageFilter filter )
{
	if ( gZoomLevel >= 1.0 )
	{
		gZoomInFilter = filter;
	}
	else
	{
		gZoomOutFilter = filter;
	}
}


void ImageView_LoadImage()
{
	if ( gpImageInfo )
		ImageView_RemoveImage();

	gpImageInfo  = gNewImageData;

	gImageHandle = Render_LoadImage( gpImageInfo, gReadData );

	// default zoom settings for now
	int width, height;
	Plat_GetWindowSize( width, height );

	gDrawInfo.aX      = ( width - gpImageInfo->aWidth ) / 2;
	gDrawInfo.aY      = ( height - gpImageInfo->aHeight ) / 2;
	gDrawInfo.aWidth  = gpImageInfo->aWidth;
	gDrawInfo.aHeight = gpImageInfo->aHeight;
	gDrawInfo.aFilter = gZoomOutFilter;

	gImagePath        = gNewImagePath;

	ImageView_FitInView();

	// update image list
	ImageList_SetPathFromFile( gNewImagePath );

	Plat_SetWindowTitle( L"Demez Image View - " + gNewImagePath.wstring() );

	gNewImagePath.clear();
	gNewImageData = nullptr;
	gReadData.clear();

	Main_ShouldDrawWindow();
}


void ImageView_Update()
{
	if ( gNewImageData )
	{
		// an image being loaded in the background is now ready to be finished loading on the main thread
		ImageView_LoadImage();
	}

	if ( Plat_IsKeyDown( K_DELETE ) )
	{
		ImageView_DeleteImage();
		return;
	}

	if ( !gpImageInfo )
		return;
	
	auto& io = ImGui::GetIO();

	// if ( io.WantCaptureMouse && !gGrabbed )
	if ( io.WantCaptureMouseUnlessPopupClose && !gGrabbed )
		return;

	gGrabbed = Plat_IsKeyPressed( K_LBUTTON );

	// check if the mouse isn't hovering over any window and we didn't grab it already
	if ( gGrabbed )
	{
		int xrel, yrel;
		Plat_GetMouseDelta( xrel, yrel );
		gDrawInfo.aX += xrel;
		gDrawInfo.aY += yrel;

		if ( xrel != 0 || yrel != 0 )
			Main_ShouldDrawWindow();
	}

	// if window resized and zoom mode set to ImageZoom_FitInView, refit the image
	// probably use a callback for this? idk

	if ( io.WantCaptureMouseUnlessPopupClose )
		return;

	HandleWheelEvent( Plat_GetMouseScroll() );
}


constexpr void operator+=( ImageFilter& filter, unsigned char other )
{
	u8& number = (u8&)filter;
	number += other;
}


void ImageView_Draw()
{
	static bool wasLoading = false;

	if ( !gNewImagePath.empty() )
	{
		if ( !wasLoading )
			Main_ShouldDrawWindow();

		wasLoading = true;
		// thanks imgui for not having this use ImWchar
#if _WIN32
		std::string path = Plat_FromUnicode( gNewImagePath.c_str() );
#else
		std::string path = gNewImagePath.string();
#endif

		path = "Loading Image: " + path;

		int width, height;
		Plat_GetWindowSize( width, height );

		float lineHeight = ImGui::GetTextLineHeightWithSpacing();

		// ImGui::SetNextWindowPos( { 16.f, (float)height - lineHeight - 24.f } );
		// ImGui::SetNextWindowSizeConstraints( { 32.f, 64.f }, { 32.f, (float)width } );
		// ImGui::SetNextWindowSize( { 32, 96 } );

		float calcWidth = ImGui::CalcItemWidth();
		ImVec2 textSize = ImGui::CalcTextSize( path.c_str() );

		ImGui::SetNextWindowPos( { 16.f, (float)height - (textSize.y * 2) - 16.f } );
		ImGui::SetNextWindowSize( { textSize.x + textSize.y, textSize.y * 2 } );

		float test = ImGui::GetCursorPosY();

		ImGui::SetNextWindowBgAlpha( 32 );

		ImGui::Begin( "loading text", 0,
			ImGuiWindowFlags_NoNav | 
			ImGuiWindowFlags_NoDecoration | 
			ImGuiWindowFlags_NoInputs |
			ImGuiWindowFlags_NoSavedSettings );

		ImGui::TextUnformatted( path.c_str() );

		// ImGui::SetWindowPos( { 16.f, (float)height - ImGui::GetWindowHeight() - 16.f } );

		ImGui::End();
	}
	else
	{
		wasLoading = false;
		Main_ShouldDrawWindow();
	}

	if ( !gpImageInfo )
		return;


	if ( ImGui::Button( "Switch Filter" ) )
	{
		gDrawInfo.aFilter += 1;
		if ( gDrawInfo.aFilter == ImageFilter_Count )
			gDrawInfo.aFilter = ImageFilter_Nearest;

		ImageView_SetImageFilter( gDrawInfo.aFilter );
	}

	gDrawInfo.aFilter = ImageView_GetImageFilter();

	std::string filter = "Filter: ";

	switch ( gDrawInfo.aFilter )
	{
		case ImageFilter_Nearest:
			filter += "Nearest";
			break;

		case ImageFilter_Linear:
			filter += "Linear";
			break;

		case ImageFilter_Cubic:
			filter += "Cubic";
			break;

		// case ImageFilter_Gaussian:
		// 	// filter += "Gaussian";
		// 	filter += "Cubic (Compute)";
		// 	break;
	}

	ImGui::Text( filter.c_str() );

	Render_DrawImage( gpImageInfo, gDrawInfo );
}


void ImageView_SetImage( const fs::path& path )
{
	gNewImagePath = path;
}


void ImageView_RemoveImage()
{
	if ( !gpImageInfo )
		return;

	Render_FreeImage( gpImageInfo );

	delete gpImageInfo;
}


bool ImageView_HasImage()
{
	return gpImageInfo;
}


void ImageView_DeleteImage()
{
	if ( !gpImageInfo )
		return;

	Plat_DeleteFile( gImagePath );
	ImageList_RemoveItem( gImagePath );

	if ( !ImageList_LoadNextImage() )
		ImageView_RemoveImage();
}


const fs::path& ImageView_GetImagePath()
{
	return gImagePath;
}


double ImageView_GetZoomLevel()
{
	return gZoomLevel;
}


void ImageView_SetZoomLevel( double level )
{
	gZoomLevel = level;
	UpdateZoom();
}


void ImageView_ResetZoom()
{
	int width, height;
	Plat_GetWindowSize( width, height );

	int centerX = width / 2;
	int centerY = height / 2;

	// keep where we are centered on in the image
	gDrawInfo.aX      = centerX - 1 / gZoomLevel * ( centerX - gDrawInfo.aX );
	gDrawInfo.aY      = centerY - 1 / gZoomLevel * ( centerY - gDrawInfo.aY );

	gZoomLevel        = 1.0;

	if ( !gpImageInfo )
		return;

	gDrawInfo.aWidth  = gpImageInfo->aWidth;
	gDrawInfo.aHeight = gpImageInfo->aHeight;
}


// TODO: set aZoom to be the current image scale
void ImageView_FitInView( bool sScaleUp )
{
	if ( !ImageView_HasImage() )
	{
		gZoomLevel = 1.0;
		return;
	}

	int width, height;
	Plat_GetWindowSize( width, height );

	// fit in the view, scaling up the image if needed
	if ( sScaleUp )
	{
		auto factor = std::min(
		  ( (float)width / (float)gpImageInfo->aWidth ),
		  ( (float)height, (float)gpImageInfo->aHeight )
		);

		gZoomLevel = factor;
	}
	// fit in the view, scale down the image if needed
	else
	{
		float factor[2] = { 1.f, 1.f };

		if ( gpImageInfo->aWidth > width )
			factor[ 0 ] = (float)width / (float)gpImageInfo->aWidth;

		if ( gpImageInfo->aHeight > height )
			factor[ 1 ] = (float)height / (float)gpImageInfo->aHeight;

		gZoomLevel = std::min( factor[0], factor[1] );
	}

	UpdateZoom();

	gDrawInfo.aX = ( width - gDrawInfo.aWidth ) / 2;
	gDrawInfo.aY = ( height - gDrawInfo.aHeight ) / 2;
}


