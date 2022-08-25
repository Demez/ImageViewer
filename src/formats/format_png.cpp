#include "imageloader.h"

#include <spng.h>


class FormatPNG: public IImageFormat
{
public:
    std::string aExt = "png";

	FormatPNG()
	{
		GetImageLoader().RegisterFormat( this );
	}

	~FormatPNG()
	{
	}

    bool CheckExt( const std::string& ext ) override
    {
        return ext == aExt;
    }

	ImageData* LoadImage( const std::string& path ) override
	{
        spng_ctx *ctx = spng_ctx_new( 0 );
        if( !ctx )
        {
            fprintf( stderr, "[FormatPNG] Failed to allocate memory for png context.\n" );
            return nullptr;
        }

        FILE *pFile = fopen( path.c_str(), "rb" );
        if( !pFile )
            return nullptr;

        ImageData* imageData = new ImageData;

        struct spng_ihdr ihdr;

        spng_set_png_file( ctx, pFile );
        spng_get_ihdr( ctx, &ihdr );

        size_t size;
        spng_decoded_image_size( ctx, SPNG_FMT_RGBA8, ( size_t* )&size );

        imageData->aData.resize( size );

        spng_decode_image( ctx, imageData->aData.data(), size, SPNG_FMT_RGBA8, 0 );

		imageData->aWidth    = ihdr.width;
		imageData->aHeight   = ihdr.height;
		imageData->aBitDepth = ihdr.bit_depth;
		imageData->aFormat   = FMT_RGBA8;

        //imageData->aFormat = SPNG_FMT_RGBA8;

        // srData.push_back( pData ); 

        spng_ctx_free( ctx );

		return imageData;
	}
};


FormatPNG* gpFmtPng = new FormatPNG;

