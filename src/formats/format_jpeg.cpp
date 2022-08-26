#include "imageloader.h"
#include "util.h"

#include <turbojpeg.h>


class FormatJpeg: public IImageFormat
{
public:
	FormatJpeg()
	{
		ImageLoader_RegisterFormat( this );
	}

	~FormatJpeg()
	{
	}

    bool CheckExt( std::wstring_view ext ) override
    {
		return ext == L"jpg" || ext == L"jpeg";
    }

	ImageInfo* LoadImage( const fs::path& path, std::vector< char >& srData ) override
	{
		if ( !fs_is_file( path.c_str() ) )
		{
			fwprintf( stderr, L"[FormatJpeg] File does not exist: %s\n", path.c_str() );
			return nullptr;
		}

		tjhandle tjpg = tjInitDecompress();

        if ( tjpg == nullptr )
		{
			fprintf( stderr, "[FormatJpeg] Failed to allocate memory for jpeg decompressor.\n" );
			return nullptr;
		}

		// FILE* pFile = fopen( path.c_str(), "rb" );
		// if ( !pFile )
		// 	return nullptr;

		std::vector< char > fileData = fs_read_file( path );

		ImageInfo* imageInfo = new ImageInfo;

		int subSamp, colorSpace;

		int ret = tjDecompressHeader3(
			tjpg,
			(const unsigned char*)fileData.data(),
			fileData.size(),
			&imageInfo->aWidth,
			&imageInfo->aHeight,
			&subSamp,
			&colorSpace
		);

        if ( ret != 0 )
		{
			fwprintf( stderr, L"[FormatJpeg] Failed to decompress header on image: %s\n", path.c_str() );
			return nullptr;
		}

		int pixelFmt = TJPF_RGB;
		// int pixelFmt = TJPF_BGRA;

		srData.resize( imageInfo->aWidth * imageInfo->aHeight * tjPixelSize[ pixelFmt ] );

		ret = tjDecompress2(
		    tjpg,
		    (const unsigned char*)fileData.data(),
			fileData.size(),
		    (unsigned char*)srData.data(),
			0,
			imageInfo->aWidth * 3,
			0,
			pixelFmt,
			TJFLAG_ACCURATEDCT
		);

        if ( ret != 0 )
		{
			// fprintf( stderr, "[FormatJpeg] Failed to decompress image: %ws\n%s\n", path.c_str(), tjGetErrorStr2( tjpg ) );
			fprintf( stderr, "[FormatJpeg] Failed to decompress image: %s\n%ws\n", tjGetErrorStr2( tjpg ), path.c_str() );
			return nullptr;
		}

		imageInfo->aFormat = FMT_RGB8;
		// imageInfo->aFormat = FMT_RGBA8;
		imageInfo->aBitDepth = 4;  // uhhhh

		return imageInfo;
	}
};


static FormatJpeg* gpFmtJpeg = new FormatJpeg;

