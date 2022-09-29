#include "args.h"

#include <vector>
#include <string>


static std::vector< std::USTRING_VIEW > gArgs;


DLL_EXPORT void Args_Init( int argc, uchar* argv[] )
{
	gArgs.resize( argc );
	for ( int i = 0; i < argc; i++ )
	{
#if 0 // _WIN32
		wchar_t nameW[ 2048 ] = { 0 };
		int     len           = Plat_ToUnicode( argv[ i ], nameW, 2048 );
		gArgs[ i ]            = nameW;
#else
		gArgs[ i ] = argv[ i ];
#endif
	}
}


DLL_EXPORT bool Args_Has( std::USTRING_VIEW search )
{
	for ( auto& arg : gArgs )
		if ( arg == search )
			return true;

	return false;
}


DLL_EXPORT int Args_Index( std::USTRING_VIEW search )
{
	for ( int i = 0; i < gArgs.size(); i++ )
		if ( gArgs[ i ] == search )
			return i;

	return -1;
}


DLL_EXPORT int Args_Count()
{
	return gArgs.size();
}


// TODO: get rid of this returning temporary fallback thing
DLL_EXPORT const std::USTRING_VIEW& Args_Get( int index, const std::USTRING& fallback )
{
	if ( index == -1 || index > gArgs.size() )
		return fallback;

	return gArgs[ index ];
}


DLL_EXPORT const std::USTRING_VIEW& Args_GetValue( std::USTRING_VIEW search, const std::USTRING& fallback )
{
	int i = Args_Index( search );

	if ( i == -1 || i + 1 > gArgs.size() )
		return fallback;

	return gArgs[ i + 1 ];
}


// function to be able to find multiple values
// returns true if it finds a value, false if it fails to
DLL_EXPORT bool Args_GetNext( int& i, std::USTRING_VIEW search, std::USTRING& ret )
{
	for ( ; i < gArgs.size(); i++ )
	{
		if ( gArgs[ i ] == search )
		{
			if ( i == -1 || i + 1 > gArgs.size() )
				return false;

			ret = gArgs[ i + 1 ];
			return true;
		}
	}

	return false;
}

