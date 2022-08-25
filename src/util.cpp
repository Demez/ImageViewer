#include "util.h"

// #include <sys/stat.h>

#ifdef _WIN32
  #include <direct.h>
  #include <io.h>

	// get rid of the dumb windows posix depreciation warnings
  #define mkdir  _mkdir
  #define chdir  _chdir
  #define access _access
  #define getcwd _getcwd
#else
  #include <unistd.h>
  #include <dirent.h>
#endif

// ==============================================================================
// Variables

static std::string gEmptyStr;

// ==============================================================================
// Qt Functions

// ==============================================================================
// Filesystem/Path Functions

std::string        fs_get_file_ext( const std::string& path )
{
	const char* dot = strrchr( path.c_str(), '.' );
	if ( !dot || dot == path )
		return gEmptyStr;

	return dot + 1;
}

bool fs_file_exists( const std::string& path )
{
	return ( access( path.c_str(), 0 ) != -1 );
}

// ==============================================================================
// Other Functions

#ifdef _WIN32
  #include <wtypes.h>  // HWND

// https://stackoverflow.com/a/31411628/12778316
static NTSTATUS( __stdcall* NtDelayExecution )( BOOL Alertable, PLARGE_INTEGER DelayInterval )                                  = (NTSTATUS( __stdcall* )( BOOL, PLARGE_INTEGER ))GetProcAddress( GetModuleHandle( "ntdll.dll" ), "NtDelayExecution" );
static NTSTATUS( __stdcall* ZwSetTimerResolution )( IN ULONG RequestedResolution, IN BOOLEAN Set, OUT PULONG ActualResolution ) = (NTSTATUS( __stdcall* )( ULONG, BOOLEAN, PULONG ))GetProcAddress( GetModuleHandle( "ntdll.dll" ), "ZwSetTimerResolution" );

// sleep for x milliseconds
void sys_sleep( float ms )
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

#endif
