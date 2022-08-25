#pragma once

#include <string>

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
// Qt Functions

// ==============================================================================
// Filesystem/Path Functions

std::string fs_get_file_ext( const std::string& path );
bool        fs_file_exists( const std::string& path );

// ==============================================================================
// Other Functions

void        sys_sleep( float ms );
