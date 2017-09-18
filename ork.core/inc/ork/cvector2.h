///////////////////////////////////////////////////////////////////////////////
// MicroOrk (Orkid)
// Copyright 1996-2013, Michael T. Mayers
// Provided under the MIT License (see LICENSE.txt)
///////////////////////////////////////////////////////////////////////////////

#pragma once

//#include "kernel.h"
#include "math_misc.h"
#include <assert.h>

///////////////////////////////////////////////////////////////////////////////
namespace ork{
///////////////////////////////////////////////////////////////////////////////

template <typename T> class TVector3;

//! templatized 2D vector
template <typename T> class  TVector2
{

public:

	TVector2();
	explicit TVector2( T _x, T _y );
	TVector2( const TVector2 &vec);
	TVector2( const TVector3<T> &vec);
	~TVector2() {};															// default destructor, does nothing

	void				Rotate(T rad);

	T					Dot( const TVector2 &vec) const;					// dot product of two vectors
	T					PerpDot( const TVector2& vec ) const;

	void				Normalize(void);									// normalize this vector
	TVector2			Normal() const;

	T					Mag(void) const;									// return magnitude of this vector
	T					Length(void) const { return Mag(); }
	T					MagSquared(void) const;								// return magnitude of this vector squared

	void				Lerp( const TVector2 &from, const TVector2 &to, T par );
	void				Serp( const TVector2 & PA, const TVector2 & PB, const TVector2 & PC, const TVector2 & PD, T Par );
	TVector2 			Perp() const { return TVector2(-y,x); }

	T					GetX(void) const { return (x); }
	T					GetY(void) const { return (y); }

	void			 	Set(T _x, T _y) { x = _x; y = _y;}
	void			 	SetX(T _x) { x = _x; }
	void				SetY(T _y) { y = _y; }

	static	TVector2	Zero(void) { return TVector2(T(0), T(0)); }

	inline T &operator[]( u32 i )
	{
		T *v = & x;
		assert( i<2 );
		return v[i];
	}

	inline TVector2 operator-() const
	{
		return TVector2( -x, -y );
	}

	inline TVector2 operator+( const TVector2 &b ) const
	{
		return TVector2( (x+b.x), (y+b.y) );
	}

	inline TVector2 operator*( const TVector2 &b ) const
	{
		return TVector2( (x*b.x), (y*b.y) );
	}

	inline TVector2 operator*( T scalar ) const
	{
		return TVector2( (x*scalar), (y*scalar) );
	}

	inline TVector2 operator-( const TVector2 &b ) const
	{
		return TVector2( (x-b.x), (y-b.y) );
	}

	inline TVector2 operator/( const TVector2 &b ) const
	{
		return TVector2( (x/b.x), (y/b.y) );
	}

	inline TVector2 operator/( T scalar ) const
	{
		return TVector2( (x/scalar), (y/scalar) );
	}

	inline void operator+=( const TVector2 & b )
	{
		x+=b.x;
		y+=b.y;
	}

	inline void operator-=( const TVector2 & b )
	{
		x-=b.x;
		y-=b.y;
	}

	inline void operator*=( T scalar )
	{
		x*=scalar;
		y*=scalar;
	}

	inline void operator*=( const TVector2 & b )
	{
		x*=b.x;
		y*=b.y;
	}

	inline void operator/=( const TVector2 &b )
	{
		x/=b.x;
		y/=b.y;
	}

	inline void operator/=( T scalar )
	{
		x/=scalar;
		y/=scalar;
	}

	inline bool operator==( const TVector2 &b ) const
	{
		return ( x == b.x && y == b.y );
	}
	inline bool operator!=( const TVector2 &b ) const
	{
		return ( x != b.x || y != b.y );
	}

	T *GetArray( void ) const { return const_cast<T*>( & x ); }

	/*template <typename U>
	static TVector2 FromTVector2(TVector2<U> vec)
	{
		return TVector2(T::FromFX(vec.GetX().FXCast()),
						T::FromFX(vec.GetY().FXCast()));
	}*/

	T					x; // x component of this vector
	T					y; // y component of this vector

};

typedef TVector2<float> CVector2;
typedef TVector2<float> fvec2;
typedef TVector2<double> dvec2;

///////////////////////////////////////////////////////////////////////////////
} // namespace ork
///////////////////////////////////////////////////////////////////////////////

template <typename T>
inline ork::TVector2<T> operator*( T scalar, const ork::TVector2<T> &b )
{
	return ork::TVector2<T>( (scalar*b.GetX()), (scalar*b.GetY()) );
}

///////////////////////////////////////////////////////////////////////////////

