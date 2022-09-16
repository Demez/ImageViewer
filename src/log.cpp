#include "platform.h"
#include "util.h"
#include "log.h"


void LogFatal( const char* spStr )
{
	// std::string messageBoxTitle;
	// vstring( messageBoxTitle, "Fatal Error: %s", channel->aName.c_str() );

	printf( spStr );
	throw std::runtime_error( spStr );
}

