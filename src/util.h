#pragma once

#include <string>
#include <vector>
#include <filesystem>

// trollface
namespace fs = std::filesystem;

#define ARR_SIZE( arr ) ( sizeof( arr ) / sizeof( arr[ 0 ] ) )

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

void        vstring( std::string& output, const char* format, ... );
void        vstring( std::string& output, const char* format, va_list args );

std::string vstring( const char* format, ... );
std::string vstring( const char* format, va_list args );


// ==============================================================================
// Vector Classes

template< typename SELF, typename T >
class vec2_base
{
  public:
	vec2_base( T x = 0.0, T y = 0.0 ) :
		x( x ), y( y )
	{
	}

	T           x{}, y{};

	constexpr T operator[]( int i )
	{
		// index into memory where vars are stored, and clamp to not read garbage
		return *( &x + std::clamp( i, 0, 1 ) );
	}

	constexpr void operator=( const SELF& other )
	{
		// Guard self assignment
		if ( this == &other )
			return;

		std::memcpy( &x, &other.x, sizeof( SELF ) );
	}

	constexpr bool operator==( const SELF& other )
	{
		// Guard self assignment
		if ( this == &other )
			return true;

		return !( std::memcmp( &x, &other.x, sizeof( SELF ) ) );
	}
};

class vec2
{
  public:
	vec2( float x = 0.f, float y = 0.f ) :
		x( x ), y( y )
	{
	}

	float           x{}, y{};

	constexpr float operator[]( int i )
	{
		// index into memory where vars are stored, and clamp to not read garbage
		return *( &x + std::clamp( i, 0, 1 ) );
	}

	constexpr void operator=( const vec2& other )
	{
		// Guard self assignment
		if ( this == &other )
			return;

		std::memcpy( &x, &other.x, sizeof( vec2 ) );
	}

	constexpr bool operator==( const vec2& other )
	{
		// Guard self assignment
		if ( this == &other )
			return true;

		return !( std::memcmp( &x, &other.x, sizeof( vec2 ) ) );
	}
};

class ivec2 : public vec2_base< ivec2, int >
{
  public:
	ivec2( int x = 0.f, int y = 0.f ) :
		vec2_base( x, y )
	{
	}
};

class vec3
{
  public:
	vec3( float x = 0.f, float y = 0.f, float z = 0.f ) :
		x( x ), y( y ), z( z )
	{
	}

	float           x{}, y{}, z{};

	constexpr float operator[]( int i )
	{
		// index into memory where vars are stored, and clamp to not read garbage
		return *( &x + std::clamp( i, 0, 2 ) );
	}

	constexpr void operator=( const vec3& other )
	{
		// Guard self assignment
		if ( this == &other )
			return;

		std::memcpy( &x, &other.x, sizeof( vec3 ) );
	}

	constexpr bool operator==( const vec3& other )
	{
		// Guard self assignment
		if ( this == &other )
			return true;

		return !( std::memcmp( &x, &other.x, sizeof( vec3 ) ) );
	}
};

