#pragma once

#include "platform.h"

#include <string>

void                Args_Init( int argc, uchar* argv[] );

bool                Args_Has( std::USTRING_VIEW search );
int                 Args_Index( std::USTRING_VIEW search );
int                 Args_Count();

const std::USTRING& Args_Get( int index, const std::USTRING& fallback = _T("") );
const std::USTRING& Args_GetValue( std::USTRING_VIEW search, const std::USTRING& fallback = _T("") );

// function to be able to find multiple values
// returns true if it finds a value, false if it fails to
bool                Args_GetNext( int& i, std::USTRING_VIEW search, std::USTRING& ret );

