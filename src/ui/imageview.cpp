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


fs::path                              gImagePath;
ImageInfo*                            gpImageInfo = nullptr;
ImageDrawInfo                         gDrawInfo;

ImageFilter                           gZoomOutFilter = ImageFilter_Cubic;
ImageFilter                           gZoomInFilter  = ImageFilter_Nearest;

size_t                                gImageHandle   = 0;
double                                gZoomLevel     = 1.0f;
bool                                  gGrabbed       = false;

std::vector< ImageLoadThreadData_t* > gLoadThreadQueue;
std::vector< ImageLoadThreadData_t* > gImageDataQueue;
// ImageLoadThreadData_t                 gImageData;


// -------------------------------------------------------------------


// TODO: probably use a mutex lock or something to have this sleep while not used
void LoadImageFunc()
{
	while ( gRunning )
	{
		while ( gLoadThreadQueue.size() )
		{
			auto queueItem = gLoadThreadQueue[ 0 ];
			queueItem->aState = ImageLoadState_Loading;

			if ( !queueItem->aInfo && !queueItem->aPath.empty() )
			{
				queueItem->aPath = fs_clean_path( queueItem->aPath );

				if ( queueItem->aInfo = ImageLoader_LoadImage( queueItem->aPath, queueItem->aData ) )
					queueItem->aState = ImageLoadState_Finished;
				else
					queueItem->aState = ImageLoadState_Error;

				// NOTE: this is possible in Vulkan, need a different VkQueue i think, probably not SDL2 though
				// gImageHandle = Render_LoadImage( queueItem->aInfo, queueItem->aData );
			}

			// if ( gLoadThreadQueue.size() && gLoadThreadQueue[ 0 ] == queueItem )
			// 	gLoadThreadQueue.erase( gLoadThreadQueue.begin() );

			vec_remove( gLoadThreadQueue, queueItem );
		}

		Plat_Sleep( 100 );
	}
}


std::thread gLoadImageThread( LoadImageFunc );


void ImageLoadThread_AddTask( ImageLoadThreadData_t* srData )
{
	gLoadThreadQueue.push_back( srData );
}


void ImageLoadThread_RemoveTask( ImageLoadThreadData_t* srData )
{
}


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

	int width, height;
	Plat_GetWindowSize( width, height );

	// offset to make the center of the window 0
	mouseX            = mouseX - ( width * 0.5 );
	mouseY            = mouseY - ( height * 0.5 );

	double oldZoom    = gZoomLevel;

	// recalc gZoomLevel
	gZoomLevel        = (double)( std::max( 1, gDrawInfo.aWidth ) * factor ) / (double)gpImageInfo->aWidth;

	// round it so we don't get something like 0.9999564598 or whatever instead of 1.0
	gZoomLevel        = std::max( ZOOM_MIN, round( gZoomLevel * 100 ) / 100 );

	// recalculate draw width and height
	gDrawInfo.aWidth  = (double)gpImageInfo->aWidth * gZoomLevel;
	gDrawInfo.aHeight = (double)gpImageInfo->aHeight * gZoomLevel;

	// recalculate image position to keep image where cursor is
	gDrawInfo.aX      = mouseX - gZoomLevel / oldZoom * ( mouseX - gDrawInfo.aX );
	gDrawInfo.aY      = mouseY - gZoomLevel / oldZoom * ( mouseY - gDrawInfo.aY );

	Main_ShouldDrawWindow();

	// lazy
	if ( gDrawInfo.aFilter == ImageFilter_Gaussian )
		Render_DownscaleImage( gpImageInfo, { gDrawInfo.aWidth, gDrawInfo.aHeight } );
}


// -------------------------------------------------------------------


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

	auto imageData = gImageDataQueue[ 0 ];

	gpImageInfo    = imageData->aInfo;

	gImageHandle   = Render_LoadImage( gpImageInfo, imageData->aData );

	// default zoom settings for now
	int width, height;
	Plat_GetWindowSize( width, height );

	gDrawInfo.aX      = ( width - gpImageInfo->aWidth ) / 2;
	gDrawInfo.aY      = ( height - gpImageInfo->aHeight ) / 2;
	gDrawInfo.aWidth  = gpImageInfo->aWidth;
	gDrawInfo.aHeight = gpImageInfo->aHeight;
	gDrawInfo.aFilter = gZoomOutFilter;

	gImagePath        = imageData->aPath;

	ImageView_FitInView();

	// update image list
	ImageList_SetPathFromFile( imageData->aPath );

	Plat_SetWindowTitle( L"Demez Image View - " + imageData->aPath.wstring() );

	imageData->aPath.clear();
	imageData->aData.clear();
	imageData->aInfo = nullptr;

	vec_remove( gImageDataQueue, imageData );
	delete imageData;

	Main_ShouldDrawWindow();
}


void ImageView_Update()
{
	if ( gImageDataQueue.size() )
	{
		switch ( gImageDataQueue[ 0 ]->aState )
		{
			case ImageLoadState_Error:
			{
				auto imageData = gImageDataQueue[ 0 ];
				vec_remove( gImageDataQueue, imageData );
				delete imageData;
				break;
			}

			case ImageLoadState_NotLoaded:
			case ImageLoadState_Loading:
				break;

			case ImageLoadState_Finished:
				// an image being loaded in the background is now ready to be finished loading on the main thread
				ImageView_LoadImage();
		}
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


std::string GetFilterName( ImageFilter filter )
{
	switch ( filter )
	{
		default:
		case ImageFilter_Nearest:
			return "Nearest";

		case ImageFilter_Linear:
			return "Linear";

		case ImageFilter_Cubic:
			return "Cubic";

		case ImageFilter_Gaussian:
			// filter += "Gaussian";
			return "Cubic (Compute)";
	}
}


void ImageView_Draw()
{
	static bool wasLoading = false;

	if ( gImageDataQueue.size() && !gImageDataQueue[ 0 ]->aPath.empty() )
	{
		if ( !wasLoading )
			Main_ShouldDrawWindow();

		wasLoading = true;

#if _WIN32
		std::string path;
		Plat_ToMultiByte( gImageDataQueue[ 0 ]->aPath.wstring(), path );
#else
		std::string path = gImageDataQueue[ 0 ]->aPath.string();
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
		if ( wasLoading )
			Main_ShouldDrawWindow();

		wasLoading = false;
	}

	if ( !gpImageInfo )
		return;

	if ( ImGui::Begin( "Image View Debug" ) )
	{
		// std::u8string what = gImagePath.u8string();
		// what.resize( gImagePath.u8string

		// char* test = utf8_decode( gImagePath.c_str(),);

		// ImGui::Text( "Image: %s", what.c_str() );

		ImGui::Separator();

		if ( ImGui::Button( "Switch Downscale Filter" ) )
		{
			gZoomOutFilter += 1;

			if ( gZoomOutFilter == ImageFilter_Gaussian )
				Render_DownscaleImage( gpImageInfo, { gDrawInfo.aWidth, gDrawInfo.aHeight } );

			else if ( gZoomOutFilter == ImageFilter_Count )
				gZoomOutFilter = ImageFilter_Nearest;
		}

		std::string downFilter = "Downscale Filter: " + GetFilterName( gZoomOutFilter );
		ImGui::Text( downFilter.c_str() );

		if ( ImGui::Button( "Switch Upscale Filter" ) )
		{
			gZoomInFilter += 1;

			if ( gZoomInFilter == ImageFilter_Count )
				gZoomInFilter = ImageFilter_Nearest;
		}

		std::string upFilter = "Upscale Filter: " + GetFilterName( gZoomInFilter );

		ImGui::Text( upFilter.c_str() );

		ImGui::Separator();
		// ImGui::Spacing();

		gDrawInfo.aFilter     = ImageView_GetImageFilter();
		std::string curFilter = "Current Filter: " + GetFilterName( gDrawInfo.aFilter );
		ImGui::Text( curFilter.c_str() );

		ImGui::Separator();

		ImGui::TextUnformatted( "Rotate" );
		ImGui::SetNextItemWidth( -1.f );  // force it to fill the window
		ImGui::SliderAngle( " rotate_slider", &gDrawInfo.aRotation, 0, 360 );
	}
	else
	{
		gDrawInfo.aFilter = ImageView_GetImageFilter();
	}

	ImGui::End();

	Render_DrawImage( gpImageInfo, gDrawInfo );
}


void ImageView_SetImage( const fs::path& path )
{
	ImageLoadThreadData_t* imageData = nullptr;
	if ( gImageDataQueue.size() == 2 )
	{
		// replace the one in queue that's left untouched with this new path
		imageData = gImageDataQueue[ 1 ];
		imageData->aPath = path;
	}
	else
	{
		imageData = gImageDataQueue.emplace_back( new ImageLoadThreadData_t{ path } );
	}

	// gImageData.aPath = path;
	// gImageData.aInfo = nullptr;
	// gImageData.aData.clear();

	ImageLoadThread_AddTask( imageData );
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
	// keep where we are centered on in the image
	gDrawInfo.aX = 1 / gZoomLevel * ( gDrawInfo.aX );
	gDrawInfo.aY = 1 / gZoomLevel * ( gDrawInfo.aY );

	gZoomLevel   = 1.0;

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

	gDrawInfo.aX = 0.f;
	gDrawInfo.aY = 0.f;
}


void ImageView_ResetRotation()
{
	gDrawInfo.aRotation = 0.f;
}


float ImageView_GetRotation()
{
	return gDrawInfo.aRotation;
}


void ImageView_SetRotation( float rotation )
{
	gDrawInfo.aRotation = rotation;
}


