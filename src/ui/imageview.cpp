#include <string>

#include "imageview.h"
#include "util.h"
#include "formats/imageloader.h"
#include "render_sdl/render_sdl.h"

#include <SDL.h>
#include "imgui.h"


std::string   gImagePath;
ImageData*    gpImageData = nullptr;
ImageDrawInfo gDrawInfo;
size_t        gImageHandle = 0;
double        gZoomLevel   = 1.0f;
bool          gGrabbed     = false;


// -------------------------------------------------------------------


constexpr double ZOOM_AMOUNT = 0.1;


void UpdateZoom()
{
	// round it so we don't get something like 0.9999564598 or whatever instead of 1.0
	gZoomLevel        = round( gZoomLevel * 100 ) / 100;

	// recalculate draw width and height
	gDrawInfo.aWidth  = (double)gpImageData->aWidth * gZoomLevel;
	gDrawInfo.aHeight = (double)gpImageData->aHeight * gZoomLevel;
}


void HandleWheelEvent( int scroll )
{
	if ( !gpImageData )
		return;

	double factor = 1.0;

	// Zoom in if scrolling up
	if ( scroll > 0 )
	{
		// max zoom level
		if ( gZoomLevel >= 100.0 )
			return;

		factor += ZOOM_AMOUNT;
	}
	else
	{
		// min zoom level
		if ( gZoomLevel <= 0.01 )
			return;

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
	SDL_GetMouseState( &mouseX, &mouseY );

	double oldZoom = gZoomLevel;

	// recalc gZoomLevel
	gZoomLevel = (double)(gDrawInfo.aWidth * factor) / (double)gpImageData->aWidth;

	// round it so we don't get something like 0.9999564598 or whatever instead of 1.0
	gZoomLevel = round( gZoomLevel * 100 ) / 100;

	// recalculate draw width and height
	gDrawInfo.aWidth  = (double)gpImageData->aWidth * gZoomLevel;
	gDrawInfo.aHeight = (double)gpImageData->aHeight * gZoomLevel;
	
	// recalculate image position to keep image where cursor is
	gDrawInfo.aX = mouseX - gZoomLevel / oldZoom * ( mouseX - gDrawInfo.aX );
	gDrawInfo.aY = mouseY - gZoomLevel / oldZoom * ( mouseY - gDrawInfo.aY );
}


void HandleMouseDrag( int xrel, int yrel )
{
	if ( !gpImageData )
		return;

	gDrawInfo.aX += xrel;
	gDrawInfo.aY += yrel;
}


// -------------------------------------------------------------------


void ImageView_HandleEvent( SDL_Event& srEvent )
{
	switch ( srEvent.type )
	{
		case SDL_MOUSEBUTTONDOWN:
		{
			// check to make sure we aren't interacting with any imgui windows
			// MouseDownOwned

			// auto& io = ImGui::GetIO();
			// 
			// // imgui window is being interacted with
			// if ( io.MouseDownOwned[ 0 ] )
			// 	return;

			gGrabbed = true;
			return;
		}

		case SDL_MOUSEBUTTONUP:
		{
			gGrabbed = false;
			return;
		}

		case SDL_MOUSEMOTION:
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
			
			HandleMouseDrag( srEvent.motion.xrel, srEvent.motion.yrel );
			return;
		}

		case SDL_MOUSEWHEEL:
		{
			HandleWheelEvent( srEvent.wheel.y );
			return;
		}

		case SDL_WINDOWEVENT:
		{
			switch ( srEvent.window.event )
			{
				case SDL_WINDOWEVENT_RESIZED:
				case SDL_WINDOWEVENT_SIZE_CHANGED:
				{
					if ( !gpImageData )
						return;

					// int width, height;
					// Render_GetWindowSize( width, height );
					// 
					// gDrawInfo.aX = ( width - gpImageData->aWidth ) / 2;
					// gDrawInfo.aY = ( height - gpImageData->aHeight ) / 2;
				}
			}

			return;
		}
	}
}


void ImageView_Draw()
{
	if ( !gpImageData )
		return;

	// int width, height;
	// Render_GetWindowSize( width, height );
	// 
	// ImageDrawInfo drawInfo;
	// drawInfo.aX      = (width - gpImageData->aWidth)/2;
	// drawInfo.aY      = (height - gpImageData->aHeight)/2;
	// drawInfo.aWidth  = gpImageData->aWidth;
	// drawInfo.aHeight = gpImageData->aHeight;

	Render_DrawImage( gpImageData, gDrawInfo );
}


bool ImageView_SetImage( const std::string& path )
{
	// TODO: this is a slow blocking operation on the main thread
	// async or move this to a image loader thread and use a callback for when it's loaded
	ImageData* newImageData = nullptr;
	if ( !(newImageData = GetImageLoader().LoadImage( path )) )
		return false;

	if ( gpImageData )
		ImageView_RemoveImage();

	gpImageData = newImageData;

	gImageHandle = Render_LoadImage( gpImageData );

	// default zoom settings for now
	int width, height;
	Render_GetWindowSize( width, height );

	gDrawInfo.aX      = ( width - gpImageData->aWidth ) / 2;
	gDrawInfo.aY      = ( height - gpImageData->aHeight ) / 2;
	gDrawInfo.aWidth  = gpImageData->aWidth;
	gDrawInfo.aHeight = gpImageData->aHeight;

	ImageView_FitInView();

	return true;
}


void ImageView_RemoveImage()
{
	if ( !gpImageData )
		return;

	delete gpImageData;
}


bool ImageView_HasImage()
{
	return gpImageData;
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
	Render_GetWindowSize( width, height );

	int centerX = width / 2;
	int centerY = height / 2;

	// keep where we are centered on in the image
	gDrawInfo.aX      = centerX - 1 / gZoomLevel * ( centerX - gDrawInfo.aX );
	gDrawInfo.aY      = centerY - 1 / gZoomLevel * ( centerY - gDrawInfo.aY );

	gZoomLevel        = 1.0;

	gDrawInfo.aWidth  = gpImageData->aWidth;
	gDrawInfo.aHeight = gpImageData->aHeight;
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
	Render_GetWindowSize( width, height );

	// fit in the view, scaling up the image if needed
	if ( sScaleUp )
	{
		auto factor = std::min(
		  ( (float)width / (float)gpImageData->aWidth ),
		  ( (float)height, (float)gpImageData->aHeight )
		);

		gZoomLevel = factor;
	}
	// fit in the view, scale down the image if needed
	else
	{
		float factor[2] = { 1.f, 1.f };

		if ( gpImageData->aWidth > width )
			factor[ 0 ] = (float)width / (float)gpImageData->aWidth;

		if ( gpImageData->aHeight > height )
			factor[ 1 ] = (float)height / (float)gpImageData->aHeight;

		gZoomLevel = std::min( factor[0], factor[1] );
	}

	UpdateZoom();

	gDrawInfo.aX = ( width - gDrawInfo.aWidth ) / 2;
	gDrawInfo.aY = ( height - gDrawInfo.aHeight ) / 2;
}


bool CheckDragInput( const std::string& dragText )
{
	if ( !dragText.starts_with( "file:///" ) )
	{
		printf( "WARNING: [CheckDragInput] unknown drag data type: %s\n", dragText.c_str() );
		return false;
	}

	std::string fileExt = fs_get_file_ext( dragText );

	if ( fileExt.empty() )
	{
		printf( "WARNING: [CheckDragInput] no file extension?: %s\n", dragText.c_str() );
		return false;
	}

	if ( !GetImageLoader().CheckExt( fileExt ) )
	{
		printf( "WARNING: [CheckDragInput] unknown file extension: %s\n", fileExt.c_str() );
		return false;
	}

	return true;
}


