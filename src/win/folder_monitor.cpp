#include "platform.h"
#include "util.h"

#include <thread>
#include <Windows.h>

#include "platform_win32.h"


static bool     gRunning         = true;
static bool     gScanPathChanged = false;
static bool     gDirChanged      = false;
static fs::path gPath;


void RefreshDirectory( LPCWSTR lpDir )
{
	// This is where you might place code to refresh your
	// directory listing, but not the subtree because it
	// would not be necessary.

	// _tprintf( TEXT( "Directory (%s) changed.\n" ), lpDir );
}


void RefreshTree( LPCWSTR lpDrive )
{
	// This is where you might place code to refresh your
	// directory listing, including the subtree.

	// _tprintf( TEXT( "Directory tree (%s) changed.\n" ), lpDrive );
}


void WatchDirectory( fs::path lpDir )
{
	DWORD  dwWaitStatus;
	HANDLE dwChangeHandles[ 2 ];
	// TCHAR  lpDrive[ 4 ];
	// TCHAR  lpFile[ _MAX_FNAME ];
	// TCHAR  lpExt[ _MAX_EXT ];

	fs::path parentPath = lpDir.parent_path();

	// _wsplitpath_s( lpDir, lpDrive, 4, NULL, 0, lpFile, _MAX_FNAME, lpExt, _MAX_EXT );

	// lpDrive[ 2 ]         = (TCHAR)'\\';
	// lpDrive[ 3 ]         = (TCHAR)'\0';

	// Watch the directory for file creation and deletion.

	dwChangeHandles[ 0 ] = FindFirstChangeNotification(
	  lpDir.c_str(),                   // directory to watch
	  FALSE,                           // do not watch subtree
	  FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE );  // watch file name and date modified changes

	if ( dwChangeHandles[ 0 ] == INVALID_HANDLE_VALUE )
	{
		printf( "ERROR: FindFirstChangeNotification function failed\n" );
		return;
	}

	// Watch the subtree for directory creation and deletion.

	dwChangeHandles[ 1 ] = FindFirstChangeNotification(
	  parentPath.c_str(),             // directory to watch
	  TRUE,                           // watch the subtree
	  FILE_NOTIFY_CHANGE_DIR_NAME );  // watch dir name changes

	if ( dwChangeHandles[ 1 ] == INVALID_HANDLE_VALUE )
	{
		printf( "ERROR: FindFirstChangeNotification function failed\n" );
		return;
	}

	// Make a final validation check on our handles.

	if ( ( dwChangeHandles[ 0 ] == NULL ) || ( dwChangeHandles[ 1 ] == NULL ) )
	{
		printf( "ERROR: Unexpected NULL from FindFirstChangeNotification\n" );
		return;
	}

	// Change notification is set. Now wait on both notification
	// handles and refresh accordingly.

	while ( !gScanPathChanged )
	{
		// Wait for notification.
		dwWaitStatus = WaitForMultipleObjects( 2, dwChangeHandles, FALSE, 500 );

		switch ( dwWaitStatus )
		{
			case WAIT_OBJECT_0:

				// A file was created, renamed, or deleted in the directory.
				// Refresh this directory and restart the notification.
				printf( "FolderMonitor: File was created, renamed, or deleted\n" );

				// RefreshDirectory( lpDir );
				gDirChanged = true;
				if ( FindNextChangeNotification( dwChangeHandles[ 0 ] ) == FALSE )
				{
					printf( "ERROR: FindNextChangeNotification function failed\n" );
					return;
				}
				break;

			case WAIT_OBJECT_0 + 1:

				// A directory was created, renamed, or deleted.
				// Refresh the tree and restart the notification.
				printf( "FolderMonitor: Directory was created, renamed, or deleted\n" );

				// RefreshTree( lpDrive );
				// gDirChanged = true;
				if ( FindNextChangeNotification( dwChangeHandles[ 1 ] ) == FALSE )
				{
					printf( "ERROR: FindNextChangeNotification function failed\n" );
					return;
				}
				break;

			case WAIT_TIMEOUT:
				break;

			default:
				printf( "ERROR: Unhandled dwWaitStatus\n" );
				break;
		}
	}
}


void FolderMonitorFunc()
{
	while ( gRunning )
	{
		if ( gScanPathChanged )
			gScanPathChanged = false;

		if ( !gPath.empty() )
			WatchDirectory( gPath.c_str() );

		Plat_Sleep( 100 );
	}
}


std::thread gFolderMonitorThread( FolderMonitorFunc );


bool FolderMonitor_Init()
{
	return true;
}


void FolderMonitor_Shutdown()
{
}


bool Plat_FolderMonitorSetPath( const fs::path& srPath )
{
	gPath = srPath;
	gScanPathChanged = true;

	return true;
}


bool Plat_FolderMonitorChanged()
{
	if ( gDirChanged )
	{
		gDirChanged = false;
		return true;
	}

	return false;
}

