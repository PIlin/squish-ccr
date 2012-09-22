/* -----------------------------------------------------------------------------

	Copyright (c) 2006 Simon Brown                          si@sjbrown.co.uk
	Copyright (c) 2012 Niels Fr�hling              niels@paradice-insight.us

	Permission is hereby granted, free of charge, to any person obtaining
	a copy of this software and associated documentation files (the
	"Software"), to	deal in the Software without restriction, including
	without limitation the rights to use, copy, modify, merge, publish,
	distribute, sublicense, and/or sell copies of the Software, and to
	permit persons to whom the Software is furnished to do so, subject to
	the following conditions:

	The above copyright notice and this permission notice shall be included
	in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
	OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
	MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
	IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
	CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
	TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
	SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

   -------------------------------------------------------------------------- */

/*! @file

	The symmetric eigensystem solver algorithm is from
	http://www.geometrictools.com/Documentation/EigenSymmetric3x3.pdf
*/

#include "maths.h"
#include "simd.h"

namespace squish {

/* *****************************************************************************
 */
#if	!defined(USE_PRE)
Sym3x3 ComputeWeightedCovariance(int n, Vec3 const* points, float const* weights)
{
  // compute the centroid
  float total = 0.0f;
  Vec3 centroid(0.0f);

  for (int i = 0; i < n; ++i) {
    total    += weights[i];
    centroid += weights[i] * points[i];
  }

  centroid /= total;

  // accumulate the covariance smatrix
  Sym3x3 covariance(0.0f);
  for (int i = 0; i < n; ++i) {
    Vec3 a = points[i] - centroid;
    Vec3 b = weights[i] * a;

    covariance[0] += a.X() * b.X();
    covariance[1] += a.X() * b.Y();
    covariance[2] += a.X() * b.Z();
    covariance[3] += a.Y() * b.Y();
    covariance[4] += a.Y() * b.Z();
    covariance[5] += a.Z() * b.Z();
  }

  // return it
  return covariance;
}

Sym3x3 ComputeWeightedCovariance(int n, Vec4 const* points, float const* weights)
{
  // compute the centroid
  float total = 0.0f;
  Vec4 centroid(0.0f);

  for (int i = 0; i < n; ++i) {
    total    += weights[i];
    centroid += weights[i] * points[i];
  }

  centroid /= total;

  // accumulate the covariance smatrix
  Sym3x3 covariance(0.0f);
  for( int i = 0; i < n; ++i )
  {
    Vec4 a = points[i] - centroid;
    Vec4 b = weights[i] * a;

    covariance[0] += a.X() * b.X();
    covariance[1] += a.X() * b.Y();
    covariance[2] += a.X() * b.Z();
    covariance[3] += a.Y() * b.Y();
    covariance[4] += a.Y() * b.Z();
    covariance[5] += a.Z() * b.Z();
  }

  // return it
  return covariance;
}

static Vec3 GetMultiplicity1Evector( Sym3x3 const& smatrix, float evalue )
{
  // compute M
  Sym3x3 m;
  m[0] = smatrix[0] - evalue;
  m[1] = smatrix[1];
  m[2] = smatrix[2];
  m[3] = smatrix[3] - evalue;
  m[4] = smatrix[4];
  m[5] = smatrix[5] - evalue;

  // compute U
  Sym3x3 u;
  u[0] = m[3]*m[5] - m[4]*m[4];
  u[1] = m[2]*m[4] - m[1]*m[5];
  u[2] = m[1]*m[4] - m[2]*m[3];
  u[3] = m[0]*m[5] - m[2]*m[2];
  u[4] = m[1]*m[2] - m[4]*m[0];
  u[5] = m[0]*m[3] - m[1]*m[1];

  // find the largest component
  float mc = std::fabs( u[0] );
  int mi = 0;
  for( int i = 1; i < 6; ++i )
  {
    float c = std::fabs( u[i] );
    if( c > mc )
    {
      mc = c;
      mi = i;
    }
  }

  // pick the column with this component
  switch( mi )
  {
    case 0:
      return Vec3( u[0], u[1], u[2] );

    case 1:
    case 3:
      return Vec3( u[1], u[3], u[4] );

    default:
      return Vec3( u[2], u[4], u[5] );
  }
}

static Vec3 GetMultiplicity2Evector( Sym3x3 const& smatrix, float evalue )
{
  // compute M
  Sym3x3 m;
  m[0] = smatrix[0] - evalue;
  m[1] = smatrix[1];
  m[2] = smatrix[2];
  m[3] = smatrix[3] - evalue;
  m[4] = smatrix[4];
  m[5] = smatrix[5] - evalue;

  // find the largest component
  float mc = std::fabs( m[0] );
  int mi = 0;
  for( int i = 1; i < 6; ++i )
  {
    float c = std::fabs( m[i] );
    if( c > mc )
    {
      mc = c;
      mi = i;
    }
  }

  // pick the first eigenvector based on this index
  switch( mi )
  {
    case 0:
    case 1:
      return Vec3( -m[1], m[0], 0.0f );

    case 2:
      return Vec3( m[2], 0.0f, -m[0] );

    case 3:
    case 4:
      return Vec3( 0.0f, -m[4], m[3] );

    default:
      return Vec3( 0.0f, -m[5], m[4] );
  }
}

Vec3 ComputePrincipleComponent( Sym3x3 const& smatrix )
{
  // compute the cubic coefficients
  float c0 = smatrix[0]*smatrix[3]*smatrix[5]
  + 2.0f*smatrix[1]*smatrix[2]*smatrix[4]
  - smatrix[0]*smatrix[4]*smatrix[4]
  - smatrix[3]*smatrix[2]*smatrix[2]
  - smatrix[5]*smatrix[1]*smatrix[1];
  float c1 = smatrix[0]*smatrix[3] + smatrix[0]*smatrix[5] + smatrix[3]*smatrix[5]
  - smatrix[1]*smatrix[1] - smatrix[2]*smatrix[2] - smatrix[4]*smatrix[4];
  float c2 = smatrix[0] + smatrix[3] + smatrix[5];

  // compute the quadratic coefficients
  float a = c1 - ( 1.0f/3.0f )*c2*c2;
  float b = ( -2.0f/27.0f )*c2*c2*c2 + ( 1.0f/3.0f )*c1*c2 - c0;

  // compute the root count check
  float Q = 0.25f*b*b + ( 1.0f/27.0f )*a*a*a;

  // test the multiplicity
  if( FLT_EPSILON < Q )
  {
    // only one root, which implies we have a multiple of the identity
    return Vec3( 1.0f );
  }
  else if( Q < -FLT_EPSILON )
  {
    // three distinct roots
    float theta = std::atan2( Vec4::sqrt( -Q ), -0.5f*b );
    float rho = Vec4::sqrt( 0.25f*b*b - Q );

//  float rt = std::pow( rho, 1.0f/3.0f );
    float rt = Vec4::cbrt( rho );
    float ct = std::cos( theta/3.0f );
    float st = std::sin( theta/3.0f );

    float l1 = ( 1.0f/3.0f )*c2 + 2.0f*rt*ct;
    float l2 = ( 1.0f/3.0f )*c2 - rt*( ct + ( float )sqrt( 3.0f )*st );
    float l3 = ( 1.0f/3.0f )*c2 - rt*( ct - ( float )sqrt( 3.0f )*st );

    // pick the larger
    if( std::fabs( l2 ) > std::fabs( l1 ) )
      l1 = l2;
    if( std::fabs( l3 ) > std::fabs( l1 ) )
      l1 = l3;

    // get the eigenvector
    return GetMultiplicity1Evector( smatrix, l1 );
  }
  else // if( -FLT_EPSILON <= Q && Q <= FLT_EPSILON )
  {
    // two roots
    float rt;
    if( b < 0.0f )
//    rt = -std::pow( -0.5f*b, 1.0f/3.0f );
      rt = -Vec4::cbrt( -0.5f*b );
    else
//    rt = std::pow( 0.5f*b, 1.0f/3.0f );
      rt = Vec4::cbrt( 0.5f*b );

    float l1 = ( 1.0f/3.0f )*c2 + rt;		// repeated
    float l2 = ( 1.0f/3.0f )*c2 - 2.0f*rt;

    // get the eigenvector
    if( std::fabs( l1 ) > std::fabs( l2 ) )
      return GetMultiplicity2Evector( smatrix, l1 );
    else
      return GetMultiplicity1Evector( smatrix, l2 );
  }
}

// sRGB spec
float basefpartition = 0.0031308f;
float baseipartition = 0.004045f;
float basefslope = 12.92f / 1.0f;
float baseislope = 1.0f / 12.92f;
float basefgamma = 2.4f / 1.0f;
float baseigamma = 1.0f / 2.4f;
float baseoffset = 0.055f;
float baseLUT_sRGB[256] = {
   0.0f,0.000303527f,0.00114819f,0.00132772f,0.00152264f,0.00173331f,0.00196007f,0.00220325f,
   0.00246318f,0.00274017f,0.00303452f,0.00334654f,0.00367651f,0.00402472f,0.00439144f,0.00477695f,
   0.00518152f,0.00560539f,0.00604883f,0.00651209f,0.00699541f,0.00749903f,0.00802319f,0.00856813f,
   0.00913406f,0.00972122f,0.0103298f,0.0109601f,0.0116122f,0.0122865f,0.012983f,0.0137021f,
   0.0144438f,0.0152085f,0.0159963f,0.0168074f,0.017642f,0.0185002f,0.0193824f,0.0202886f,
   0.021219f,0.0221739f,0.0231534f,0.0241576f,0.0251869f,0.0262412f,0.0273209f,0.028426f,
   0.0295568f,0.0307134f,0.031896f,0.0331048f,0.0343398f,0.0356013f,0.0368894f,0.0382044f,
   0.0395462f,0.0409152f,0.0423114f,0.043735f,0.0451862f,0.0466651f,0.0481718f,0.0497066f,
   0.0512695f,0.0528606f,0.0544803f,0.0561285f,0.0578054f,0.0595112f,0.0612461f,0.06301f,
   0.0648033f,0.0666259f,0.0684782f,0.0703601f,0.0722719f,0.0742136f,0.0761854f,0.0781874f,
   0.0802198f,0.0822827f,0.0843762f,0.0865005f,0.0886556f,0.0908417f,0.093059f,0.0953075f,
   0.0975873f,0.0998987f,0.102242f,0.104616f,0.107023f,0.109462f,0.111932f,0.114435f,
   0.116971f,0.119538f,0.122139f,0.124772f,0.127438f,0.130136f,0.132868f,0.135633f,
   0.138432f,0.141263f,0.144128f,0.147027f,0.14996f,0.152926f,0.155926f,0.158961f,
   0.162029f,0.165132f,0.168269f,0.171441f,0.174647f,0.177888f,0.181164f,0.184475f,
   0.187821f,0.191202f,0.194618f,0.198069f,0.201556f,0.205079f,0.208637f,0.212231f,
   0.215861f,0.219526f,0.223228f,0.226966f,0.23074f,0.234551f,0.238398f,0.242281f,
   0.246201f,0.250158f,0.254152f,0.258183f,0.262251f,0.266356f,0.270498f,0.274677f,
   0.278894f,0.283149f,0.287441f,0.291771f,0.296138f,0.300544f,0.304987f,0.309469f,
   0.313989f,0.318547f,0.323143f,0.327778f,0.332452f,0.337164f,0.341914f,0.346704f,
   0.351533f,0.3564f,0.361307f,0.366253f,0.371238f,0.376262f,0.381326f,0.386429f,
   0.391572f,0.396755f,0.401978f,0.40724f,0.412543f,0.417885f,0.423268f,0.42869f,
   0.434154f,0.439657f,0.445201f,0.450786f,0.456411f,0.462077f,0.467784f,0.473531f,
   0.47932f,0.48515f,0.491021f,0.496933f,0.502886f,0.508881f,0.514918f,0.520996f,
   0.527115f,0.533276f,0.539479f,0.545724f,0.552011f,0.55834f,0.564712f,0.571125f,
   0.57758f,0.584078f,0.590619f,0.597202f,0.603827f,0.610496f,0.617207f,0.62396f,
   0.630757f,0.637597f,0.64448f,0.651406f,0.658375f,0.665387f,0.672443f,0.679542f,
   0.686685f,0.693872f,0.701102f,0.708376f,0.715693f,0.723055f,0.730461f,0.73791f,
   0.745404f,0.752942f,0.760525f,0.768151f,0.775822f,0.783538f,0.791298f,0.799103f,
   0.806952f,0.814847f,0.822786f,0.83077f,0.838799f,0.846873f,0.854993f,0.863157f,
   0.871367f,0.879622f,0.887923f,0.896269f,0.904661f,0.913099f,0.921582f,0.930111f,
   0.938686f,0.947307f,0.955973f,0.964686f,0.973445f,0.982251f,0.991102f,1.0f};
float baseLUT_Linear[256] = {
   0.0f,0.00392157f,0.00784314f,0.0117647f,0.0156863f,0.0196078f,0.0235294f,0.027451f,
   0.0313726f,0.0352941f,0.0392157f,0.0431373f,0.0470588f,0.0509804f,0.054902f,0.0588235f,
   0.0627451f,0.0666667f,0.0705882f,0.0745098f,0.0784314f,0.0823529f,0.0862745f,0.0901961f,
   0.0941176f,0.0980392f,0.101961f,0.105882f,0.109804f,0.113725f,0.117647f,0.121569f,
   0.12549f,0.129412f,0.133333f,0.137255f,0.141176f,0.145098f,0.14902f,0.152941f,
   0.156863f,0.160784f,0.164706f,0.168627f,0.172549f,0.176471f,0.180392f,0.184314f,
   0.188235f,0.192157f,0.196078f,0.2f,0.203922f,0.207843f,0.211765f,0.215686f,
   0.219608f,0.223529f,0.227451f,0.231373f,0.235294f,0.239216f,0.243137f,0.247059f,
   0.25098f,0.254902f,0.258824f,0.262745f,0.266667f,0.270588f,0.27451f,0.278431f,
   0.282353f,0.286275f,0.290196f,0.294118f,0.298039f,0.301961f,0.305882f,0.309804f,
   0.313726f,0.317647f,0.321569f,0.32549f,0.329412f,0.333333f,0.337255f,0.341176f,
   0.345098f,0.34902f,0.352941f,0.356863f,0.360784f,0.364706f,0.368627f,0.372549f,
   0.376471f,0.380392f,0.384314f,0.388235f,0.392157f,0.396078f,0.4f,0.403922f,
   0.407843f,0.411765f,0.415686f,0.419608f,0.423529f,0.427451f,0.431373f,0.435294f,
   0.439216f,0.443137f,0.447059f,0.45098f,0.454902f,0.458824f,0.462745f,0.466667f,
   0.470588f,0.47451f,0.478431f,0.482353f,0.486275f,0.490196f,0.494118f,0.498039f,
   0.501961f,0.505882f,0.509804f,0.513726f,0.517647f,0.521569f,0.52549f,0.529412f,
   0.533333f,0.537255f,0.541176f,0.545098f,0.54902f,0.552941f,0.556863f,0.560784f,
   0.564706f,0.568627f,0.572549f,0.576471f,0.580392f,0.584314f,0.588235f,0.592157f,
   0.596078f,0.6f,0.603922f,0.607843f,0.611765f,0.615686f,0.619608f,0.623529f,
   0.627451f,0.631373f,0.635294f,0.639216f,0.643137f,0.647059f,0.65098f,0.654902f,
   0.658824f,0.662745f,0.666667f,0.670588f,0.67451f,0.678431f,0.682353f,0.686275f,
   0.690196f,0.694118f,0.698039f,0.701961f,0.705882f,0.709804f,0.713726f,0.717647f,
   0.721569f,0.72549f,0.729412f,0.733333f,0.737255f,0.741176f,0.745098f,0.74902f,
   0.752941f,0.756863f,0.760784f,0.764706f,0.768627f,0.772549f,0.776471f,0.780392f,
   0.784314f,0.788235f,0.792157f,0.796078f,0.8f,0.803922f,0.807843f,0.811765f,
   0.815686f,0.819608f,0.823529f,0.827451f,0.831373f,0.835294f,0.839216f,0.843137f,
   0.847059f,0.85098f,0.854902f,0.858824f,0.862745f,0.866667f,0.870588f,0.87451f,
   0.878431f,0.882353f,0.886275f,0.890196f,0.894118f,0.898039f,0.901961f,0.905882f,
   0.909804f,0.913725f,0.917647f,0.921569f,0.92549f,0.929412f,0.933333f,0.937255f,
   0.941176f,0.945098f,0.94902f,0.952941f,0.956863f,0.960784f,0.964706f,0.968627f,
   0.972549f,0.976471f,0.980392f,0.984314f,0.988235f,0.992157f,0.996078f,1.0f};

void SetGamma() {
  ;
}

const float *ComputeGammaLUT(bool sRGB) {
  return (sRGB ? baseLUT_sRGB : baseLUT_Linear);
}
#endif

} // namespace squish

#if	defined(USE_AMP) || defined(USE_COMPUTE)
#if	!defined(USE_COMPUTE)
// this one works on a vector<6>
#include "maths_vector.cpp"
#else
// this one works on two float4s
#include "maths_packed.cpp"
#endif
#endif