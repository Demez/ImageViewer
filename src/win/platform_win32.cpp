#include "platform.h"
#include "util.h"
#include "ui/imageview.h"
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

#include "drag_drop.h"

HWND gHWND = nullptr;

// Per Frame Events
int  gMouseDelta[2];
int  gMousePos[2];
int  gMousePosPrev[2];
int  gMouseScroll;


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

// uh
#define WM_OPENFILES ( WM_USER + 1 )

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
		case WM_MOUSEWHEEL:
		{
			WORD fwKeys  = GET_KEYSTATE_WPARAM( wParam );
			gMouseScroll += GET_WHEEL_DELTA_WPARAM( wParam );

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

		case WM_OPENFILES:
		{
			//printf( "WM_OPENFILES (custom)\n" );
			break;
		}

		case WM_SETCURSOR:
		{
			//printf( "uMsg: WM_SETCURSOR\n" );
			break;
		}

		default:
		{
			//printf( "unregistered message: %u\n", uMsg );
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


int Plat_GetMouseScroll()
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

#define IDD_MAINDLG 101

bool Plat_Init()
{
	DragDrop_Init();

	WNDCLASSEX wc     = { 0 };
	ZeroMemory( &wc, sizeof( wc ) );

	wc.cbClsExtra    = 0;
	wc.cbSize        = sizeof( wc );
	wc.cbWndExtra    = 0;
	wc.hInstance     = GetModuleHandle( NULL );
	wc.hCursor       = LoadCursor( NULL, IDC_ARROW );
	wc.hIcon         = LoadIcon( NULL, IDI_APPLICATION );
	wc.hIconSm       = LoadIcon( 0, IDI_APPLICATION );
	wc.hbrBackground = (HBRUSH)GetStockObject( DKGRAY_BRUSH );  // TODO: remove me
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
		fprintf( stderr, "Failed to create window, %Ls\n", Plat_GetError() );
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

	return true;
}


void Plat_Shutdown()
{
}


void Plat_Update()
{
	// reset per frame events
	// gMouseDelta[ 0 ] = 0;
	// gMouseDelta[ 1 ] = 0;

	gMouseScroll    = 0;

	memset( gKeyDown, false, KEY_COUNT );

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

	ImGui_ImplWin32_NewFrame();
}


void* Plat_GetWindow()
{
	return gHWND;
}


void Plat_GetWindowSize( int& srWidth, int& srHeight )
{
	RECT rect;
	GetWindowRect( gHWND, &rect );

	srWidth  = rect.right - rect.left;
	srHeight = rect.bottom - rect.top;
}


void Plat_SetWindowTitle( const std::USTRING& srTitle )
{
	SetWindowText( gHWND, srTitle.c_str() );
}


bool Plat_WindowOpen()
{
	return gHWND;
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

std::USTRING Plat_ToUnicode( const char* spStr )
{
	WCHAR nameW[ 2048 ] = { 0 };

	// the following function converts the UTF-8 filename to UTF-16 (WCHAR) nameW
	int   len           = MultiByteToWideChar( CP_UTF8, 0, spStr, -1, nameW, 2048 );

	if ( len > 0 )
		return nameW;

	return L"";
}

int Plat_ToUnicode( const char* spStr, wchar_t* spDst, int sSize )
{
	// the following function converts the UTF-8 filename to UTF-16 (WCHAR) nameW
	return MultiByteToWideChar( CP_UTF8, 0, spStr, -1, spDst, sSize );
}

std::USTRING Plat_GetModuleName()
{
	uchar buffer[ MAX_PATH ];
	GetModuleFileName( NULL, buffer, MAX_PATH );
	return buffer;
}

