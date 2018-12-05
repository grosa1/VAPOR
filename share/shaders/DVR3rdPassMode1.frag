#version 410 core

in vec4 gl_FragCoord;
layout(location = 0) out vec4 color;

uniform sampler2D  backFaceTexture;
uniform sampler2D  frontFaceTexture;
uniform sampler3D  volumeTexture;
uniform usampler3D missingValueMaskTexture; // !!unsigned integer!!
uniform sampler1D  colorMapTexture;

uniform vec2  dataRanges[2];
uniform vec3  boxExtents[2];
uniform ivec3 volumeDims;        // number of vertices of this volumeTexture
uniform ivec2 viewportDims;      // width and height of this viewport
uniform vec4  clipPlanes[6];     // clipping planes in **un-normalized** model coordinates

uniform float stepSize1D;
uniform bool  flags[3];
uniform float lightingCoeffs[4];

uniform mat4 MV;
uniform mat4 Projection;
uniform mat4 inversedMV;

// 
// Derive helper variables
//
const float ULP        = 1.2e-7f;
const float ULP10      = 1.2e-6f;
bool  fast             = flags[0];          // fast rendering mode
bool  lighting         = fast ? false : flags[1];   // no lighting in fast mode
bool  hasMissingValue  = flags[2];          // has missing values or not
float ambientCoeff     = lightingCoeffs[0];
float diffuseCoeff     = lightingCoeffs[1];
float specularCoeff    = lightingCoeffs[2];
float specularExp      = lightingCoeffs[3];
// min, max, and range values of this variable. 
vec3  valueRange       = vec3(dataRanges[0], dataRanges[0].y - dataRanges[0].x); 
// min, max, and range values on this color map
vec3  colorMapRange    = vec3(dataRanges[1], dataRanges[1].y - dataRanges[1].x); 
vec3  boxMin           = boxExtents[0];     // min coordinates of the bounding box of this volume
vec3  boxMax           = boxExtents[1];     // max coordinates of the bounding box of this volume
vec3  volumeDimsf      = vec3( volumeDims );
vec3  boxSpan          = boxMax - boxMin;
mat4  transposedInverseMV = transpose(inversedMV);

//
// Input:  normalized value w.r.t. valueRange.
// Output: normalized value w.r.t. colorMapRange.
//
float TranslateValue( const in float value )
{
    if( colorMapRange.x != colorMapRange.y )
    {
        float orig = value * valueRange.z + valueRange.x;
        return (orig - colorMapRange.x) / colorMapRange.z;
    }
    else
        return value;
}

//
// Input:  Location to be evaluated in texture coordinates and model coordinates.
// Output: If this location should be skipped.
// Note:   It is skipped in two cases: 1) it represents a missing value
//                                     2) it is outside of clipping planes
//
bool ShouldSkip( const in vec3 tc, const in vec3 mc )
{
    if( hasMissingValue && (texture(missingValueMaskTexture, tc).r != 0u) )
        return true;

    vec4 positionModel = vec4( mc, 1.0 );
    for( int i = 0; i < 6; i++ )
    {
        if( dot(positionModel, clipPlanes[i]) < 0.0 )
            return true;
    }

    return false;
}

//
// Input:  Location to be evaluated in texture coordinates
// Output: Gradient at that location
//
vec3 CalculateGradient( const in vec3 tc )
{
    vec3 h0 = vec3(-0.5 ) / volumeDimsf;
    vec3 h1 = vec3( 0.5 ) / volumeDimsf;
    vec3 h  = vec3( 1.0 );

    if ((tc.x + h0.x) < 0.0) {
        h0.x = 0.0;
        h.x  = 0.5;
    }
    if ((tc.x + h1.x) > 1.0) {
        h1.x = 0.0;
        h.x  = 0.5;
    }
    if ((tc.y + h0.y) < 0.0) {
        h0.y = 0.0;
        h.y  = 0.5;
    }
    if ((tc.y + h1.y) > 1.0) {
        h1.y = 0.0;
        h.y  = 0.5;
    }
    if ((tc.z + h0.z) < 0.0) {
        h0.z = 0.0;
        h.z  = 0.5;
    }
    if ((tc.z + h1.z) > 1.0) {
        h1.z = 0.0;
        h.z  = 0.5;
    }

    vec3 a0, a1;
    a0.x = texture( volumeTexture, tc + vec3(h0.x, 0.0f, 0.0f) ).r;
    a1.x = texture( volumeTexture, tc + vec3(h1.x, 0.0f, 0.0f) ).r;
    a0.y = texture( volumeTexture, tc + vec3(0.0f, h0.y, 0.0f) ).r;
    a1.y = texture( volumeTexture, tc + vec3(0.0f, h1.y, 0.0f) ).r;
    a0.z = texture( volumeTexture, tc + vec3(0.0f, 0.0f, h0.z) ).r;
    a1.z = texture( volumeTexture, tc + vec3(0.0f, 0.0f, h1.z) ).r;

    return (a1 - a0 / h);
}


void main(void)
{
    gl_FragDepth        = 1.0;
    color               = vec4( 0.0 );
    vec3  lightDirEye   = vec3(0.0, 0.0, 1.0); 

    // Calculate texture coordinates of this fragment
    vec2 fragTexture    = gl_FragCoord.xy / vec2( viewportDims );

    vec3 stopEye        = texture( backFaceTexture,  fragTexture ).xyz;
    vec3 startEye       = texture( frontFaceTexture, fragTexture ).xyz;
    vec3 rayDirEye      = stopEye - startEye ;
    float rayDirLength  = length( rayDirEye );
    if( rayDirLength    < ULP10 )
        discard;

    float nStepsf       = rayDirLength / stepSize1D;
    vec3  stepSize3D    = rayDirEye    / nStepsf;

    vec3  startModel    = (inversedMV * vec4(startEye, 1.0)).xyz;
    vec3  startTexture  = (startModel - boxMin) / boxSpan;
    if( !ShouldSkip( startTexture, startModel ) )
    {
        float step1Value   = texture( volumeTexture, startTexture ).r;
              color        = texture( colorMapTexture, TranslateValue(step1Value) );
              color.rgb   *= color.a;
    }

    // let's do a ray casting! 
    vec3 step2Eye = startEye;
    int  nSteps   = int(nStepsf) + 2;
    int  stepi;
    for( stepi = 1; stepi < nSteps; stepi++ )
    {
        if( color.a > 0.999 )  // You can still see something with 0.99...
            break;

             step2Eye     = startEye + stepSize3D * float( stepi );
        vec3 step2Model   = (inversedMV * vec4(step2Eye, 1.0)).xyz;
        vec3 step2Texture = (step2Model - boxMin) / boxSpan;
        if( ShouldSkip( step2Texture, step2Model ) )
            continue;

        float step2Value   = texture( volumeTexture, step2Texture ).r;
        vec4  backColor    = texture( colorMapTexture, TranslateValue(step2Value) );
        
        // Apply lighting
        if( lighting && backColor.a > 0.001 )
        {
            vec3 gradientModel = CalculateGradient( step2Texture );
            if( length( gradientModel ) > ULP10 )
            {
                vec3 gradientEye = (transposedInverseMV * vec4( gradientModel, 0.0 )).xyz;
                     gradientEye = normalize( gradientEye );
                float diffuse    = abs( dot(lightDirEye, gradientEye) );
                vec3 viewDirEye  = normalize( -step2Eye );
                vec3 reflectionEye = reflect( -lightDirEye, gradientEye );
                float specular   = pow( max(0.0, dot( reflectionEye, viewDirEye )), specularExp ); 
                backColor.rgb    = backColor.rgb * (ambientCoeff + diffuse * diffuseCoeff) + 
                                   specular * specularCoeff;
            }
        }

        // Color compositing
        color.rgb += (1.0 - color.a) * backColor.a * backColor.rgb;
        color.a   += (1.0 - color.a) * backColor.a;
    }

    // Apply depth if sufficient opaqueness
    if( color.a > 0.7 )
    {
        vec4 step2Clip =  Projection    * vec4( step2Eye, 1.0 );
        vec3 step2Ndc  =  step2Clip.xyz / step2Clip.w;
        gl_FragDepth   =  gl_DepthRange.diff * 0.5 * step2Ndc.z +
                         (gl_DepthRange.near + gl_DepthRange.far) * 0.5;
    }

}       // End main()

