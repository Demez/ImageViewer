#include "imageloader.h"

#include <spng.h>


class FormatPNG: public IImageFormat
{
public:
    std::wstring aExt = L"png";

	FormatPNG()
	{
		ImageLoader_RegisterFormat( this );
	}

	~FormatPNG()
	{
	}

    bool CheckExt( std::wstring_view ext ) override
    {
        return ext == aExt;
    }

	ImageInfo* LoadImage( const fs::path& path, std::vector< char >& srData ) override
	{
        spng_ctx *ctx = spng_ctx_new( 0 );
        if( !ctx )
        {
            fprintf( stderr, "[FormatPNG] Failed to allocate memory for png context.\n" );
            return nullptr;
        }

        FILE* pFile = _wfopen( path.c_str(), L"rb" );
        if( !pFile )
            return nullptr;

        struct spng_ihdr ihdr;
		int              err = 0;

        err = spng_set_png_file( ctx, pFile );
		if ( err != 0 )
		{
			printf( "[FormatPNG] Failed to set image: %s\n", spng_strerror( err ) );
			spng_ctx_free( ctx );
			fclose( pFile );
			return nullptr;
		}

        err = spng_get_ihdr( ctx, &ihdr );
		if ( err != 0 )
		{
			printf( "[FormatPNG] Failed to get ihdr: %s\n", spng_strerror( err ) );
			spng_ctx_free( ctx );
			fclose( pFile );
			return nullptr;
		}

		// look into RGBA16?
        spng_format pngFmt = SPNG_FMT_RGBA8;

        // if ( ihdr.color_type == SPNG_COLOR_TYPE_TRUECOLOR )
		// {
		// 	pngFmt = SPNG_FMT_RGB8;
		// }

        size_t size;
		err = spng_decoded_image_size( ctx, pngFmt, (size_t*)&size );
		if ( err != 0 )
		{
			printf( "[FormatPNG] Failed to decode image size: %s\n", spng_strerror( err ) );
			spng_ctx_free( ctx );
			fclose( pFile );
			return nullptr;
		}

        srData.resize( size );
		err = spng_decode_image( ctx, srData.data(), size, pngFmt, 0 );
		if ( err != 0 )
		{
			printf( "[FormatPNG] Failed to decode image: %s\n", spng_strerror( err ) );
			spng_ctx_free( ctx );
			fclose( pFile );
			return nullptr;
		}

		ImageInfo* imageData = new ImageInfo;
		imageData->aWidth    = ihdr.width;
		imageData->aHeight   = ihdr.height;
		imageData->aBitDepth = ihdr.bit_depth;
		imageData->aFormat   = pngFmt == SPNG_FMT_RGBA8 ? FMT_RGBA8 : FMT_RGB8;

        spng_ctx_free( ctx );

        fclose( pFile );

		return imageData;
	}
};


FormatPNG* gpFmtPng = new FormatPNG;

