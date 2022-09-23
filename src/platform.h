#pragma once

#include <string>

#include <filesystem>


typedef void* Module;


#ifdef _WIN32
	#define EXT_DLL _T(".dll")
	#define DLL_EXPORT __declspec(dllexport)
	#define DLL_IMPORT __declspec(dllimport)
#elif __linux__
	#define EXT_DLL ".so"
	#define DLL_EXPORT __attribute__((__visibility__("default")))
	#define DLL_IMPORT
#else
	#error "Library loading not setup for this platform"
#endif


#ifdef _WIN32

  // platform specific unicode character type
  using uchar = wchar_t;

  #define USTRING      wstring
  #define USTRING_VIEW wstring_view

  #define _T( str )    L##str

  #define stat         _stat

#else

  // platform specific unicode character type
  using uchar = char;

  #define USTRING      string
  #define USTRING_VIEW string_view

  #define _T( str ) str

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

void         Plat_SetMouseCapture( bool capture );

bool         Plat_IsKeyDown( Key key );    // like typing in a text box
bool         Plat_IsKeyPressed( Key key ); // 

void*        Plat_GetWindow();
void         Plat_GetWindowSize( int& srWidth, int& srHeight );
void         Plat_SetWindowTitle( const std::USTRING& srTitle );
void         Plat_SetMinWindowSize( int sWidth, int sHeight );
bool         Plat_WindowOpen();
bool         Plat_WindowShown(); 
bool         Plat_WindowFocused(); 

void         Plat_Sleep( float ms );

// Shell Operations
void         Plat_BrowseToFile( const std::filesystem::path& file );
void         Plat_OpenFileProperties( const std::filesystem::path& file );
bool         Plat_DeleteFile( const std::filesystem::path& file, bool showConfirm = true );

// Undo/Redo support (MOVE TO MAIN CODE, THIS IS NOT PLATFORM SPECIFIC THE WAY I DO THIS HERE)
bool         Plat_CanUndo();
bool         Plat_CanRedo();
bool         Plat_Undo();
bool         Plat_Redo();

int          Plat_Stat( const std::filesystem::path& file, struct stat* info );

std::USTRING Plat_ToUnicode( const char* spStr );
int          Plat_ToUnicode( const char* spStr, wchar_t* spDst, int sSize );

std::string  Plat_FromUnicode( const uchar* spStr );

std::USTRING Plat_GetModuleName();

Module       Plat_LoadLibrary( const uchar* path );
void         Plat_CloseLibrary( Module mod );
void*        Plat_LoadFunc( Module mod, const char* name );

// Folder Monitoring
bool         Plat_FolderMonitorSetPath( const std::filesystem::path& srPath );
bool         Plat_FolderMonitorChanged();

