#pragma once

#include <string>

#include <filesystem>


typedef void* Module;


#ifdef _WIN32
	#define EXT_DLL _T(".dll")
	#define DLL_EXPORT __declspec(dllexport)
	#define DLL_IMPORT __declspec(dllimport)
#elif __unix__
	#define EXT_DLL ".so"
	#define DLL_EXPORT __attribute__((__visibility__("default")))
	#define DLL_IMPORT
#else
	#error "Library loading not setup for this platform"
#endif


#ifdef _WIN32

  // platform specific unicode character type
  using uchar = wchar_t;

  #define ustring      wstring
  #define ustring_view wstring_view

  #define USTRING      wstring
  #define USTRING_VIEW wstring_view

  #define _T( str )    L##str

  #define stat         _stat
  #define unprintf     _snwprintf

#else

  // platform specific unicode character type
  using uchar = char;

  #define ustring      string
  #define ustring_view string_view

  #define USTRING      string
  #define USTRING_VIEW string_view

  #define _T( str )    str

  #define unprintf     snprintf

#endif

  
#if 0
enum KeyState
  {
	KeyState_Invalid      = ( 0 << 0 ),
	KeyState_Released     = ( 1 << 0 ),
	KeyState_Pressed      = ( 1 << 1 ),
	KeyState_JustReleased = ( 1 << 2 ),
	KeyState_JustPressed  = ( 1 << 3 ),
};

typedef unsigned char KeyState;
#endif


enum Key
{
	K_LBUTTON,
	K_RBUTTON,
	K_MBUTTON,

	K_ESCAPE,
	K_ENTER,
	K_SHIFT,

	K_LEFT,
	K_RIGHT,
	K_UP,
	K_DOWN,

	K_DELETE,

	KEY_COUNT
};


const uchar* Plat_GetError();

void         Plat_ShowMessageBox();  // NOT IMPLEMENTED YET

bool         Plat_Init();
void         Plat_Shutdown();
void         Plat_Update();

void         Plat_GetMouseDelta( int& xrel, int& yrel );
void         Plat_GetMousePos( int& xrel, int& yrel );
char         Plat_GetMouseScroll();

bool         Plat_IsKeyDown( Key key );    // like typing in a text box
bool         Plat_IsKeyPressed( Key key ); // 

void*        Plat_GetWindow();
void         Plat_GetWindowSize( int& srWidth, int& srHeight );
void         Plat_SetWindowTitle( const uchar* spTitle );
void         Plat_SetMinWindowSize( int sWidth, int sHeight );
bool         Plat_WindowOpen();
bool         Plat_WindowShown(); 
bool         Plat_WindowFocused(); 
bool         Plat_WindowMinimized(); 

void         Plat_Sleep( float ms );

// Shell Operations
void         Plat_BrowseToFile( const std::filesystem::path& file );
void         Plat_OpenFileProperties( const std::filesystem::path& file );
bool         Plat_DeleteFile( const std::filesystem::path& file, bool showConfirm = true, bool addToUndo = true );
bool         Plat_RestoreFile( const std::filesystem::path& file );

int          Plat_Stat( const std::filesystem::path& file, struct stat* info );

std::wstring Plat_ToWideChar( const char* spStr );
int          Plat_ToWideChar( const char* spStr, wchar_t* spDst, int sSize );

std::string  Plat_ToMultiByte( const wchar_t* spStr );
int          Plat_ToMultiByte( const wchar_t* spStr, char* spDst, int sSize );
int          Plat_ToMultiByte( const std::wstring& srStr, std::string& srDst );

// std::USTRING Plat_GetModuleName();
std::ustring Plat_GetModuleName();

Module       Plat_LoadLibrary( const uchar* path );
void         Plat_CloseLibrary( Module mod );
void*        Plat_LoadFunc( Module mod, const char* name );

// Folder Monitoring
bool         Plat_FolderMonitorSetPath( const std::filesystem::path& srPath );
bool         Plat_FolderMonitorChanged();

