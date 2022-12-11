#include "imageloader.h"
#include "util.h"

#include "jxl/decode.h"
#include "jxl/decode_cxx.h"
#include "jxl/resizable_parallel_runner.h"
#include "jxl/resizable_parallel_runner_cxx.h"


constexpr int gPixelSize = sizeof( char ) * 4;  // 4 channels


class FormatJpegXL: public IImageFormat
{
public:
	FormatJpegXL()
	{
		ImageLoader_RegisterFormat( this );
	}

	~FormatJpegXL()
	{
	}

    bool CheckExt( std::wstring_view ext ) override
    {
		return ext == L"jxl";
    }

    bool CheckHeader( const fs::path& path ) override
    {
		// QByteArray header = device->peek(32);
		// if (header.size() < 12) {
		// 	return false;
		// }
		// 
		// JxlSignature signature = JxlSignatureCheck((const uint8_t *)header.constData(), header.size());
		// if (signature == JXL_SIG_CODESTREAM || signature == JXL_SIG_CONTAINER) {
		// 	return true;
		// }
		// return false;

		std::vector< char > header = fs_read_bytes( path, 32 );
		if (header.size() < 12) {
			return false;
		}

		JxlSignature signature = JxlSignatureCheck( (const uint8_t *)header.data(), header.size() );
		if (signature == JXL_SIG_CODESTREAM || signature == JXL_SIG_CONTAINER) {
			return true;
		}

		return false;
    }

	ImageInfo* LoadImage( const fs::path& path, std::vector< char >& srData ) override
	{
		if ( !fs_is_file( path.c_str() ) )
		{
			fwprintf( stderr, L"[FormatJpegXL] File does not exist: %s\n", path.c_str() );
			return nullptr;
		}

		// Multi-threaded parallel runner.
		auto runner = JxlResizableParallelRunnerCreate( nullptr );

		auto dec    = JxlDecoderCreate( nullptr );

		if ( JXL_DEC_SUCCESS != JxlDecoderSubscribeEvents( dec, JXL_DEC_BASIC_INFO | JXL_DEC_COLOR_ENCODING | JXL_DEC_FULL_IMAGE ) )
		{
			fprintf( stderr, "JxlDecoderSubscribeEvents failed\n" );
			return nullptr;
		}

		if ( JXL_DEC_SUCCESS != JxlDecoderSetParallelRunner( dec, JxlResizableParallelRunner, runner ) )
		{
			fprintf( stderr, "JxlDecoderSetParallelRunner failed\n" );
			return nullptr;
		}

		JxlBasicInfo   info;
		JxlPixelFormat format = { 4, JXL_TYPE_UINT8, JXL_NATIVE_ENDIAN, 0 };

		std::vector< char > fileData = fs_read_file( path );

		JxlDecoderStatus ret = JxlDecoderSetInput( dec, (const u8*)fileData.data(), fileData.size() );

		JxlDecoderCloseInput( dec );

		ImageInfo* imageInfo = new ImageInfo;

		// JxlDecoderSetInput
		// JxlDecoderProcessInput
		// JxlDecoderReleaseInput
		// JxlDecoderCloseInput

		std::vector< uint8_t > icc_profile;
		std::vector< float >   pixels;
		bool                   running = true;

		while ( running )
		{
			JxlDecoderStatus status = JxlDecoderProcessInput( dec );

			switch ( status )
			{
				case JXL_DEC_ERROR:
				{
					fprintf( stderr, "Decoder error\n" );
					delete imageInfo;
					return nullptr;
				}

				case JXL_DEC_NEED_MORE_INPUT:
				{
					fprintf( stderr, "Error, already provided all input\n" );
					return nullptr;
				}
				case JXL_DEC_BASIC_INFO:
				{
					if ( JXL_DEC_SUCCESS != JxlDecoderGetBasicInfo( dec, &info ) )
					{
						fprintf( stderr, "JxlDecoderGetBasicInfo failed\n" );
						return nullptr;
					}

					imageInfo->aWidth  = info.xsize;
					imageInfo->aHeight = info.ysize;

					JxlResizableParallelRunnerSetThreads(
					  runner,
					  JxlResizableParallelRunnerSuggestThreads( info.xsize, info.ysize ) );
					break;
				}
				case JXL_DEC_COLOR_ENCODING:
				{
					// Get the ICC color profile of the pixel data
					size_t icc_size;
					if ( JXL_DEC_SUCCESS !=
						 JxlDecoderGetICCProfileSize(
						   dec, &format, JXL_COLOR_PROFILE_TARGET_DATA, &icc_size ) )
					{
						fprintf( stderr, "JxlDecoderGetICCProfileSize failed\n" );
						return nullptr;
					}
					icc_profile.resize( icc_size );
					if ( JXL_DEC_SUCCESS != JxlDecoderGetColorAsICCProfile(
											  dec, &format,
											  JXL_COLOR_PROFILE_TARGET_DATA,
											  icc_profile.data(), icc_profile.size() ) )
					{
						fprintf( stderr, "JxlDecoderGetColorAsICCProfile failed\n" );
						return nullptr;
					}
					break;
				}
				case JXL_DEC_NEED_IMAGE_OUT_BUFFER:
				{
					size_t buffer_size;
					if ( JXL_DEC_SUCCESS !=
						 JxlDecoderImageOutBufferSize( dec, &format, &buffer_size ) )
					{
						fprintf( stderr, "JxlDecoderImageOutBufferSize failed\n" );
						return nullptr;
					}

					if ( buffer_size != imageInfo->aWidth * imageInfo->aHeight * gPixelSize )
					{
						fprintf( stderr, "Invalid out buffer size %llu %llu\n",
								 static_cast< uint64_t >( buffer_size ),
						         static_cast< uint64_t >( imageInfo->aWidth * imageInfo->aHeight * gPixelSize ) );
						return nullptr;
					}

					srData.resize( imageInfo->aWidth * imageInfo->aHeight * 4 );
					void*  pixels_buffer      = (void*)srData.data();
					// size_t pixels_buffer_size = srData.size() * sizeof( float );
					size_t pixels_buffer_size = srData.size() * sizeof( char );

					JxlDecoderStatus ret = JxlDecoderSetImageOutBuffer( dec, &format, pixels_buffer, pixels_buffer_size );
					if ( JXL_DEC_SUCCESS != ret )
					{
						fprintf( stderr, "JxlDecoderSetImageOutBuffer failed\n" );
						return nullptr;
					}
					break;
				}
				case JXL_DEC_FULL_IMAGE:
				{
					// Nothing to do. Do not yet return. If the image is an animation, more
					// full frames may be decoded. This example only keeps the last one.
					break;
				}
				case JXL_DEC_SUCCESS:
				{
					// All decoding successfully finished.
					// It's not required to call JxlDecoderReleaseInput(dec.get()) here since
					// the decoder will be destroyed.
					running = false;
					break;
				}
				default:
				{
					fprintf( stderr, "Unknown decoder status\n" );
					delete imageInfo;
					return nullptr;
				}
			}
		}

		JxlDecoderDestroy( dec );
		JxlResizableParallelRunnerDestroy( runner );

		imageInfo->aFormat = FMT_RGBA8;
		// imageInfo->aFormat = FMT_RGBA8;
		imageInfo->aBitDepth = 4;  // uhhhh

		return imageInfo;
	}
};


static FormatJpegXL* gpFmtJpegXL = new FormatJpegXL;

