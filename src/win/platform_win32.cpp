#include "platform.h"
#include "util.h"
#include "ui/imageview.h"
#include "main.h"
#include "render.h"
#include "imgui_impl_win32.h"

#include <Windows.h>

#include <WinUser.h>
#include <wtypes.h>  // HWND
#include <direct.h>
#include <io.h>
#include <fileapi.h> 
#include <handleapi.h> 
#include <shlwapi.h> 
#include <shlobj_core.h> 
#include <windowsx.h>  // GET_X_LPARAM/GET_Y_LPARAM

#include "platform_win32.h"


#define SHCNF_ACCEPT_INTERRUPTS     0x0001
#define SHCNF_ACCEPT_NON_INTERRUPTS 0x0002

#define WM_SHELLNOTIFY              ( WM_USER + 5 )
#define WM_SHELLNOTIFYRBINDIR       ( WM_USER + 6 )


HWND          gHWND = nullptr;

// Per Frame Events
int           gMouseDelta[ 2 ];
int           gMousePos[ 2 ];
int           gMousePosPrev[ 2 ];
char          gMouseScroll;

bool          gWindowShown;
bool          gWindowFocused;

int           gMinWidth             = 320;
int           gMinHeight            = 240;

LPSHELLFOLDER gpDesktop             = NULL;
LPSHELLFOLDER gpRecycleBin          = NULL;
LPITEMIDLIST  gPidlRecycleBin       = NULL;


// Ordered in the same order of the enums
int  gKeyToVK[ KEY_COUNT ] = {
	 VK_LBUTTON,
	 VK_RBUTTON,
	 VK_MBUTTON,

	 VK_ESCAPE,
	 VK_RETURN,
	 VK_SHIFT,

	 VK_LEFT,
	 VK_RIGHT,
	 VK_UP,
	 VK_DOWN,

	 VK_DELETE,
};


Key VKToKey( int vkey )
{
	switch ( vkey )
	{
		default: return KEY_COUNT;

		case VK_LBUTTON: return K_LBUTTON;
		case VK_RBUTTON: return K_RBUTTON;
		case VK_MBUTTON: return K_MBUTTON;

		case VK_ESCAPE:  return K_ESCAPE;
		case VK_RETURN:  return K_ENTER;
		case VK_SHIFT:   return K_SHIFT;

		case VK_LEFT:    return K_LEFT;
		case VK_RIGHT:   return K_RIGHT;
		case VK_UP:      return K_UP;
		case VK_DOWN:    return K_DOWN;

		case VK_DELETE:  return K_DELETE;
	}
}


bool gKeyDown[ KEY_COUNT ] = {};
bool gKeyPressed[ KEY_COUNT ] = {};


void SetKeyState( int winKey, bool down )
{
	Key key = VKToKey( winKey );

	if ( key != KEY_COUNT )
	{
		gKeyDown[ key ]    = down;
		gKeyPressed[ key ] = down;
	}
}


const uchar* Plat_GetError()
{
	DWORD       errorID = GetLastError();

	static uchar message[ 512 ];
	memset( message, 512, 0 );

	if ( errorID == 0 )
		return message;  // No error message

	LPSTR strErrorMessage = NULL;

	FormatMessage(
	  FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_ARGUMENT_ARRAY | FORMAT_MESSAGE_ALLOCATE_BUFFER,
	  NULL,
	  errorID,
	  0,
	  (LPWSTR)&strErrorMessage,
	  0,
	  NULL );

	//std::string message;
	//message.resize(512);
	//snprintf( message.data(), 512, "Win32 API Error %d: %s", errorID, strErrorMessage );

	_snwprintf( message, 512, L"Win32 API Error %d: %hs", errorID, strErrorMessage );

	// Free the Win32 string buffer.
	LocalFree( strErrorMessage );

	return message;
}


#if 0
bool RawInput_Init()
{
	RAWINPUTDEVICE Rid[ 1 ];

	Rid[ 0 ].usUsagePage = 0x01;            // HID_USAGE_PAGE_GENERIC
	Rid[ 0 ].usUsage     = 0x02;            // HID_USAGE_GENERIC_MOUSE
	Rid[ 0 ].dwFlags     = RIDEV_NOLEGACY;  // adds mouse and also ignores legacy mouse messages
	// Rid[ 0 ].hwndTarget  = gHWND;
	Rid[ 0 ].hwndTarget  = 0;

	if ( RegisterRawInputDevices( Rid, 1, sizeof( Rid[ 0 ] ) ) == FALSE )
	{
		fprintf( stderr, "Failed to create window, %ws\n", Plat_GetError() );
		return false;
	}
	return true;
}
#endif


extern IMGUI_API LRESULT ImGui_ImplWin32_WndProcHandler( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );

// LRESULT CALLBACK WindowProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
LRESULT WindowProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	ImGui_ImplWin32_WndProcHandler( hwnd, uMsg, wParam, lParam );

	switch ( uMsg )
	{
		case WM_NCDESTROY:
		{
			gHWND = nullptr;
			break;
		}
		case WM_MOUSEMOVE:
		{
			gMousePos[ 0 ]  = GET_X_LPARAM( lParam );
			gMousePos[ 1 ]  = GET_Y_LPARAM( lParam );
			break;
		}
#if 0
		case WM_INPUT:
		{
			RAWINPUT  input;
			UINT      szData = sizeof( input ), szHeader = sizeof( RAWINPUTHEADER );
			HRAWINPUT handle = reinterpret_cast< HRAWINPUT >( lParam );

			GetRawInputData( handle, RID_INPUT, &input, &szData, szHeader );
			if ( input.header.dwType == RIM_TYPEMOUSE )
			{
				// Here input.data.mouse.ulRawButtons is 0 at all times.
			}

			printf( "WM_INPUT\n" );
			break;
		}
#endif
		case WM_MOUSEWHEEL:
		{
			WORD fwKeys  = GET_KEYSTATE_WPARAM( wParam );
			gMouseScroll += GET_WHEEL_DELTA_WPARAM( wParam ) / 120;
			break;
		}

		case WM_LBUTTONDOWN:
		{
			SetKeyState( VK_LBUTTON, true );
			break;						  
		}
		case WM_MBUTTONDOWN:			  
		{								  
			SetKeyState( VK_MBUTTON, true );
			break;
		}
		case WM_RBUTTONDOWN:
		{
			SetKeyState( VK_RBUTTON, true );
			break;
		}
		case WM_LBUTTONUP:
		{
			SetKeyState( VK_LBUTTON, false );
			break;
		}
		case WM_MBUTTONUP:
		{
			SetKeyState( VK_MBUTTON, false );
			break;
		}
		case WM_RBUTTONUP:
		{
			SetKeyState( VK_RBUTTON, false );
			break;
		}

		case WM_SYSKEYDOWN:
		case WM_KEYDOWN:
		{
			SetKeyState( wParam, true );
			break;
		}
		case WM_SYSKEYUP:
		case WM_KEYUP:
		{
			SetKeyState( wParam, false );
			break;
		}

		case WM_GETMINMAXINFO:
		{
			// Minimum window width and height
			MINMAXINFO* minMaxInfo = (MINMAXINFO*)lParam;
			minMaxInfo->ptMinTrackSize.x = gMinWidth;
			minMaxInfo->ptMinTrackSize.y = gMinHeight;
			break;
		}

		case WM_SETCURSOR:
		{
			//printf( "uMsg: WM_SETCURSOR\n" );
			break;
		}

		case WM_SHOWWINDOW:
		{
			gWindowShown = ( wParam == TRUE );
			break;
		}

		case WM_NCACTIVATE:
		{
			gWindowFocused = ( wParam == TRUE );
			break;
		}

		default:
		{
			// printf( "unregistered message: %u\n", uMsg );
			break;
		}
	}

	// drawing events
	switch ( uMsg )
	{
		case WM_MOUSEMOVE:
		case WM_MOUSEWHEEL:
		case WM_LBUTTONDOWN:
		case WM_MBUTTONDOWN:
		case WM_RBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_MBUTTONUP:
		case WM_RBUTTONUP:
		case WM_SYSKEYDOWN:
		case WM_KEYDOWN:
		case WM_SYSKEYUP:
		case WM_KEYUP:
		case WM_SHOWWINDOW:
		case WM_NCACTIVATE:
		{
			Main_ShouldDrawWindow();
			break;
		}

		case WM_PAINT:
		{
			// hack
			static bool firstTime = true;
			if ( firstTime )
			{
				firstTime = false;
				break;
			}

			int width, height;
			Plat_GetWindowSize( width, height );
			Render_SetResolution( width, height );

			Render_Reset();
			ImGui_ImplWin32_NewFrame();
			Main_WindowDraw();
			break;
		}
	}

	return DefWindowProc( hwnd, uMsg, wParam, lParam );
}


void Plat_GetMouseDelta( int& xrel, int& yrel )
{
	xrel = gMouseDelta[ 0 ];
	yrel = gMouseDelta[ 1 ];
}


void Plat_GetMousePos( int& xrel, int& yrel )
{
	// also could do GetCursorPos();
	xrel = gMousePos[ 0 ];
	yrel = gMousePos[ 1 ];
}


char Plat_GetMouseScroll()
{
	return gMouseScroll;
}


void Plat_SetMouseCapture( bool capture )
{

}


bool Plat_IsKeyDown( Key key )
{
	if ( key > KEY_COUNT || key < 0 )
		return false;

	return gKeyDown[ key ];
}


bool Plat_IsKeyPressed( Key key )
{
	if ( key > KEY_COUNT || key < 0 )
		return false;

	// return GetAsyncKeyState( gKeyToVK[ key ] );
	// short what = GetKeyState( gKeyToVK[ key ] );

	return gKeyPressed[ key ];
}


bool UndoSys_Init()
{
	HRESULT hr = SHGetDesktopFolder( &gpDesktop );
	if ( hr != S_OK )
	{
		printf( "UndoSys_Init(): Failed to get Desktop Folder!\n" );
		return false;
	}

	hr = SHGetSpecialFolderLocation( 0, CSIDL_BITBUCKET, &gPidlRecycleBin );
	if ( hr != S_OK )
	{
		printf( "UndoSys_Init(): Failed to get Recycle Bin Location!\n" );
		return false;
	}

	hr = gpDesktop->BindToObject( gPidlRecycleBin, NULL, IID_IShellFolder, (LPVOID*)&gpRecycleBin );
	if ( hr != S_OK )
	{
		printf( "UndoSys_Init(): Failed to Bind to Recycle Bin!\n" );
		return false;
	}

	// STRRET strRet;
	// hr = gpDesktop->GetDisplayNameOf( gPidlRecycleBin, SHGDN_NORMAL, &strRet );
	// 
	// if ( hr != S_OK )
	// {
	// 	printf( "UndoSys_Init(): Failed to get Recycle Bin Display Name!\n" );
	// 	return false;
	// }

	return true;
}


bool UndoSys_Shutdown()
{
	return true;
}


bool Plat_Init()
{
	// allow for wchar_t to be printed in console
	setlocale( LC_ALL, "" );

	if ( !UndoSys_Init() )
	{
		printf( "Plat_Init(): Failed to Initialize UndoManager\n" );
		return false;
	}

	if ( OleInitialize( NULL ) != S_OK )
	{
		printf( "Plat_Init(): Failed to Initialize Ole\n" );
		return false;
	}

	WNDCLASSEX wc     = { 0 };
	ZeroMemory( &wc, sizeof( wc ) );

	wc.cbClsExtra    = 0;
	wc.cbSize        = sizeof( wc );
	wc.cbWndExtra    = 0;
	wc.hInstance     = GetModuleHandle( NULL );
	wc.hCursor       = LoadCursor( NULL, IDC_ARROW );
	wc.hIcon         = LoadIcon( NULL, IDI_APPLICATION );
	wc.hIconSm       = LoadIcon( 0, IDI_APPLICATION );
	wc.hbrBackground = (HBRUSH)GetStockObject( BLACK_BRUSH );  // does this affect perf? i hope not
	wc.style         = CS_HREDRAW | CS_VREDRAW;  // redraw if size changes
	wc.lpszClassName = L"demez_imgviewer";
	wc.lpszMenuName  = 0;
	wc.lpfnWndProc   = WindowProc;

	ATOM _Atom( RegisterClassEx( &wc ) );

	if ( _Atom == 0 )
	{
		// somehow not running on windows nt?
		return false;
	}

	const LPTSTR _ClassName( MAKEINTATOM( _Atom ) );

	LPCWSTR lpWindowName = L"Demez Image Viewer";

	DWORD   dwStyle      = WS_VISIBLE | WS_OVERLAPPEDWINDOW | WS_EX_CONTROLPARENT;
	// DWORD   dwExStyle    = WS_EX_ACCEPTFILES;
	DWORD   dwExStyle    = 0;

	gHWND                = CreateWindowEx(
					 dwExStyle,
					 _ClassName,
					 lpWindowName,
					 dwStyle,
					 CW_USEDEFAULT,
					 CW_USEDEFAULT,
					 // 1280, 720,
					 CW_USEDEFAULT, CW_USEDEFAULT,
					 NULL, NULL,
					 GetModuleHandle( NULL ),
					 nullptr );

	// UpdateWindow( gHWND );

	if ( !gHWND )
	{
		fprintf( stderr, "Failed to create window, %ws\n", Plat_GetError() );
		return false;
	}

	if ( !DragDrop_Register( gHWND ) )
	{
		printf( "Failed to register Win32 Drag and Drop!\n" );
	}

	if ( !ImGui_ImplWin32_Init( gHWND ) )
	{
		fputs( "Failed to init ImGui for Win32\n", stderr );
		return false;
	}

	// if ( !RawInput_Init() )
	// {
	// 	return false;
	// }

	return true;
}


void Plat_Shutdown()
{
	UndoSys_Shutdown();
	FolderMonitor_Shutdown();
}


void Plat_Update()
{
	// reset per frame events
	// gMouseDelta[ 0 ] = 0;
	// gMouseDelta[ 1 ] = 0;

	gMouseScroll = 0;

	memset( gKeyDown, false, KEY_COUNT );

	ImGui_ImplWin32_NewFrame();

	MSG msg = {};
	// NOTE: if you put gHWID in, drag and drop's just break horribly
	// it will freeze the source window, and this window only updates when resized or moved around after
	// until all drag events are processed, then it's back to normal
	// if ( PeekMessage( &msg, 0, 0, 0, PM_REMOVE ) )
	while ( PeekMessage( &msg, 0, 0, 0, PM_REMOVE ) )
	{
		TranslateMessage( &msg );
		DispatchMessage( &msg );
	}
	
	gMouseDelta[ 0 ]   = gMousePos[ 0 ] - gMousePosPrev[ 0 ];
	gMouseDelta[ 1 ]   = gMousePos[ 1 ] - gMousePosPrev[ 1 ];

	gMousePosPrev[ 0 ] = gMousePos[ 0 ];
	gMousePosPrev[ 1 ] = gMousePos[ 1 ];

	// ImGui_ImplWin32_NewFrame();
}


void* Plat_GetWindow()
{
	return gHWND;
}


void Plat_GetWindowSize( int& srWidth, int& srHeight )
{
	RECT rect;
	// GetWindowRect( gHWND, &rect );
	GetClientRect( gHWND, &rect );

	// https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getwindowinfo
	WINDOWINFO windowInfo;
	windowInfo.cbSize = sizeof( WINDOWINFO );
	GetWindowInfo( gHWND, &windowInfo );

	srWidth  = rect.right - rect.left;
	srHeight = rect.bottom - rect.top;

	// srWidth  = windowInfo.rcClient.right - windowInfo.rcClient.left;
	// srHeight = windowInfo.rcClient.bottom - windowInfo.rcClient.top;
}


void Plat_GetClientSize( int& srWidth, int& srHeight )
{
	RECT rect;
	GetClientRect( gHWND, &rect );

	srWidth  = rect.right - rect.left;
	srHeight = rect.bottom - rect.top;
}


void Plat_SetWindowTitle( const std::USTRING& srTitle )
{
	SetWindowText( gHWND, srTitle.c_str() );
}


void Plat_SetMinWindowSize( int sWidth, int sHeight )
{
	gMinWidth = sWidth;
	gMinWidth = sHeight;
}


bool Plat_WindowOpen()
{
	return gHWND;
}


bool Plat_WindowShown()
{
	return gWindowShown;
}


bool Plat_WindowFocused()
{
	return gWindowFocused;
}


// https://stackoverflow.com/a/31411628/12778316
static NTSTATUS( __stdcall* NtDelayExecution )( BOOL Alertable, PLARGE_INTEGER DelayInterval )                                  = (NTSTATUS( __stdcall* )( BOOL, PLARGE_INTEGER ))GetProcAddress( GetModuleHandle( L"ntdll.dll" ), "NtDelayExecution" );
static NTSTATUS( __stdcall* ZwSetTimerResolution )( IN ULONG RequestedResolution, IN BOOLEAN Set, OUT PULONG ActualResolution ) = (NTSTATUS( __stdcall* )( ULONG, BOOLEAN, PULONG ))GetProcAddress( GetModuleHandle( L"ntdll.dll" ), "ZwSetTimerResolution" );

// sleep for x milliseconds
void Plat_Sleep( float ms )
{
	static bool once = true;
	if ( once )
	{
		ULONG actualResolution;
		ZwSetTimerResolution( 1, true, &actualResolution );
		once = false;
	}

	LARGE_INTEGER interval{};
	interval.QuadPart = -1 * (int)( ms * 10000.0f );
	NtDelayExecution( false, &interval );
}

void Plat_BrowseToFile( const fs::path& file )
{
	ITEMIDLIST* pidl = ILCreateFromPath( file.c_str() );
	if ( pidl )
	{
		SHOpenFolderAndSelectItems( pidl, 0, 0, 0 );
		ILFree( pidl );
	}
}

void Plat_OpenFileProperties( const fs::path& file )
{
	if ( !SHObjectProperties( gHWND, SHOP_FILEPATH, file.c_str(), NULL ) )
	{
		wprintf( L"Failed to open File Properties for file: %s\n", file.c_str() );
	}
}


int Plat_Stat( const std::filesystem::path& file, struct stat* info )
{
	return _wstat( file.c_str(), info );
}


std::wstring Plat_ToWideChar( const char* spStr )
{
	WCHAR nameW[ 2048 ] = { 0 };

	// the following function converts the UTF-8 filename to UTF-16 (WCHAR) nameW
	int   len           = MultiByteToWideChar( CP_UTF8, 0, spStr, -1, nameW, 2048 );

	if ( len > 0 )
		return nameW;

	return L"";
}

int Plat_ToWideChar( const char* spStr, wchar_t* spDst, int sSize )
{
	// the following function converts the UTF-8 filename to UTF-16 (WCHAR) nameW
	return MultiByteToWideChar( CP_UTF8, 0, spStr, -1, spDst, sSize );
}


std::string Plat_ToMultiByte( const wchar_t* spStr )
{
	char name[ 2048 ] = { 0 };

	// the following function converts the UTF-8 filename to UTF-16 (WCHAR) nameW
	int  len          = WideCharToMultiByte( CP_UTF8, 0, spStr, -1, name, 2048, NULL, NULL );

	return name;
}


int Plat_ToMultiByte( const wchar_t* spStr, char* spDst, int sSize )
{
	return WideCharToMultiByte( CP_UTF8, 0, spStr, -1, spDst, sSize, NULL, NULL );
}


int Plat_ToMultiByte( const std::wstring& srStr, std::string& srDst )
{
	srDst.resize( srStr.size() );
	return WideCharToMultiByte( CP_UTF8, 0, srStr.data(), -1, srDst.data(), srStr.size(), NULL, NULL );
}


std::USTRING Plat_GetModuleName()
{
	uchar buffer[ MAX_PATH ];
	GetModuleFileName( NULL, buffer, MAX_PATH );
	return buffer;
}


Module Plat_LoadLibrary( const uchar* path )
{
	return (Module)LoadLibrary( path );
}

void Plat_CloseLibrary( Module mod )
{
	FreeLibrary( (HMODULE)mod );
}

void* Plat_LoadFunc( Module mod, const char* name )
{
	return GetProcAddress( (HMODULE)mod, name );
}


// ---------------------------------------------------------------------


static BOOL ExecCommand( LPITEMIDLIST pidl, std::string_view cmd )
{
	BOOL          bReturn  = FALSE;
	LPCONTEXTMENU pCtxMenu = NULL;

	HRESULT       hr       = gpRecycleBin->GetUIObjectOf( gHWND, 1, (LPCITEMIDLIST*)&pidl, IID_IContextMenu, NULL, (LPVOID*)&pCtxMenu );

	if ( SUCCEEDED( hr ) )
	{
		UINT  uiID        = UINT( -1 );
		UINT  uiCommand   = 0;
		UINT  uiMenuFirst = 1;
		UINT  uiMenuLast  = 0x00007FFF;
		HMENU hmenuCtx;
		int   iMenuPos = 0;
		int   iMenuMax = 0;
		TCHAR szMenuItem[ 128 ];
		TCHAR szTrace[ 512 ];
		char  verb[ MAX_PATH ];

		hmenuCtx = CreatePopupMenu();
		hr       = pCtxMenu->QueryContextMenu( hmenuCtx, 0, uiMenuFirst, uiMenuLast, CMF_NORMAL );

		iMenuMax = GetMenuItemCount( hmenuCtx );
		wsprintf( szTrace, _T("Nb Items added to the menu %d\n\n"), iMenuMax );

		for ( iMenuPos = 0; iMenuPos < iMenuMax; iMenuPos++ )
		{
			GetMenuStringW( hmenuCtx, iMenuPos, szMenuItem, sizeof( szMenuItem ), MF_BYPOSITION );

			uiID = GetMenuItemID( hmenuCtx, iMenuPos );

			if ( ( uiID == -1 ) || ( uiID == 0 ) )
			{
				printf( "No Verb found for entry %d\n", uiID );
				continue;
			}

			// When we'll have found the right command, we'll be obliged to perform a
			// 'uiID - 1' else the verbs are going to be be misaligned from they're
			// real ID
			hr = pCtxMenu->GetCommandString( uiID - 1, GCS_VERBA, NULL, verb, sizeof( verb ) );
			if ( FAILED( hr ) )
			{
				verb[ 0 ] = TCHAR( '\0' );
			}
			else
			{
				if ( verb == cmd )
				{
					uiCommand = uiID - 1;
					break;
				}
			}
		}

		if ( (UINT)-1 != uiCommand )
		{
			// could use CMINVOKECOMMANDINFOEX for the W strings it contains
			CMINVOKECOMMANDINFO cmi;

			ZeroMemory( &cmi, sizeof( CMINVOKECOMMANDINFO ) );
			cmi.cbSize       = sizeof( CMINVOKECOMMANDINFO );
			cmi.fMask        = CMIC_MASK_FLAG_NO_UI;
			cmi.hwnd         = gHWND;
			cmi.lpParameters = NULL;
			cmi.lpDirectory  = NULL;
			cmi.lpVerb       = MAKEINTRESOURCEA( uiCommand );
			cmi.nShow        = SW_SHOWNORMAL;
			cmi.dwHotKey     = NULL;
			cmi.hIcon        = NULL;

			hr               = pCtxMenu->InvokeCommand( &cmi );

			if ( SUCCEEDED( hr ) )
			{
				bReturn = TRUE;
			}
		}
	}

	pCtxMenu->Release();

	return bReturn;
}


bool Plat_RestoreFile( const fs::path& file )
{
	HRESULT      hr        = S_OK;
	LPITEMIDLIST pidl      = NULL;
	IEnumIDList* enumFiles = NULL;

	// Iterate through list
	gpRecycleBin->EnumObjects( gHWND, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS | SHCONTF_INCLUDEHIDDEN, &enumFiles );

	if ( !SUCCEEDED( hr ) )
		return false;

	STRRET strRet;
	bool   foundFile = false;
	while ( enumFiles->Next( 1, &pidl, NULL ) != S_FALSE )
	{
		hr = gpRecycleBin->GetDisplayNameOf( pidl, SHGDN_NORMAL, &strRet );

		if ( !SUCCEEDED( hr ) )
			continue;

		if ( strRet.pOleStr == file )
		{
			foundFile = true;
			break;
		}
	}

	if ( !foundFile )
		return false;

#if 0
	// Remove from system clipboard history
	{
		// if ( TRUE == m_ChkRBin )
		{
			SHChangeNotifyEntry entry;
			LPITEMIDLIST        ppidl;
			// pfSHChangeNotifyRegister SHChangeNotifyRegister;

			// SHChangeNotifyRegister = (pfSHChangeNotifyRegister)GetProcAddress( m_hShell32,
			//                                                                    MAKEINTRESOURCE( 2 ) );

			if ( SHGetSpecialFolderLocation( gHWND, CSIDL_BITBUCKET, &ppidl ) != NOERROR )
			{
				printf( "GetSpecialFolder problem\n" );
			}

			entry.pidl       = ppidl;
			entry.fRecursive = TRUE;
			ULONG m_hNotifyRBin  = SHChangeNotifyRegister( gHWND,
				                                            SHCNF_ACCEPT_INTERRUPTS | SHCNF_ACCEPT_NON_INTERRUPTS,
				                                            SHCNE_ALLEVENTS,
				                                            WM_SHELLNOTIFY,  // Message that would be sent by the Shell
				                                            1,
			                                               &entry );
			if ( NULL == m_hNotifyRBin )
			{
				printf( "Warning: Change Register Failed for RecycleBin\n" );
			}
		}
		// else
		// {
		// 	pfSHChangeNotifyDeregister SHChangeNotifyDeregister = (pfSHChangeNotifyDeregister)GetProcAddress( m_hShell32,
		// 	                                                                                                  MAKEINTRESOURCE( 4 ) );
		// 
		// 	if ( NULL != SHChangeNotifyDeregister )
		// 	{
		// 		BOOL bDeregister = SHChangeNotifyDeregister( m_hNotifyRBin );
		// 	}
		// }
	}
#endif

	return ExecCommand( pidl, "undelete" );
}


bool Plat_DeleteFile( const fs::path& file, bool showConfirm, bool addToUndo )
{
	TCHAR Buffer[ 2048 + 4 ];

	wcsncpy_s( Buffer, 2048 + 4, file.c_str(), 2048 );
	Buffer[ wcslen( Buffer ) + 1 ] = 0;  //Double-Null-Termination

	SHFILEOPSTRUCT s;
	s.hwnd                  = gHWND;
	s.wFunc                 = FO_DELETE;
	s.pFrom                 = Buffer;
	s.pTo                   = NULL;
	s.fFlags                = FOF_ALLOWUNDO;
	s.fAnyOperationsAborted = false;
	s.hNameMappings         = NULL;
	s.lpszProgressTitle     = NULL;

	if ( !showConfirm )
		s.fFlags |= FOF_SILENT;

	int rc = SHFileOperation( &s );

	if ( rc != 0 )
	{
		wprintf( L"Failed To Delete File: %s\n", file.c_str() );
		return false;
	}

	wprintf( L"Deleted File: %s\n", file.c_str() );

	if ( !addToUndo )
		return true;

	UndoOperation* undo = UndoSys_AddUndo();

	if ( undo == nullptr )
	{
		wprintf( L"DeleteFile: Failed to add to undo stack?\n" );
		return true;
	}

	auto deleteData   = new UndoData_Delete;
	deleteData->aPath = file;
	undo->apData      = deleteData;

	return true;
}


void Plat_RenameFile( const fs::path& srPath, const fs::path& srNewName, bool addToUndo )
{

}

