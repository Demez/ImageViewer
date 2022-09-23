
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

