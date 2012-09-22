/* -----------------------------------------------------------------------------

	Copyright (c) 2006 Simon Brown                          si@sjbrown.co.uk

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

#ifndef SQUISH_SIMD_SSE_H
#define SQUISH_SIMD_SSE_H

#include <xmmintrin.h>
#if ( SQUISH_USE_SSE > 1 )
#include <emmintrin.h>
#endif
#if ( SQUISH_USE_SSE >= 3 )
#include <intrin.h>
#endif
#if ( SQUISH_USE_SSE >= 4 )
#include <smmintrin.h>
#endif
#if ( SQUISH_USE_XSSE == 4 )
#include <intrin.h>
#endif
#if ( SQUISH_USE_XSSE == 3 )
#include <tmmintrin.h>
#endif

#pragma warning(disable: 4127)

#define SQUISH_SSE_SPLAT( a )								\
	( ( a ) | ( ( a ) << 2 ) | ( ( a ) << 4 ) | ( ( a ) << 6 ) )

#define SQUISH_SSE_SHUF( x, y, z, w )							\
	( ( x ) | ( ( y ) << 2 ) | ( ( z ) << 4 ) | ( ( w ) << 6 ) )

#define SQUISH_SSE_SWAP64( )								\
	SQUISH_SSE_SHUF( 2, 3, 0, 1 )

#define SQUISH_SSE_SWAP32( )								\
	SQUISH_SSE_SHUF( 3, 2, 1, 0 )

namespace squish {

#define COL4_CONST( X ) Col4( X )

class Col4
{
public:
	typedef Col4 const& Arg;

	Col4() {}

	explicit Col4( __m128i v ) : m_v( v ) {}

	Col4( Col4 const& arg ) : m_v( arg.m_v ) {}

	Col4& operator=( Col4 const& arg )
	{
		m_v = arg.m_v;
		return *this;
	}

	explicit Col4( int s ) : m_v( _mm_set1_epi32( s ) ) {}
	explicit Col4( float s ) : m_v( _mm_set1_epi32( (int) s ) ) {}

	Col4( int r, int g, int b, int a ) : m_v( _mm_setr_epi32( r, g, b, a ) ) {}
	Col4( Col3 v, int w ) : m_v( _mm_setr_epi32( v.x, v.y, v.z, w ) ) {}

	explicit Col4( unsigned int s ) : m_v( _mm_set1_epi32( s ) ) {}
	explicit Col4( const unsigned int (&_rgba)[4] ) : m_v( _mm_load_si128( (const __m128i *)&_rgba ) ) {}
	explicit Col4( u8 const *source ) : m_v( _mm_loadu_si128( (const __m128i *)source ) ) {}

	Col3 GetCol3() const
	{
#ifdef __GNUC__
		__attribute__ ((__aligned__ (16))) int c[4];
		_mm_store_si128( (__m128i *)c, m_v );
		return Col3( c[0], c[1], c[2] );
#else
		return Col3( m_v.m128i_i32[0], m_v.m128i_i32[1], m_v.m128i_i32[2] );
#endif
	}

	int GetLong() const
	{
		return _mm_cvtsi128_si32 ( m_v );
	}

	Col4 SetLong( int v ) const
	{
		return Col4 ( _mm_cvtsi32_si128 ( v ) );
	}

	int R() const { return _mm_extract_epi16( m_v, 0 ); }
	int G() const { return _mm_extract_epi16( m_v, 2 ); }
	int B() const { return _mm_extract_epi16( m_v, 4 ); }
	int A() const { return _mm_extract_epi16( m_v, 6 ); }

	Col4 SplatR() const { return Col4( _mm_shuffle_epi32( m_v, SQUISH_SSE_SPLAT( 0 ) ) ); }
	Col4 SplatG() const { return Col4( _mm_shuffle_epi32( m_v, SQUISH_SSE_SPLAT( 1 ) ) ); }
	Col4 SplatB() const { return Col4( _mm_shuffle_epi32( m_v, SQUISH_SSE_SPLAT( 2 ) ) ); }
	Col4 SplatA() const { return Col4( _mm_shuffle_epi32( m_v, SQUISH_SSE_SPLAT( 3 ) ) ); }

	template<const int inv>
	void SetRGBA( int r, int g, int b, int a ) {
		__m128i v = _mm_setzero_si128();

		v = _mm_insert_epi16( v, r, 0 );
		v = _mm_insert_epi16( v, g, 2 );
		v = _mm_insert_epi16( v, b, 4 );
		v = _mm_insert_epi16( v, a, 6 );

		if (inv) {
			v = _mm_sub_epi32( _mm_set1_epi32( inv ), v );
		}

		m_v = v;
	}

	template<const int inv>
	void SetRGBApow2( int r, int g, int b, int a ) {
		__m128i v = _mm_setzero_si128();

		v = _mm_insert_epi16( v, r, 0 );
		v = _mm_insert_epi16( v, g, 2 );
		v = _mm_insert_epi16( v, b, 4 );
		v = _mm_insert_epi16( v, a, 6 );

		if (inv) {
			v = _mm_sub_epi32( _mm_set1_epi32( inv ), v );
		}

		v = _mm_slli_epi32( v, 23 );
		v = _mm_add_epi32( v, _mm_castps_si128( _mm_set1_ps(1.0f) ) );

		m_v = _mm_cvttps_epi32( _mm_castsi128_ps( v ) );
	}

	Col4& operator&=( Arg v )
	{
		m_v = _mm_and_si128( m_v, v.m_v );
		return *this;
	}

	Col4& operator^=( Arg v )
	{
		m_v = _mm_xor_si128( m_v, v.m_v );
		return *this;
	}

	Col4& operator|=( Arg v )
	{
		m_v = _mm_or_si128( m_v, v.m_v );
		return *this;
	}

	Col4& operator>>=( const int n )
	{
		m_v = _mm_srli_epi32( m_v, n );
		return *this;
	}

	Col4& operator<<=( const int n )
	{
		m_v = _mm_slli_epi32( m_v, n );
		return *this;
	}

	Col4& operator+=( Arg v )
	{
		m_v = _mm_add_epi32( m_v, v.m_v );
		return *this;
	}

	Col4& operator-=( Arg v )
	{
		m_v = _mm_sub_epi32( m_v, v.m_v );
		return *this;
	}

	Col4& operator*=( Arg v )
	{
	//	m_v = _mm_mul_epi32( m_v, v.m_v );
		m_v = _mm_mullo_epi16( m_v, v.m_v );
		return *this;
	}

	friend Col4 operator&( Col4::Arg left, Col4::Arg right  )
	{
		return Col4( _mm_and_si128( left.m_v, right.m_v ) );
	}

	friend Col4 operator^( Col4::Arg left, Col4::Arg right  )
	{
		return Col4( _mm_xor_si128( left.m_v, right.m_v ) );
	}

	friend Col4 operator|( Col4::Arg left, Col4::Arg right  )
	{
		return Col4( _mm_or_si128( left.m_v, right.m_v ) );
	}

	friend Col4 operator>>( Col4::Arg left, int right  )
	{
		return Col4( _mm_srli_epi32( left.m_v, right ) );
	}

	friend Col4 operator<<( Col4::Arg left, int right  )
	{
		return Col4( _mm_slli_epi32( left.m_v, right ) );
	}

	friend Col4 operator+( Col4::Arg left, Col4::Arg right  )
	{
		return Col4( _mm_add_epi32( left.m_v, right.m_v ) );
	}

	friend Col4 operator-( Col4::Arg left, Col4::Arg right  )
	{
		return Col4( _mm_sub_epi32( left.m_v, right.m_v ) );
	}

	friend Col4 operator*( Col4::Arg left, Col4::Arg right  )
	{
	//	return Col4( _mm_mul_epi32( left.m_v, right.m_v ) );
		return Col4( _mm_mullo_epi16( left.m_v, right.m_v ) );
	}

	friend Col4 operator*( Col4::Arg left, int right  )
	{
	//	return Col4( _mm_mul_epi32( left.m_v, _mm_set1_epi32( right ) ) );
		return Col4( _mm_mullo_epi16( left.m_v, _mm_set1_epi32( right ) ) );
	}

	template<const int n>
	friend Col4 ShiftLeft( Col4::Arg a )
	{
		if (n <= 0)
			return Col4( a.m_v );
		if (n & 7)
			return Col4( _mm_slli_epi32( _mm_slli_si128( a.m_v, n >> 3 ), n & 7 ) );

			return Col4( _mm_slli_si128( a.m_v, n >> 3 ) );
	}

	template<const int n>
	friend Col4 ShiftRight( Col4::Arg a )
	{
		if (n <= 0)
			return Col4( a.m_v );
		if (n & 7)
			return Col4( _mm_srli_epi32( _mm_srli_si128( a.m_v, n >> 3 ), n & 7 ) );

			return Col4( _mm_srli_si128( a.m_v, n >> 3 ) );
	}

	template<const int n>
	friend Col4 ShiftRightHalf( Col4::Arg a )
	{
		return Col4( n > 0 ? _mm_srli_epi64( a.m_v, n ) : a.m_v );
	}

	friend Col4 ShiftRightHalf( Col4::Arg a, const int n )
	{
		return Col4( _mm_srl_epi64( a.m_v, _mm_cvtsi32_si128( n ) ) );
	}

	friend Col4 ShiftRightHalf( Col4::Arg a, Col4::Arg b )
	{
		return Col4( _mm_srl_epi64( a.m_v, b.m_v ) );
	}

	template<const int n>
	friend Col4 ShiftLeftHalf( Col4::Arg a )
	{
		return Col4( n > 0 ? _mm_slli_epi64( a.m_v, n ) : a.m_v );
	}

	friend Col4 ShiftLeftHalf( Col4::Arg a, const int n )
	{
		return Col4( _mm_sll_epi64( a.m_v, _mm_cvtsi32_si128( n ) ) );
	}

	template<const int r, const int g, const int b, const int a>
	friend Col4 ShiftLeftLo( Col4::Arg v )
	{
		// (1 << r, 1 << g, 1 << b, 1 << a);
		Col4 p2; p2.SetRGBApow2<0>(r, g, b, a);

	//	return Col4( _mm_mul_epi32( v.m_v, p2.m_v ) );
		return Col4( _mm_mullo_epi16( v.m_v, p2.m_v ) );
	}

	template<const int n, const int p>
	friend Col4 MaskBits( Col4::Arg a )
	{
		if ((p + n) <= 0)
			return Col4(0);
		if ((p + n) >= 64)
			return a;

		// compile time
		__int64 base = ~(0xFFFFFFFFFFFFFFFFULL << (     (p + n) & 63));
	//	__int64 base =  (0xFFFFFFFFFFFFFFFFULL >> (64 - (p + n) & 63));
		__m128i mask = _mm_setr_epi32(
		  (int)(base >>  0),
		  (int)(base >> 32), 0, 0
		);

		return Col4( _mm_and_si128( a.m_v, mask ) );
	}

	friend Col4 MaskBits( Col4::Arg a, const int n, const int p )
	{
		const int val = 64 - (p + n);

		__m128i shift = _mm_max_epi16( _mm_cvtsi32_si128( val ), _mm_set1_epi32( 0 ) );
		__m128i mask = _mm_setr_epi32(
		  0xFFFFFFFF,
		  0xFFFFFFFF, 0, 0
		);

		mask = _mm_srl_epi64( mask, shift );

		// (0xFFFFFFFFFFFFFFFFULL >> (64 - (p + n) & 63))
		return Col4( _mm_and_si128( a.m_v, mask ) );
	}

	template<const int n, const int p>
	friend Col4 CopyBits( Col4::Arg left, Col4::Arg right )
	{
		if (!n)
			return left;
		if (!p)
			return MaskBits<n, 0>(right);
		if ((p + n) >= 64)
			return (left) | ShiftLeftHalf<p>(right);

#if ( SQUISH_USE_XSSE == 4 )
		return Col4( _mm_inserti_si64( left.m_v, right.m_v, n, p ) );
#else
		return MaskBits<p, 0>(left) | MaskBits<n, p>(ShiftLeftHalf<p>(right));
	//	return               (left) | MaskBits<n, p>(ShiftLeftHalf<p>(right));
#endif
	}

	friend Col4 CopyBits( Col4::Arg left, Col4 right, const int n, const int p )
	{
#if ( SQUISH_USE_XSSE == 4 )
		/* ---- ---bl xxxx xxxx */
		const int val = (p << 8) | (n << 0);

		right.m_v = _mm_unpacklo_epi64( right.m_v, _mm_cvtsi32_si128( val ) );
		return Col4( _mm_insert_si64( left.m_v, right.m_v ) );
#else
		return MaskBits(left, p, 0) | MaskBits(ShiftLeftHalf(right, p), n, p);
	//	return         (left      ) | MaskBits(ShiftLeftHalf(right, p), n, p);
#endif
	}

	template<const int n, const int p>
	friend Col4 ExtrBits( Col4::Arg a )
	{
		if (!n)
			return Col4(0);
		if (!p)
			return MaskBits<n, 0>(a);
		if ((n + p) >= 64)
			return ShiftRightHalf<p>(a);

#if ( SQUISH_USE_XSSE == 4 )
		return Col4( _mm_extracti_si64( a.m_v, n, p ) );
#else
		return MaskBits<n, 0>(ShiftRightHalf<p>(a));
#endif
	}

	friend Col4 ExtrBits( Col4::Arg a, const int n, const int p )
	{
#if ( SQUISH_USE_XSSE == 4 )
		/* ---- ----- ---- ---bl */
		const int val = (p << 8) | (n << 0);

		return Col4( _mm_extract_si64( a.m_v, _mm_cvtsi32_si128( val ) ) );
#else
		return MaskBits(ShiftRightHalf(a, p), n, 0);
#endif
	}

	template<const int n, const int p>
	friend void ExtrBits( Col4::Arg left, Col4 &right )
	{
		right  = ExtrBits<n, p>( left );
	}

	template<const int n, const int p>
	friend void ConcBits( Col4::Arg left, Col4 &right )
	{
		right  = ShiftLeft<32>( right );
		if (n > 0)
			right |= ExtrBits<n, p>( left );
	}

	template<const int n, const int p>
	friend void ReplBits( Col4::Arg left, Col4 &right )
	{
		if (!n)
			return;
		if ((n < 0)) {
			right  = ExtrBits<-n, p>( left );
			right.m_v = _mm_shuffle_epi32( right.m_v, SQUISH_SSE_SHUF( 0, 0, 0, 3 ) );
		}
		else {
			right  = ExtrBits< n, p>( left );
			right.m_v = _mm_shuffle_epi32( right.m_v, SQUISH_SSE_SHUF( 0, 0, 0, 0 ) );
		}
	}

	//! Returns a*b + c
	friend Col4 MultiplyAdd( Col4::Arg a, Col4::Arg b, Col4::Arg c )
	{
	//	return Col4( _mm_add_epi32( _mm_mul_epi32( a.m_v, b.m_v ), c.m_v ) );
		return Col4( _mm_add_epi32( _mm_mullo_epi16( a.m_v, b.m_v ), c.m_v ) );
	}

	//! Returns -( a*b - c )
	friend Col4 NegativeMultiplySubtract( Col4::Arg a, Col4::Arg b, Col4::Arg c )
	{
	//	return Col4( _mm_sub_epi32( c.m_v, _mm_mul_epi32( a.m_v, b.m_v ) ) );
		return Col4( _mm_sub_epi32( c.m_v, _mm_mullo_epi16( a.m_v, b.m_v ) ) );
	}

	template<const int f, const int t>
	friend Col4 Shuffle( Arg a )
	{
		if (f == t)
			return a;

		return Col4( _mm_shuffle_epi32( a.m_v, SQUISH_SSE_SHUF(
			(t == 0 ? f : 0),
			(t == 1 ? f : 1),
			(t == 2 ? f : 2),
			(t == 3 ? f : 3)
		) ) );
	}

	template<const int f, const int t>
	friend Col4 Exchange( Arg a )
	{
		if (f == t)
			return a;

		return Col4( _mm_shuffle_epi32( a.m_v, SQUISH_SSE_SHUF(
			(t == 0 ? f : (f == 0 ? t : 0)),
			(t == 1 ? f : (f == 1 ? t : 1)),
			(t == 2 ? f : (f == 2 ? t : 2)),
			(t == 3 ? f : (f == 3 ? t : 3))
		) ) );
	}

	friend Col4 HorizontalAdd( Arg a )
	{
#if ( SQUISH_USE_XSSE == 3 )
		__m128i res = _mm_hadd_epi32( a.m_v, a.m_v );
		return Col4( _mm_hadd_epi32( res, res ) );
#else
		__m128i res = a.m_v;

		res = _mm_add_epi32( res, _mm_shuffle_epi32( res, SQUISH_SSE_SWAP64() ) );
		res = _mm_add_epi32( res, _mm_shuffle_epi32( res, SQUISH_SSE_SWAP32() ) );

		return Col4( res );
#endif
	}

	friend Col4 HorizontalAdd( Arg a, Arg b )
	{
#if ( SQUISH_USE_XSSE == 3 )
		__m128i resc;

		resc = _mm_hadd_epi32( a.m_v, b.m_v );
		resc = _mm_hadd_epi32( resc, resc );
		resc = _mm_hadd_epi32( resc, resc );

		return Col4( resc );
#else
		__m128i resa = a.m_v;
		__m128i resb = b.m_v;
		__m128i resc;

		resc = _mm_add_epi32( resa, resb );
		resc = _mm_add_epi32( resc, _mm_shuffle_epi32( resc, SQUISH_SSE_SWAP64() ) );
		resc = _mm_add_epi32( resc, _mm_shuffle_epi32( resc, SQUISH_SSE_SWAP32() ) );

		return Col4( resc );
#endif
	}

	friend Col4 HorizontalAddTiny( Arg a )
	{
#if ( SQUISH_USE_XSSE == 4 ) || ( SQUISH_USE_SSE >= 3 )
		__m128 res = _mm_castsi128_ps ( a.m_v );

		// relies on correct de-normal floating-point treatment
		res = _mm_hadd_ps( res, res );
		res = _mm_hadd_ps( res, res );

		return Col4( _mm_castps_si128 ( res ) );
#else
		return HorizontalAdd( a );
#endif
	}

	friend Col4 HorizontalAddTiny( Arg a, Arg b )
	{
#if ( SQUISH_USE_XSSE == 4 ) || ( SQUISH_USE_SSE >= 3 )
		__m128 resa = _mm_castsi128_ps ( a.m_v );
		__m128 resb = _mm_castsi128_ps ( b.m_v );
		__m128 resc;

		// relies on correct de-normal floating-point treatment
		resc = _mm_hadd_ps( resa, resb );
		resc = _mm_hadd_ps( resc, resc );
		resc = _mm_hadd_ps( resc, resc );

		return Col4( _mm_castps_si128 ( resc ) );
#else
		return HorizontalAdd( a, b );
#endif
	}

	friend Col4 Dot( Arg left, Arg right )
	{
	//	return HorizontalAdd( Col4( _mm_mul_epi32( left.m_v, right.m_v ) ) );
		return HorizontalAdd( Col4( _mm_mullo_epi16( left.m_v, right.m_v ) ) );
	}

	friend Col4 DotTiny( Arg left, Arg right )
	{
	//	return HorizontalAdd( Col4( _mm_mul_epi16( left.m_v, right.m_v ) ) );
		return HorizontalAddTiny( Col4( _mm_mullo_epi16( left.m_v, right.m_v ) ) );
	}

	friend Col4 Min( Col4::Arg left, Col4::Arg right )
	{
	//	return Col4( _mm_min_epi32( left.m_v, right.m_v ) );
		return Col4( _mm_min_epi16( left.m_v, right.m_v ) );
	}

	friend Col4 Max( Col4::Arg left, Col4::Arg right )
	{
	//	return Col4( _mm_max_epi32( left.m_v, right.m_v ) );
		return Col4( _mm_max_epi16( left.m_v, right.m_v ) );
	}

	friend bool CompareAnyLessThan( Col4::Arg left, Col4::Arg right )
	{
		__m128i bits = _mm_cmpeq_epi32( left.m_v, right.m_v );
		int value = _mm_movemask_epi8( bits );
		return value != 0x0000;
	}

	friend bool CompareAllEqualTo( Col4::Arg left, Col4::Arg right )
	{
		__m128i bits = _mm_cmpeq_epi32( left.m_v, right.m_v );
		int value = _mm_movemask_epi8( bits );
		return value == 0xFFFF;
	}

	friend Col4 IsNotZero( Col4::Arg v )
	{
		return Col4( _mm_cmpgt_epi32( v.m_v, _mm_setzero_si128( ) ) );
	}

	friend Col4 IsOne( Col4::Arg v )
	{
		return Col4( _mm_cmpeq_epi32( v.m_v, _mm_set1_epi32( 0x000000FF ) ) );
	}

	friend Col4 TransferA( Col4::Arg left, Col4::Arg right )
	{
		__m128i l = _mm_and_si128( left.m_v , _mm_set_epi32( 0x00000000, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF ) );
		__m128i r = _mm_and_si128( right.m_v, _mm_set_epi32( 0xFFFFFFFF, 0x00000000, 0x00000000, 0x00000000 ) );

		return Col4( _mm_or_si128( l, r ) );
	}

	friend Col4 KillA( Col4::Arg left )
	{
		return Col4( _mm_or_si128( left.m_v, _mm_setr_epi32( 0x00, 0x00, 0x00, 0xFF ) ) );
	}

	friend void PackBytes( Col4::Arg a, int &loc )
	{
		__m128i

		r = _mm_packs_epi32( a.m_v, a.m_v );
		r = _mm_packus_epi16( r, r );

		loc = _mm_cvtsi128_si32 ( r );
	}

	// clamp the output to [0, 1]
	Col4 Clamp() const {
		Col4 const one (0xFF);
		Col4 const zero(0x00);

		return Min(one, Max(zero, *this));
	}

	friend void LoadAligned( Col4 &a, Col4 &b, Col4::Arg c )
	{
	        a.m_v = c.m_v;
		b.m_v = _mm_shuffle_epi32( a.m_v, SQUISH_SSE_SWAP64() );
	}

	friend void LoadAligned( Col4 &a, void const *source )
	{
		a.m_v = _mm_load_si128( (__m128i const *)source );
	}

	friend void LoadAligned( Col4 &a, Col4 &b, void const *source )
	{
		a.m_v = _mm_load_si128( (__m128i const *)source );
		b.m_v = _mm_shuffle_epi32( a.m_v, SQUISH_SSE_SWAP64() );
	}

	friend void LoadUnaligned( Col4 &a, Col4 &b, void const *source )
	{
		a.m_v = _mm_loadu_si128( (__m128i const *)source );
		b.m_v = _mm_shuffle_epi32( a.m_v, SQUISH_SSE_SWAP64() );
	}

	friend void StoreAligned( Col4::Arg a, Col4::Arg b, Col4 &c )
	{
		c.m_v = _mm_unpacklo_epi64( a.m_v, b.m_v );
	}

	friend void StoreAligned( Col4::Arg a, void *destination )
	{
		_mm_store_si128( (__m128i *)destination, a.m_v );
	}

	friend void StoreAligned( Col4::Arg a, Col4::Arg b, void *destination )
	{
		_mm_store_si128( (__m128i *)destination, _mm_unpacklo_epi64( a.m_v, b.m_v ) );
	}

	friend void StoreUnaligned( Col4::Arg a, Col4::Arg b, void *destination )
	{
		_mm_storeu_si128( (__m128i *)destination, _mm_unpacklo_epi64( a.m_v, b.m_v ) );
	}

private:
	__m128i m_v;
};

#if	!defined(USE_PRE)
#if 0
inline Col4 LengthSquared( Col4::Arg v )
{
  return Dot( v, v );
}
#endif

inline Col4 LengthSquaredTiny( Col4::Arg v )
{
  return DotTiny( v, v );
}
#endif

#define VEC4_CONST( X ) Vec4( X )

class Vec4
{
public:
	typedef Vec4 const& Arg;

	Vec4() {}

	explicit Vec4( __m128 v ) : m_v( v ) {}

	Vec4( Vec4 const& arg ) : m_v( arg.m_v ) {}

	Vec4& operator=( Vec4 const& arg )
	{
		m_v = arg.m_v;
		return *this;
	}

	explicit Vec4( float s ) : m_v( _mm_set1_ps( s ) ) {}
	explicit Vec4( int s ) : m_v( _mm_set1_ps( (float) s ) ) {}

	Vec4( float x, float y, float z, float w ) : m_v( _mm_setr_ps( x, y, z, w ) ) {}
	Vec4( Vec3 v, float w ) : m_v( _mm_setr_ps( v.x, v.y, v.z, w ) ) {}

	Vec3 GetVec3() const
	{
#ifdef __GNUC__
		__attribute__ ((__aligned__ (16))) float c[4];
#else
		__declspec(align(16)) float c[4];
#endif
		_mm_store_ps( c, m_v );
		return Vec3( c[0], c[1], c[2] );
	}

	float X() const { return ((float *)&m_v)[0]; }
	float Y() const { return ((float *)&m_v)[1]; }
	float Z() const { return ((float *)&m_v)[2]; }
	float W() const { return ((float *)&m_v)[3]; }

	float &GetX() { return ((float *)&m_v)[0]; }
	float &GetY() { return ((float *)&m_v)[1]; }
	float &GetZ() { return ((float *)&m_v)[2]; }
	float &GetW() { return ((float *)&m_v)[3]; }
	// let the compiler figure this one out, probably spills to memory
	float &GetO(int o) { return ((float *)&m_v)[o]; }

	Vec4 SplatX() const { return Vec4( _mm_shuffle_ps( m_v, m_v, SQUISH_SSE_SPLAT( 0 ) ) ); }
	Vec4 SplatY() const { return Vec4( _mm_shuffle_ps( m_v, m_v, SQUISH_SSE_SPLAT( 1 ) ) ); }
	Vec4 SplatZ() const { return Vec4( _mm_shuffle_ps( m_v, m_v, SQUISH_SSE_SPLAT( 2 ) ) ); }
	Vec4 SplatW() const { return Vec4( _mm_shuffle_ps( m_v, m_v, SQUISH_SSE_SPLAT( 3 ) ) ); }

	template<const int inv>
	void SetXYZW( int x, int y, int z, int w ) {
		__m128i v = _mm_setzero_si128();

		v = _mm_insert_epi16( v, x, 0 );
		v = _mm_insert_epi16( v, y, 2 );
		v = _mm_insert_epi16( v, z, 4 );
		v = _mm_insert_epi16( v, w, 6 );

		if (inv) {
			v = _mm_sub_epi32( _mm_set1_epi32( inv ), v );
		}

		m_v = _mm_cvtepi32_ps( v );
	}

	template<const int inv>
	void SetXYZWpow2( int x, int y, int z, int w ) {
		__m128i v = _mm_setzero_si128();

		v = _mm_insert_epi16( v, x, 0 );
		v = _mm_insert_epi16( v, y, 2 );
		v = _mm_insert_epi16( v, z, 4 );
		v = _mm_insert_epi16( v, w, 6 );

		if (inv) {
			v = _mm_sub_epi32( _mm_set1_epi32( inv ), v );
		}

		v = _mm_slli_epi32( v, 23 );
		v = _mm_add_epi32( v, _mm_castps_si128( _mm_set1_ps(1.0f) ) );

		m_v = _mm_castsi128_ps( v );
	}

	Vec4& operator+=( Arg v )
	{
		m_v = _mm_add_ps( m_v, v.m_v );
		return *this;
	}

	Vec4& operator-=( Arg v )
	{
		m_v = _mm_sub_ps( m_v, v.m_v );
		return *this;
	}

	Vec4& operator*=( Arg v )
	{
		m_v = _mm_mul_ps( m_v, v.m_v );
		return *this;
	}

	Vec4& operator/=( float v )
	{
		*this *= Reciprocal( Vec4( v ) );
		return *this;
	}

	friend Vec4 operator&( Vec4::Arg left, Vec4::Arg right  )
	{
		return Vec4( _mm_and_ps( left.m_v, right.m_v ) );
	}

	friend Vec4 operator+( Vec4::Arg left, Vec4::Arg right  )
	{
		return Vec4( _mm_add_ps( left.m_v, right.m_v ) );
	}

	friend Vec4 operator-( Vec4::Arg left, Vec4::Arg right  )
	{
		return Vec4( _mm_sub_ps( left.m_v, right.m_v ) );
	}

	friend Vec4 operator*( Vec4::Arg left, Vec4::Arg right  )
	{
		return Vec4( _mm_mul_ps( left.m_v, right.m_v ) );
	}

	friend Vec4 operator*( Vec4::Arg left, float right  )
	{
		return Vec4( _mm_mul_ps( left.m_v, _mm_set1_ps( right ) ) );
	}

	friend Vec4 operator*( float left, Vec4::Arg right )
	{
		return Vec4( _mm_mul_ps( _mm_set1_ps( left ), right.m_v ) );
	}

	friend Vec4 operator/( Vec4::Arg left, float right  )
	{
		return left * Reciprocal( Vec4( right ) );
	}

	friend Vec4 operator*( Vec4::Arg left, int right  )
	{
#if ( SQUISH_USE_SSE == 1 )
		...
#else
		return Vec4( _mm_mul_ps( left.m_v, _mm_cvtepi32_ps( _mm_set1_epi32( right ) ) ) );
#endif
	}

	//! Returns a*b + c
	friend Vec4 MultiplyAdd( Vec4::Arg a, Vec4::Arg b, Vec4::Arg c )
	{
		return Vec4( _mm_add_ps( _mm_mul_ps( a.m_v, b.m_v ), c.m_v ) );
	}

	//! Returns -( a*b - c )
	friend Vec4 NegativeMultiplySubtract( Vec4::Arg a, Vec4::Arg b, Vec4::Arg c )
	{
		return Vec4( _mm_sub_ps( c.m_v, _mm_mul_ps( a.m_v, b.m_v ) ) );
	}

	template<const int f, const int t>
	friend Vec4 Shuffle( Arg a )
	{
		if (f == t)
			return a;

		return Vec4( _mm_castsi128_ps( _mm_shuffle_epi32( _mm_castps_si128( a.m_v ), SQUISH_SSE_SHUF(
			(t == 0 ? f : 0),
			(t == 1 ? f : 1),
			(t == 2 ? f : 2),
			(t == 3 ? f : 3)
		) ) ) );
	}

	template<const int f, const int t>
	friend Vec4 Exchange( Arg a )
	{
		if (f == t)
			return a;

		return Vec4( _mm_castsi128_ps( _mm_shuffle_epi32( _mm_castps_si128( a.m_v ), SQUISH_SSE_SHUF(
			(t == 0 ? f : (f == 0 ? t : 0)),
			(t == 1 ? f : (f == 1 ? t : 1)),
			(t == 2 ? f : (f == 2 ? t : 2)),
			(t == 3 ? f : (f == 3 ? t : 3))
		) ) ) );
	}

	friend Vec4 HorizontalAdd( Arg a )
	{
#if ( SQUISH_USE_XSSE == 4 ) || ( SQUISH_USE_SSE >= 3 )
		__m128 res = a.m_v;

		res = _mm_hadd_ps( res, res );
		res = _mm_hadd_ps( res, res );

		return Vec4( res );
#else
		__m128 res = a.m_v;

		res = _mm_add_ps( res, _mm_shuffle_ps( res, res, SQUISH_SSE_SWAP64() ) );
		res = _mm_add_ps( res, _mm_shuffle_ps( res, res, SQUISH_SSE_SWAP32() ) );

		return Vec4( res );
#endif
	}

	friend Vec4 HorizontalAdd( Arg a, Arg b )
	{
#if ( SQUISH_USE_XSSE == 4 ) || ( SQUISH_USE_SSE >= 3 )
		__m128 resc;

		resc = _mm_hadd_ps( a.m_v, b.m_v );
		resc = _mm_hadd_ps( resc, resc );
		resc = _mm_hadd_ps( resc, resc );

		return Vec4( resc );
#else
		__m128 resc;

		resc = _mm_add_ps( a.m_v, b.m_v );
		resc = _mm_add_ps( resc, _mm_shuffle_ps( resc, resc, SQUISH_SSE_SWAP64() ) );
		resc = _mm_add_ps( resc, _mm_shuffle_ps( resc, resc, SQUISH_SSE_SWAP32() ) );

		return Vec4( resc );
#endif
	}

	friend Vec4 Reciprocal( Vec4::Arg v )
	{
		// get the reciprocal estimate
		__m128 estimate = _mm_rcp_ps( v.m_v );

		// one round of Newton-Rhaphson refinement
		__m128 diff = _mm_sub_ps( _mm_set1_ps( 1.0f ), _mm_mul_ps( estimate, v.m_v ) );
		return Vec4( _mm_add_ps( _mm_mul_ps( diff, estimate ), estimate ) );
	}

	friend Vec4 Dot( Arg left, Arg right )
	{
		return HorizontalAdd( Vec4( _mm_mul_ps( left.m_v, right.m_v ) ) );
	}

	friend void Dot( Arg left, Arg right, float *r )
	{
		Vec4 res = Dot( left, right );

		_mm_store_ss( r, res.m_v );
	}

	friend Vec4 Min( Vec4::Arg left, Vec4::Arg right )
	{
		return Vec4( _mm_min_ps( left.m_v, right.m_v ) );
	}

	friend Vec4 Max( Vec4::Arg left, Vec4::Arg right )
	{
		return Vec4( _mm_max_ps( left.m_v, right.m_v ) );
	}

	// clamp the output to [0, 1]
	Vec4 Clamp() const {
		Vec4 const one (1.0f);
		Vec4 const zero(0.0f);

		return Min(one, Max(zero, *this));
	}

	template<const bool round>
	friend Col4 FloatToInt( Vec4::Arg v )
	{
#if ( SQUISH_USE_SSE == 1 )
		...
#else
		// use SSE2 instructions
		if (round)
		      return Col4( _mm_cvttps_epi32( _mm_add_ps( v.m_v, _mm_set1_ps( 0.5f ) ) ) );
		else
		      return Col4( _mm_cvttps_epi32( v.m_v ) );
#endif
	}

	friend Vec4 Truncate( Vec4::Arg v )
	{
#if ( SQUISH_USE_SSE == 1 )
		// convert to ints
		__m128 input = v.m_v;
		__m64 lo = _mm_cvttps_pi32( input );
		__m64 hi = _mm_cvttps_pi32( _mm_movehl_ps( input, input ) );

		// convert to floats
		__m128 part = _mm_movelh_ps( input, _mm_cvtpi32_ps( input, hi ) );
		__m128 truncated = _mm_cvtpi32_ps( part, lo );

		// clear out the MMX multimedia state to allow FP calls later
		_mm_empty();
		return Vec4( truncated );
#else
		// use SSE2 instructions
		return Vec4( _mm_cvtepi32_ps( _mm_cvttps_epi32( v.m_v ) ) );
#endif
	}

	friend bool CompareAnyLessThan( Vec4::Arg left, Vec4::Arg right )
	{
		__m128 bits = _mm_cmplt_ps( left.m_v, right.m_v );
		int value = _mm_movemask_ps( bits );
		return value != 0;
	}

	friend int CompareFirstLessThan( Vec4::Arg left, Vec4::Arg right )
	{
		return _mm_comilt_ss( left.m_v, right.m_v );
	}

	friend int CompareFirstGreaterThan( Vec4::Arg left, Vec4::Arg right )
	{
		return _mm_comigt_ss( left.m_v, right.m_v );
	}

	Vec4 IsNotOne( ) const
	{
		return Vec4( _mm_cmpneq_ps( m_v, _mm_set1_ps( 1.0f ) ) );
	}

	friend Vec4 TransferW( Vec4::Arg left, Vec4::Arg right )
	{
		/* [new W, ....., ....., old Z] */
		//m128 u = _mm_unpackhi_ps( left.m_v, right.m_v );
		/* [new W, new W, old Z, old Z] */
		__m128 u = _mm_shuffle_ps( left.m_v, right.m_v, SQUISH_SSE_SHUF( 2, 2, 3, 3 ) );
		/* [new W, old Z, old Y, old X] */
		       u = _mm_shuffle_ps( left.m_v, u, SQUISH_SSE_SHUF( 0, 1, 0, 2 ) );

		return Vec4( u );
	}

	friend Vec4 KillW( Vec4::Arg left )
	{
		return Vec4( _mm_and_ps( left.m_v, _mm_castsi128_ps ( _mm_setr_epi32( ~0, ~0, ~0,  0 ) ) ) );
	}

	friend Vec4 OnlyW( Vec4::Arg left )
	{
		return Vec4( _mm_and_ps( left.m_v, _mm_castsi128_ps ( _mm_setr_epi32(  0,  0,  0, ~0 ) ) ) );
	}

	void SwapXYZW( Vec4 &with )
	{
		/* inplace swap based on xors */
		     m_v = _mm_xor_ps( m_v, with.m_v );
		with.m_v = _mm_xor_ps( with.m_v, m_v );
		     m_v = _mm_xor_ps( m_v, with.m_v );
	}

	void SwapXYZ ( Vec4 &with )
	{
		/* [old W, old W, new Z, new Z] */
		__m128 u = _mm_shuffle_ps( m_v, with.m_v, SQUISH_SSE_SHUF( 3, 3, 2, 2 ) );
		__m128 v = _mm_shuffle_ps( with.m_v, m_v, SQUISH_SSE_SHUF( 3, 3, 2, 2 ) );
		__m128 w = m_v;

		/* [new X, new Y, new Z, old W] */
		     m_v = _mm_shuffle_ps( with.m_v, u, SQUISH_SSE_SHUF( 0, 1, 2, 0 ) );
		with.m_v = _mm_shuffle_ps(        w, v, SQUISH_SSE_SHUF( 0, 1, 2, 0 ) );
	}

	void SwapW   ( Vec4 &with )
	{
		/* [old Z, old Z, new W, new W] */
		__m128 u = _mm_shuffle_ps( m_v, with.m_v, SQUISH_SSE_SHUF( 2, 2, 3, 3 ) );
		__m128 v = _mm_shuffle_ps( with.m_v, m_v, SQUISH_SSE_SHUF( 2, 2, 3, 3 ) );

		/* [old X, old Y, old Z, new W] */
		     m_v = _mm_shuffle_ps(      m_v, u, SQUISH_SSE_SHUF( 0, 1, 0, 2 ) );
		with.m_v = _mm_shuffle_ps( with.m_v, v, SQUISH_SSE_SHUF( 0, 1, 0, 2 ) );
	}

	// 5-6% of execution time as std::sqrt
	static doinline float sqrt(float in) {
		__m128 s;
		float r;

		s = _mm_load_ss(&in);
		s = _mm_sqrt_ss(s);

		_mm_store_ss(&r, s);
		return r;
	}

	// 3-5% of execution time as std::pow
	static doinline float cbrt(float in) {
		__m128 n, x, c, u, v;
		float r;
	//	float check = std::pow(in, 1.0f/3.0f);

	//	n = _mm_load_ss(&check);
		n = _mm_load_ss(&in);
		// initial guess, poor: sqrt( n ) + 1e-15
		x = _mm_add_ss(_mm_sqrt_ss( n ), _mm_set1_ps(1e-15f));
		// initial guess, cool hack: Y = (((Y >> 17) * 0xAAAB)) + (709921077L << 0);
		x = _mm_castsi128_ps(_mm_add_epi32(_mm_mul_epu32(_mm_srli_epi32(_mm_castps_si128( n ), 17), _mm_set1_epi32(0xAAAB)), _mm_set1_epi32(709921077L)));

		for (int i = 0; i < 1; i++) {
		  // X * X * X
		  c = _mm_mul_ss(x, _mm_mul_ss(x, x));

		  // ((c * t) + (n * f))
		  u = _mm_add_ss(_mm_mul_ss(c, _mm_set1_ps(2)), _mm_mul_ss(n, _mm_set1_ps(4)));
		  // ((c * f) + (n * t))
		  v = _mm_add_ss(_mm_mul_ss(c, _mm_set1_ps(4)), _mm_mul_ss(n, _mm_set1_ps(2)));

		  // 1 / u
		  __m128 estimate = _mm_rcp_ss(v);
		  __m128 diff = _mm_sub_ss(_mm_set1_ps(1.0f), _mm_mul_ss(estimate, v));
		  v = _mm_add_ss(_mm_mul_ss(diff, estimate), estimate);

		  // ((c * t) + (n * f)) * (x) / ((c * f) + (n * t))
		  x = _mm_mul_ss(x, _mm_mul_ss(u, v));
		}

		_mm_store_ss(&r, x);
		return r;
	}

private:
	__m128 m_v;
};

// TODO: figure out how to put static const instances into an incomplete class body
namespace Vec4C {

  const Vec4 zero = Vec4(0.0f);
  const Vec4 one = Vec4(1.0f);
  const Vec4 half = Vec4(0.5f);

}

#if	!defined(USE_PRE)
inline Vec4 LengthSquared( Vec4::Arg v )
{
  return Dot( v, v );
}

inline void LengthSquared( Vec4::Arg v , float *r )
{
  Dot( v, v, r );
}
#endif

} // namespace squish

#endif // ndef SQUISH_SIMD_SSE_H