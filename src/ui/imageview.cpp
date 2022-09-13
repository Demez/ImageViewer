#include <string>
#include <thread>
#include <condition_variable>

#include "args.h"
#include "imageview.h"
#include "platform.h"
#include "ui/imagelist.h"
#include "util.h"
#include "formats/imageloader.h"
#include "render.h"

#include "imgui.h"
//#include "async/async.h"


fs::path      gImagePath;
ImageInfo*    gpImageInfo = nullptr;
ImageDrawInfo gDrawInfo;
size_t        gImageHandle = 0;
double        gZoomLevel   = 1.0f;
bool          gGrabbed     = false;


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
bool HandleWheelEvent( int scroll )
{
	if ( !gpImageInfo || scroll == 0 )
		return false;

	printf( "scroll: %d\n", scroll );

	double factor = 1.0;

	// Zoom in if scrolling up
	if ( scroll > 0 )
	{
		// max zoom level
		if ( gZoomLevel >= 100.0 )
			return false;

		factor += ZOOM_AMOUNT;
	}
	else
	{
		// min zoom level
		if ( gZoomLevel <= 0.01 )
			return false;

		factor -= ZOOM_AMOUNT;
	}

	// TODO: round zoom more the further you get in
	// ex. round by 100 at 200%, round by 1000 at 300%
	// at least i think that will work

	// also need to somehow do a zoom level check to fit inside the window
	// like fragment does

	// TODO: add zoom levels to snap to here
	// 100, 200, 400, 500, 50, 25, etc

	auto roundedZoom = std::max( 0.1f, roundf( gZoomLevel * factor * 10 ) / 10 );

	printf( "rounded zoom: %.3f\n", roundedZoom );

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

	return true;
}


// -------------------------------------------------------------------


void ImageView_EventMouseMotion( int xrel, int yrel )
{
	if ( !gGrabbed )
		return;

	auto& io = ImGui::GetIO();

	// imgui window is being interacted with
	if ( io.MouseDownOwned[ 0 ] )
	{
		gGrabbed = false;
		return;
	}

	if ( !gpImageInfo )
		return;

	gDrawInfo.aX += xrel;
	gDrawInfo.aY += yrel;
}


bool ImageView_Update()
{
	bool shouldDraw = false;

	gGrabbed = Plat_IsKeyPressed( K_LBUTTON );

	if ( gGrabbed )
	{
		int dx, dy;
		Plat_GetMouseDelta( dx, dy );
		ImageView_EventMouseMotion( dx, dy );
		shouldDraw = true;
	}

	// if window resized and zoom mode set to ImageZoom_FitInView, refit the image
	// probably use a callback for this? idk

	if ( !gpImageInfo )
		return false;

	shouldDraw |= HandleWheelEvent( Plat_GetMouseScroll() );

	return shouldDraw;
}


void ImageView_Draw()
{
	if ( gpImageInfo )
		Render_DrawImage( gpImageInfo, gDrawInfo );
}


void ImageView_SetImageInternal( const fs::path& path )
{
	fs::path            file         = fs_clean_path( path );

	// TODO: this is a slow blocking operation on the main thread
	// async or move this to a image loader thread and use a callback for when it's loaded
	ImageInfo*          newImageData = nullptr;
	std::vector< char > data;
	if ( !( newImageData = ImageLoader_LoadImage( file, data ) ) )
		return;

	if ( gpImageInfo )
		ImageView_RemoveImage();

	gpImageInfo  = newImageData;

	gImageHandle = Render_LoadImage( gpImageInfo, data );

	// default zoom settings for now
	int width, height;
	Plat_GetWindowSize( width, height );

	gDrawInfo.aX      = ( width - gpImageInfo->aWidth ) / 2;
	gDrawInfo.aY      = ( height - gpImageInfo->aHeight ) / 2;
	gDrawInfo.aWidth  = gpImageInfo->aWidth;
	gDrawInfo.aHeight = gpImageInfo->aHeight;

	gImagePath        = file;

	ImageView_FitInView();

	// update image list
	ImageList_SetPathFromFile( file );

	Plat_SetWindowTitle( L"Demez Image View - " + path.wstring() );
}


void ImageView_SetImage( const fs::path& path )
{
	// static bool threaded = !Args_Has( _T("-no-threading") );
	ImageView_SetImageInternal( path );
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


