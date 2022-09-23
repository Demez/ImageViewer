#include "imagelist.h"
#include "platform.h"
#include "util.h"
#include "main.h"
#include "formats/imageloader.h"
#include "ui/imageview.h"
#include "render.h"
#include "imgui.h"

#include <unordered_map>
#include <thread>


static fs::path                                    gCurrentDir;
static std::vector< fs::path >                     gImages;
static size_t                                      gIndex;

static FileSort                                    gSortMode = FileSort_DateModNewest;
std::unordered_map< fs::path, fs::file_time_type > gDateMap;
std::unordered_map< fs::path, struct stat >        gFileStatMap;

extern fs::path                                    gImagePath;


struct ImageListElement
{
	size_t        aIndex;
	ImageInfo*    apInfo;
	ImageDrawInfo aDrawData;
	size_t        aImageHandle = 0;
};


static std::vector< ImageInfo* > gImageThumbnails;


static fs::path                         gNewImagePath = L"C:\\Users\\Demez\\Downloads\\[twitter] lewdakiddo_rb26—2022.09.09—1568386946812215297—FcQHmKGaIAACxvH.png";
static ImageInfo*          gNewImageData = nullptr;
static std::vector< char >              gReadData;

// temp
static ImageDrawInfo                    gDrawData;
static size_t                           gImageHandle = 0;


void ImageList_LoadThumbnail( const fs::path& path )
{
	gNewImagePath = fs_clean_path( path );

	// TODO: this is a slow blocking operation on the main thread
	// async or move this to a image loader thread and use a callback for when it's loaded
	if ( !( gNewImageData = ImageLoader_LoadImage( gNewImagePath, gReadData ) ) )
		return;

	gImageHandle = Render_LoadImage( gNewImageData, gReadData );
}


// TODO: probably use a mutex lock or something to have this sleep while not used
void LoadThumbnailFunc()
{
	while ( gRunning )
	{
		if ( !gNewImageData && !gNewImagePath.empty() )
			ImageList_LoadThumbnail( gNewImagePath );

		Plat_Sleep( 100 );
	}
}

// std::thread gLoadImageThread( LoadImageFunc );


void ImageList_Update()
{
	if ( Plat_IsKeyDown( K_RIGHT ) )
	{
		ImageList_LoadNextImage();
		Main_ShouldDrawWindow();
	}

	else if ( Plat_IsKeyDown( K_LEFT ) )
	{
		ImageList_LoadPrevImage();
		Main_ShouldDrawWindow();
	}

	// TEMP !!!!
	if ( gNewImageData == nullptr )
	{
		// ImageList_LoadThumbnail( gNewImagePath );
	}

	// check folder monitor for if a change happened
	if ( !Plat_FolderMonitorChanged() )
		return;

	// something changed, reload files
	ImageList_LoadFiles();
}


void ImageList_Draw()
{
	int width, height;
	Plat_GetWindowSize( width, height );

	ImGui::SetNextWindowPos( { 0, 0 } );
	// ImGui::SetNextWindowSize( {(float)width, 64} );
	ImGui::SetNextWindowSizeConstraints( { (float)width, 80 }, { (float)width, height/2.f } );

	if ( !ImGui::Begin( "Image List", 0, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_AlwaysHorizontalScrollbar ) )
	{
		ImGui::End();
		return;
	}

	auto region = ImGui::GetContentRegionAvail();

	int selected = 0;
	for ( size_t i = 0; i < gImages.size(); i++ )
	{
		ImGui::PushID( i );

		// thanks imgui for not having this use ImWchar
#if _WIN32
		std::string path = Plat_FromUnicode( gImages[ i ].c_str() );
#else
		std::string path = gImages[ i ].string();
#endif

		if ( ImGui::Selectable( path.c_str(), gImages[ i ] == gImagePath, 0, { region.y, region.y } ) )
		{
			selected = i;
			ImageView_SetImage( gImages[ i ] );
		}

		// ImGui::BeginTooltip();
		// ImGui::TextUnformatted( "lol" );
		// ImGui::EndTooltip();

		ImGui::PopID();
		ImGui::SameLine();
	}

	// uhhh
	// gDrawData.aHeight = 48;
	// gDrawData.aWidth  = 48;
	// gDrawData.aX      = 12;
	// gDrawData.aY      = 12;
	// Render_DrawImage( gNewImageData, gDrawData ); 
	
	// ImGui::Image();

	ImGui::End();
}


void ImageList_Draw2()
{
	// int width, height;
	// Plat_GetWindowSize( width, height );
	// 
	// // uhhh
	// gDrawData.aHeight = 48;
	// gDrawData.aWidth  = 48;
	// gDrawData.aX      = 12;
	// gDrawData.aY      = 12;
	// Render_DrawImage( gNewImageData, gDrawData ); 
}


void ImageList_SetSortMode( FileSort sortMode )
{
	if ( sortMode > FileSort_Count )
	{
		printf( "ImageList: Warning - tried to set an invalid file sort mode\n" );
		return;
	}

	gSortMode = sortMode;

	if ( ImageList_InFolder() )
		ImageList_SortFiles();
}


FileSort ImageList_GetSortMode()
{
	return gSortMode;
}


void ImageList_SetPathFromFile( const fs::path& srFile )
{
	// ImageList_SetPath( fs_get_dir_name( srFile ) );
	ImageList_SetPath( srFile.parent_path() );

	// find index of the image we have loaded
	gIndex = vec_index( gImages, srFile );

	if ( gIndex == SIZE_MAX )
	{
		ImageList_LoadFiles();
		gIndex = vec_index( gImages, srFile );
	}
}


void ImageList_SetPath( const fs::path& srPath )
{
	fs::path path = fs_clean_path( srPath );

	if ( gCurrentDir.empty() )
		gCurrentDir = path;

	else if ( gCurrentDir == path )
		return;

	gCurrentDir = path;

	Plat_FolderMonitorSetPath( path );

	ImageList_LoadFiles();
}


bool ImageList_InFolder()
{
	return !gCurrentDir.empty();
}


void ImageList_LoadFiles()
{
	if ( gCurrentDir.empty() )
		return;

	wprintf( L"ImageList: Loading files in directory: %s\n", gCurrentDir.c_str() );

	gImages.clear();

	// TODO: maybe move this somewhere else later, for win32 folder view sorting support
	fs::path    file;
	DirHandle_t dir = fs_read_first( gCurrentDir, file, ReadDir_AbsPaths );

	if ( dir == nullptr )
	{
		printf( "uh\n" );
	}

	bool search = true;

	while ( search )
	{
		if ( fs_is_file( file.c_str() ) && ImageLoader_SupportsImage( file ) )
		{
			gImages.push_back( file );
		}

		search = fs_read_next( dir, file );
	}

	fs_read_close( dir );

	wprintf( L"ImageList: Finished Loading files in directory: %s\n", gCurrentDir.c_str() );

	ImageList_SortFiles();
}


int qsort_date_mod_newest( const void* spLeft , const void* spRight )
{
	const auto* pathX = (fs::path*)spLeft;
	const auto* pathY = (fs::path*)spRight;

	const auto& x     = gDateMap.at( *pathX );
	const auto& y     = gDateMap.at( *pathY );

	if ( x > y )
		return -1;
	else if ( x < y )
		return 1;

	return 0;
}


int qsort_date_mod_oldest( const void* spLeft, const void* spRight )
{
	const auto* pathX = (fs::path*)spLeft;
	const auto* pathY = (fs::path*)spRight;

	const auto& x     = gDateMap.at( *pathX );
	const auto& y     = gDateMap.at( *pathY );

	if ( x > y )
		return 1;
	else if ( x < y )
		return -1;

	return 0;
}


int qsort_date_created_newest( const void* spLeft , const void* spRight )
{
	const auto* pathX = (fs::path*)spLeft;
	const auto* pathY = (fs::path*)spRight;

	const auto& x     = gFileStatMap.at( *pathX );
	const auto& y     = gFileStatMap.at( *pathY );

	if ( x.st_ctime > y.st_ctime )
		return -1;
	else if ( x.st_ctime < y.st_ctime )
		return 1;

	return 0;
}


int qsort_date_created_oldest( const void* spLeft, const void* spRight )
{
	const auto* pathX = (fs::path*)spLeft;
	const auto* pathY = (fs::path*)spRight;

	const auto& x     = gFileStatMap.at( *pathX );
	const auto& y     = gFileStatMap.at( *pathY );

	if ( x.st_ctime > y.st_ctime )
		return 1;
	else if ( x.st_ctime < y.st_ctime )
		return -1;

	return 0;
}


void ImageList_SortFiles()
{
	Main_ShouldDrawWindow();

	if ( gDateMap.size() != gImages.size() )
		gDateMap.clear();

	if ( gFileStatMap.size() != gImages.size() )
		gFileStatMap.clear();

	switch ( gSortMode )
	{
		default:
		{
			printf( "Unimplemented Sort Mode!\n" );
			break;
		}

		case FileSort_None:
			break;

		case FileSort_DateModNewest:
		{
			if ( gDateMap.empty() )
			{
				for ( auto& file : gImages )
					gDateMap[ file ] = fs::last_write_time( file );
			}

			std::qsort( gImages.data(), gImages.size(), sizeof( fs::path ), qsort_date_mod_newest );
			break;
		}

		case FileSort_DateModOldest:
		{
			if ( gDateMap.empty() )
			{
				for ( auto& file : gImages )
					gDateMap[ file ] = fs::last_write_time( file );
			}

			std::qsort( gImages.data(), gImages.size(), sizeof( fs::path ), qsort_date_mod_oldest );
			break;
		}

		case FileSort_DateCreatedNewest:
		{
			if ( gFileStatMap.empty() )
			{
				for ( auto& file : gImages )
				{
					struct stat fileStat;
					int ret = Plat_Stat( file, &fileStat );
					gFileStatMap[ file ] = fileStat;
				}
			}

			std::qsort( gImages.data(), gImages.size(), sizeof( fs::path ), qsort_date_created_newest );
			break;
		}

		case FileSort_DateCreatedOldest:
		{
			if ( gFileStatMap.empty() )
			{
				for ( auto& file : gImages )
				{
					struct stat fileStat;
					int         ret      = Plat_Stat( file, &fileStat );
					gFileStatMap[ file ] = fileStat;
				}
			}

			std::qsort( gImages.data(), gImages.size(), sizeof( fs::path ), qsort_date_created_oldest );
			break;
		}
	}

	// OPTIONAL:
	// checks if the image currently loaded was deleted,
	// and automatically goes to the next image if so

	size_t newIndex = vec_index( gImages, gImagePath );

	// file we had loaded was deleted
	if ( newIndex == SIZE_MAX )
	{
		// are we now outside the total image count?
		if ( gIndex > gImages.size() )
		{
			gIndex = gImages.size() - 1;
			ImageView_SetImage( gImages[ gIndex ] );
		}
		else
		{
			// nope, load whatever image fell into that slot now
			ImageView_SetImage( gImages[ gIndex ] );
		}
	}
	else
	{
		// update our image index
		gIndex = newIndex;
	}
}


bool ImageList_LoadPrevImage()
{
	if ( gImages.empty() )
		return false;

	if ( gImages.size() == 1 )
		return false;

	if ( gIndex == 0 )
		gIndex = gImages.size()-1;
	else
		gIndex--;

	ImageView_SetImage( gImages[ gIndex ] );
	return true;
}


bool ImageList_LoadNextImage()
{
	if ( gImages.empty() )
		return false;

	if ( gImages.size() == 1 )
		return false;

	gIndex++;

	if ( gIndex >= gImages.size() )
		gIndex = 0;

	ImageView_SetImage( gImages[ gIndex ] );
	return true;
}


void ImageList_RemoveItem( const fs::path& srFile )
{
	size_t index = vec_index( gImages, srFile );

	if ( index == SIZE_MAX )
	{
		wprintf( L"ImageList: File not in image list: %s\n", srFile.c_str() );
		return;
	}

	vec_remove_index( gImages, index );
}

