#include "util.h"

#include <vector>
#include <filesystem>
#include <unordered_map>
#include <fstream>

// #include <sys/stat.h>

#ifdef _WIN32
  #include <direct.h>
  #include <io.h>
  #include <fileapi.h> 
  #include <handleapi.h> 
  #include <shlwapi.h> 
  #include <shlobj_core.h> 

	// get rid of the dumb windows posix depreciation warnings
  #define mkdir  _mkdir
  #define chdir  _chdir
  #define access _access
  #define waccess _waccess
  #define getcwd _getcwd
  #define stat   _stat
  #define wstat  _wstat
  constexpr char PATH_SEP = '\\';
  constexpr wchar_t PATH_SEPW = L'\\';
#else
  #include <unistd.h>
  #include <dirent.h>
  constexpr char PATH_SEP = '/';
constexpr wchar_t PATH_SEPW = L'/';
#endif


namespace fs = std::filesystem;


// ==============================================================================
// Variables

static std::string gEmptyStr;
static std::wstring gEmptyWStr;

// ==============================================================================
// Qt Functions

// ==============================================================================
// Filesystem/Path Functions

std::wstring fs_get_file_ext( const fs::path& path )
{
	// const char* dot = strrchr( path.c_str(), '.' );
	const wchar_t* dot = wcsrchr( path.c_str(), '.' );
	if ( !dot || dot == path )
		return gEmptyWStr;

	return dot + 1;
}

std::string fs_get_dir_name( const std::string& path )
{
	size_t i = path.length();
	for ( ; i > 0; i-- )
	{
		if ( path[ i ] == '/' || path[ i ] == '\\' )
			break;
	}

	return path.substr( 0, i );
}

bool fs_file_exists( const wchar_t* path )
{
	return ( waccess( path, 0 ) != -1 );
}

std::wstring fs_clean_path( const std::wstring &path )
{
    std::vector< std::wstring > v;

    int n = path.length();
    std::wstring out;

    std::wstring root;

    // if ( fs_is_absolute( path ) )
    {
#ifdef _WIN32

#elif __unix__
        root = "/";
#endif
    }

    for ( int i = 0; i < n; i++ )
    {
        std::wstring dir;
        // forming the current directory.

        while ( i < n && (path[i] != '/' && path[i] != '\\') )
        {
            dir += path[i];
            i++;
        }

        // if ".." , we pop.
        if ( dir == L".." )
        {
            if ( !v.empty() )
                v.pop_back();
        }
        else if ( dir != L"." && !dir.empty() )
        {
            // push the current directory into the vector.
            v.push_back( dir );
        }
    }
    
    // build the cleaned path
    size_t len = v.size();
    for ( size_t i = 0; i < len; i++ )
    {
        out += (i+1 == len) ? v[i] : v[i] + PATH_SEPW;
    }

    // vector is empty
    if ( out.empty() )
        return root;  // PATH_SEP
    
    return root + out;
}

bool fs_is_dir( const wchar_t* path )
{
	struct stat s;

	if ( wstat( path, &s ) == 0 )
		return ( s.st_mode & S_IFDIR );

	return false;
}

bool fs_is_file( const wchar_t* path )
{
	struct stat s;

	if ( wstat( path, &s ) == 0 )
		return ( s.st_mode & S_IFREG );

	return false;
}


struct SearchParams
{
	ReadDirFlags flags;
	fs::path     rootPath;
};

std::unordered_map< DirHandle_t, SearchParams > gSearchParams;


/* Read the first file in a Directory  */
DirHandle_t fs_read_first( const fs::path& path, fs::path& file, ReadDirFlags flags )
{
#ifdef _WIN32
	fs::path        readPath = path / L"*";  // NOTE: do "*.*" for files only

	WIN32_FIND_DATA ffd;
	HANDLE          hFind = INVALID_HANDLE_VALUE;

	hFind = FindFirstFile( readPath.c_str(), &ffd );

	if ( INVALID_HANDLE_VALUE == hFind )
		return nullptr;

	gSearchParams[ hFind ] = { flags, path };

	if ( flags & ReadDir_AbsPaths )
		// file = fs_clean_path( path + PATH_SEP + ffd.cFileName );
		// file = fs_clean_path( path / ffd.cFileName );
		file = path / ffd.cFileName;

	else
		file = ffd.cFileName;

	return hFind;

#elif __unix__
	return nullptr;
#else
#endif
}

/* Get the next file in the directory */
bool fs_read_next( DirHandle_t dirh, fs::path& file )
{
#ifdef _WIN32
	WIN32_FIND_DATA ffd;

	if ( !FindNextFile( dirh, &ffd ) )
		return false;

	auto it = gSearchParams.find( dirh );
	if ( it != gSearchParams.end() )
	{
		if ( it->second.flags & ReadDir_AbsPaths )
			// file = fs_clean_path( it->second.rootPath + PATH_SEP + ffd.cFileName );
			file = it->second.rootPath / ffd.cFileName;
		else
			file = ffd.cFileName;

		return true;
	}
	else
	{
		printf( "[Filesystem] handle not in handle list??\n" );
		return false;
	}

#elif __unix__
	// somehow do stuff with wildcards
	// probably have to have an std::unordered_map< handles, search string > variable so you can do a comparison here
#else
#endif

	return false;
}

/* Close a Directory */
bool fs_read_close( DirHandle_t dirh )
{
#ifdef _WIN32
	return FindClose( dirh );

#elif __unix__
	return closedir( (DIR*)dirh );

#else
#endif
}


std::vector< char > fs_read_file( const fs::path& srFilePath )
{
	/* Open file.  */
	std::ifstream file( srFilePath.wstring(), std::ios::ate | std::ios::binary );
	if ( !file.is_open() )
	{
		fwprintf( stderr, L"Failed to open file: %s", srFilePath.c_str() );
		return {};
	}

	int                 fileSize = (int)file.tellg();
	std::vector< char > buffer( fileSize );
	file.seekg( 0 );

	/* Read contents.  */
	file.read( buffer.data(), fileSize );
	file.close();

	return buffer;
}


bool fs_read_file( const fs::path& srFilePath, std::vector< char >& srData )
{
	/* Open file.  */
	std::ifstream file( srFilePath.wstring(), std::ios::ate | std::ios::binary );
	if ( !file.is_open() )
	{
		fwprintf( stderr, L"Failed to open file: %s", srFilePath.c_str() );
		return false;
	}

	int fileSize = (int)file.tellg();
	srData.resize( fileSize );
	file.seekg( 0 );

	/* Read contents.  */
	file.read( srData.data(), fileSize );
	file.close();

	return true;
}


std::vector< char > fs_read_bytes( const fs::path& srFilePath, int bytes )
{
	/* Open file.  */
	std::ifstream file( srFilePath.wstring(), std::ios::ate | std::ios::binary );
	if ( !file.is_open() )
	{
		fwprintf( stderr, L"Failed to open file: %s", srFilePath.c_str() );
		return {};
	}

	std::vector< char > buffer( bytes );

	/* Read contents.  */
	file.seekg( 0 );
	file.read( buffer.data(), bytes );
	file.close();

	return buffer;
}

// ==============================================================================
// Other Functions


// very cool
// https://stackoverflow.com/questions/55424746/is-there-an-analogous-function-to-vsnprintf-that-works-with-stdstring
void vstring( std::string& result, const char* format, ... )
{
	va_list args, args_copy;

	va_start( args, format );
	va_copy( args_copy, args );

	int len = vsnprintf( nullptr, 0, format, args );
	if ( len < 0 )
	{
		va_end( args_copy );
		va_end( args );
		printf( "vstring va_args: vsnprintf failed\n" );
		return;
	}

	if ( len > 0 )
	{
		result.resize( len );
		vsnprintf( result.data(), len + 1, format, args_copy );
	}

	va_end( args_copy );
	va_end( args );
}

void vstring( std::string& s, const char* format, va_list args )
{
	va_list copy;
	va_copy( copy, args );
	int len = std::vsnprintf( nullptr, 0, format, copy );
	va_end( copy );

	if ( len >= 0 )
	{
		//std::string s( std::size_t(len) + 1, '\0' );
		s.resize( std::size_t( len ) + 1, '\0' );
		std::vsnprintf( s.data(), s.size(), format, args );
		s.resize( len );
		return;
	}

	printf( "vstring va_list: vsnprintf failed\n" );
}

std::string vstring( const char* format, ... )
{
	std::string result;
	va_list     args, args_copy;

	va_start( args, format );
	va_copy( args_copy, args );

	int len = vsnprintf( nullptr, 0, format, args );
	if ( len < 0 )
	{
		va_end( args_copy );
		va_end( args );
		throw std::runtime_error( "vsnprintf error" );
	}

	if ( len > 0 )
	{
		result.resize( len );
		vsnprintf( result.data(), len + 1, format, args_copy );
	}

	va_end( args_copy );
	va_end( args );

	return result;
}

std::string vstring( const char* format, va_list args )
{
	va_list copy;
	va_copy( copy, args );
	int len = std::vsnprintf( nullptr, 0, format, copy );
	va_end( copy );

	if ( len >= 0 )
	{
		std::string s( std::size_t( len ) + 1, '\0' );
		std::vsnprintf( s.data(), s.size(), format, args );
		s.resize( len );
		return s;
	}

	throw std::runtime_error( "vsnprintf error" );
}

