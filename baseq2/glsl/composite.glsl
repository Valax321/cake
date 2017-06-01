// common
INOUTTYPE vec4 texcoords;

#ifdef VERTEXSHADER
uniform mat4 orthomatrix;

layout(location = 0) in vec4 position;
layout(location = 2) in vec4 texcoord;


void CompositeVS ()
{
	gl_Position = orthomatrix * position;
	texcoords = texcoord;
}
#endif


#ifdef FRAGMENTSHADER
uniform sampler2D diffuse;
uniform vec2 rescale;

uniform int compositeMode;
uniform vec2 texScale;

out vec4 fragColor;

vec3 saturate( vec3 v ) { return clamp( v, 0.0, 1.0 ); }
// https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
vec3 ACESFilm( vec3 x )
{
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return saturate( ( x * ( a * x + b ) ) / ( x * ( c * x + d ) + e ) );
}

void CompositeFS ()
{
	vec4 color = vec4(0.0);
	vec2 st = gl_FragCoord.st * texScale;
		
	// brightpass + tonemap
	if(compositeMode == 1)
	{
		// multiply with 4 because the FBO is only 1/4th of the screen resolution
		st *= vec2(4.0, 4.0);
		
		color = texture (diffuse, st);

		// get the luminance of the current pixel
		float Y = dot( vec4(0.2125, 0.7154, 0.0721, 0.0), color );
		if(Y < 0.1)
		{
			fragColor = vec4( 0.0, 0.0, 0.0, 1.0 );
			return;
		}
		
		float hdrKey = 0.015;
		float hdrAverageLuminance = 0.005;
		float hdrMaxLuminance = 1;
		
		// calculate the relative luminance
		float Yr = ( hdrKey * Y ) / hdrAverageLuminance;

		float Ymax = hdrMaxLuminance;
		
		float avgLuminance = max( hdrAverageLuminance, 0.001 );
		float linearExposure = ( hdrKey / avgLuminance );
		float exposure = log2( max( linearExposure, 0.0001 ) );
		
		//exposure = -2.0;
		vec3 exposedColor = exp2( exposure ) * color.rgb;
		
		color.rgb = ACESFilm( exposedColor );
		
		// adjust contrast
		const float hdrContrastThreshold = 2;
		const float hdrContrastOffset = 3;
		
		float T = max( Yr - hdrContrastThreshold, 0.0 );		
		float B = T > 0.0 ? T / ( hdrContrastOffset + T ) : T;
		
		// clamp to 0, 1
		//color.rgb *= clamp( B, 0.0, 1.0 );
	}
	// hdr chromatic glare
	else if(compositeMode == 2)
	{
		const float gaussFact[9] = float[9](0.13298076, 0.12579441, 0.10648267, 0.08065691, 0.05467002, 0.03315905, 0.01799699, 0.00874063, 0.00379866);
		
		const vec3 chromaticOffsets[9] = vec3[](
			vec3(0.5, 0.5, 0.5), // w
			vec3(0.8, 0.3, 0.3),
		//	vec3(1.0, 0.2, 0.2), // r
			vec3(0.5, 0.2, 0.8),
			vec3(0.2, 0.2, 1.0), // b
			vec3(0.2, 0.3, 0.9),
			vec3(0.2, 0.9, 0.2), // g
			vec3(0.3, 0.5, 0.3),
			vec3(0.3, 0.5, 0.3),
			vec3(0.3, 0.5, 0.3)
			//vec3(0.3, 0.5, 0.3)
		);
	
		vec3 sumColor = vec3( 0.0 );
		vec3 sumSpectrum = vec3( 0.0 );

		const int tap = 4;
		const int samples = 9;
		
		float scale = 13.0; // bloom width
		const float weightScale = 2.3; // bloom strength
	
		for( int i = 0; i < samples; i++ )
		{
			vec3 so = chromaticOffsets[ i ];
			vec4 color = texture( diffuse, st + vec2( float( i ), 0 ) * texScale );
				
			float weight = gaussFact[ i ];
			sumColor += color.rgb * ( so.rgb * weight * weightScale );
		}	

		for( int i = 1; i < samples; i++ )
		{
			vec3 so = chromaticOffsets[ i ];
			vec4 color = texture( diffuse, st + vec2( float( -i ), 0 ) * texScale );
				
			float weight = gaussFact[ i ];
			sumColor += color.rgb * ( so.rgb * weight * weightScale );
		}
	
		fragColor = vec4( sumColor, 1.0 );
		return;
	}	
	
	// convert from linear RGB to sRGB
	float gamma = 1.0 / 2.2;
	color.r = pow( color.r, gamma );
	color.g = pow( color.g, gamma );
	color.b = pow( color.b, gamma );
	
	fragColor = color;
}
#endif

