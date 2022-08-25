#include "imageloader.h"


class FormatWin32WIC: public IImageFormat
{
public:
	FormatWin32WIC()
	{
		GetImageLoader().RegisterFormat( this );
	}

	~FormatWin32WIC()
	{
	}

	ImageData* LoadImage( const std::string& path ) override
	{
		return nullptr;
	}

	bool CheckExt( const std::string& ext ) override
	{
		return false;
	}
};


FormatWin32WIC* gpFmtWin32WIC = new FormatWin32WIC;

