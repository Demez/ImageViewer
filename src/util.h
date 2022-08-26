#pragma once

#include <string>
#include <vector>
#include <filesystem>

// trollface
namespace fs = std::filesystem;

// ==============================================================================
// Short Types

using u8  = unsigned char;
using u16 = unsigned short;
using u32 = unsigned int;
using u64 = unsigned long long;

using s8  = char;
using s16 = short;
using s32 = int;
using s64 = long long;

using f32 = float;
using f64 = double;

// ==============================================================================
// Filesystem/Path Functions

using DirHandle_t = void*;

enum ReadDirFlags_: unsigned char
{
	ReadDir_None = 0,
	ReadDir_AbsPaths = 1 << 1,
	ReadDir_NoDirs = 1 << 2,
	ReadDir_NoFiles = 1 << 3,
	ReadDir_Recursive = 1 << 4,
};

using ReadDirFlags = unsigned char;

std::wstring               fs_get_file_ext( const fs::path& path );
std::string                fs_get_dir_name( const std::string& path );
bool                       fs_file_exists( const wchar_t* path );
std::wstring               fs_clean_path( const std::wstring& path );
// bool                       fs_is_absolute( const std::wstring& path );
// std::vector< std::string > fs_scan_dir( const std::string& path, ReadDirFlags flags );

bool                       fs_is_dir( const wchar_t* path );
bool                       fs_is_file( const wchar_t* path );

DirHandle_t                fs_read_first( const fs::path& path, fs::path& file, ReadDirFlags flags );
bool                       fs_read_next( DirHandle_t dirh, fs::path& file );
bool                       fs_read_close( DirHandle_t dirh );

std::vector< char >        fs_read_file( const fs::path& srFilePath );

// ==============================================================================
// std::vector Functions

template< class T >
constexpr size_t vec_index( std::vector< T >& vec, T item, size_t fallback = SIZE_MAX )
{
	auto it = std::find( vec.begin(), vec.end(), item );
	if ( it != vec.end() )
		return it - vec.begin();

	return fallback;
}

template< class T >
constexpr size_t vec_index( const std::vector< T >& vec, T item, size_t fallback = SIZE_MAX )
{
	auto it = std::find( vec.begin(), vec.end(), item );
	if ( it != vec.end() )
		return it - vec.begin();

	return fallback;
}

template< class T >
constexpr void vec_remove( std::vector< T >& vec, T item )
{
	vec.erase( vec.begin() + vec_index( vec, item ) );
}

// Remove item if it exists
template< class T >
constexpr void vec_remove_if( std::vector< T >& vec, T item )
{
	size_t index = vec_index( vec, item );
	if ( index != SIZE_MAX )
		vec.erase( vec.begin() + index );
}

template< class T >
constexpr void vec_remove_index( std::vector< T >& vec, size_t index )
{
	vec.erase( vec.begin() + index );
}

template< class T >
constexpr bool vec_contains( std::vector< T >& vec, T item )
{
	return ( std::find( vec.begin(), vec.end(), item ) != vec.end() );
}

template< class T >
constexpr bool vec_contains( const std::vector< T >& vec, T item )
{
	return ( std::find( vec.begin(), vec.end(), item ) != vec.end() );
}


// ==============================================================================
// Other Functions

void         sys_sleep( float ms );
void         sys_browse_to_file( const fs::path& file );
std::wstring sys_to_wstr( const char* spStr );

