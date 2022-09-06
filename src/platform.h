#pragma once

#include <string>

#include <filesystem>


#ifdef _WIN32

  // platform specific unicode character type
  using uchar = wchar_t;

  #define USTRING      wstring
  #define USTRING_VIEW wstring_view

  #define _T( str ) L##str

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

	KEY_COUNT
};


bool         Plat_Init();
void         Plat_Shutdown();
void         Plat_Update();

void         Plat_GetMouseDelta( int& xrel, int& yrel );
void         Plat_GetMousePos( int& xrel, int& yrel );
int          Plat_GetMouseScroll();

void         Plat_SetMouseCapture( bool capture );

bool         Plat_IsKeyDown( Key key );    // like typing in a text box
bool         Plat_IsKeyPressed( Key key ); // 

void*        Plat_GetWindow();
void         Plat_GetWindowSize( int& srWidth, int& srHeight );
void         Plat_SetWindowTitle( const std::USTRING& srTitle );
bool         Plat_WindowOpen();

void         Plat_Sleep( float ms );
void         Plat_BrowseToFile( const std::filesystem::path& file );

std::USTRING Plat_ToUnicode( const char* spStr );
int          Plat_ToUnicode( const char* spStr, wchar_t* spDst, int sSize );

std::USTRING Plat_GetModuleName();

// ---------------------------------------------------------------------------------------
// Drag and Drop

// setup function pointers for callbacks

// check if valid drop target
// and accept drop

void         Plat_DragDropCallback();
void         Plat_DragDropAccept( bool accept = true );


// hmmm
// IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect 
struct PlatDropTarget
{
	virtual void DragEnter() = 0;
	virtual void DragOver() = 0;
	virtual void DragLeave() = 0;
	virtual void Drop() = 0;
};

