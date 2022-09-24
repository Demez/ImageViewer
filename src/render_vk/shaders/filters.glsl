
float Triangular( float f )
{
	f = f / 2.0;
	if( f < 0.0 )
	{
		return ( f + 1.0 );
	}
	else
	{
		return ( 1.0 - f );
	}
	return 0.0;
}


vec4 BiCubic( sampler2D tex, vec2 TexCoord, vec2 sourceSize, vec2 destSize )
{
	float texelSizeX = 1.0 / sourceSize.x; //size of one texel
	float texelSizeY = 1.0 / sourceSize.y; //size of one texel

    vec4 nSum = vec4( 0.0, 0.0, 0.0, 0.0 );
    vec4 nDenom = vec4( 0.0, 0.0, 0.0, 0.0 );

    float a = fract( TexCoord.x * sourceSize.x ); // get the decimal part
    float b = fract( TexCoord.y * sourceSize.y ); // get the decimal part

	int nX = int( TexCoord.x * sourceSize.x );
	int nY = int( TexCoord.y * sourceSize.y );

	vec2 TexCoord1 = vec2( float(nX) / sourceSize.x + 0.5 / sourceSize.x,
					       float(nY) / sourceSize.y + 0.5 / sourceSize.y );

    for( int m = -1; m <=2; m++ )
    {
        for( int n =-1; n<= 2; n++)
        {
			vec4 vecData = texture( tex, TexCoord1 + vec2(texelSizeX * float( m ), texelSizeY * float( n )) );

			float f  = Triangular( float( m ) - a );
			vec4 vecCooef1 = vec4( f,f,f,f );

			float f1 = Triangular( -( float( n ) - b ) );
			vec4 vecCoeef2 = vec4( f1, f1, f1, f1 );

            nSum = nSum + ( vecData * vecCoeef2 * vecCooef1  );
            nDenom = nDenom + (( vecCoeef2 * vecCooef1 ));
        }
    }

    return nSum / nDenom;
}


// NOTE: REALLY REALLY EXPENSIVE DUE TO EXCESSIVE TEXTURE LOOKUPS !!!!!!
vec4 DoPixelBinning( sampler2D tex, vec2 texCoord, vec2 sourceSize, vec2 destSize )
{
	// no reason to run
	if ( destSize.x >= sourceSize.x )
		return texture( tex, texCoord );

	vec2 blockSize = (sourceSize / destSize) * 0.5;
	vec2 texelSize = vec2(1.0, 1.0) / sourceSize;

	vec4 colorTest = vec4(0, 0, 0, 0);
	int sampleCount = 0;

	for( float x = 0; x <= blockSize.x; x++ )
    {
        for( float y = 0; y <= blockSize.y; y++ )
        {
			colorTest += texture( tex, texCoord + vec2(texelSize.x * x, texelSize.y * y) );
			sampleCount++;
		}
	}

	return colorTest / sampleCount;
}





float sinc( float x )
{
    const float PI = 3.1415926;
    return sin(PI * x) / (PI * x);
}

float LWeight( float x )
{
    if (abs(x) < 1e-8) {
        return 1.0f;
    }
    else {
        return sinc(x) * sinc(x / 3);
    }
}

/*
vec4 Lanczos( sampler2D textureSampler, vec2 texCoord )
{
    ivec2 dim = textureSize(textureSampler, 0);

    vec2 imgCoord = vec2(dim) * texCoord;
    vec2 samplePos = floor(imgCoord - 0.5) + 0.5;
    vec2 interpFactor = imgCoord - samplePos;
    samplePos /= vec2(dim);

    vec4 nSum = vec4( 0.0, 0.0, 0.0, 0.0 );
    float fX,fY;
        
    for( int m = -2; m <=3; m++ )
    {
        fX  = LWeight(float(m)-interpFactor.x);
        vec4 vecCooef1 = vec4( fX,fX,fX,fX );

        for( int n = -2; n<= 3; n++)
        {
			vec4 vecData = textureOffset( textureSampler, samplePos, ivec2(m,n) );

			fY = LWeight(float(n)-interpFactor.y);
			vec4 vecCoeef2 = vec4( fY, fY, fY, fY );
            nSum += vecData * vecCoeef2 * vecCooef1 ;
        }
    }

    return nSum;
}
*/
