#pragma once

#include "platform.h"

#include <string>


DLL_EXPORT void  Args_Init( int argc, uchar* argv[] );

DLL_EXPORT bool  Args_Has( std::USTRING_VIEW search );
DLL_EXPORT int   Args_Index( std::USTRING_VIEW search );
DLL_EXPORT int   Args_Count();

DLL_EXPORT const std::USTRING_VIEW& Args_Get( int index, const std::USTRING& fallback = _T("") );
DLL_EXPORT const std::USTRING_VIEW& Args_GetValue( std::USTRING_VIEW search, const std::USTRING& fallback = _T("") );

// function to be able to find multiple values
// returns true if it finds a value, false if it fails to
DLL_EXPORT bool                     Args_GetNext( int& i, std::USTRING_VIEW search, std::USTRING& ret );


#define ARGS_HAS( search ) Args_Has( _T( search ) )

