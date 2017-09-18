///////////////////////////////////////////////////////////////////////////////
// MicroOrk (Orkid)
// Copyright 1996-2013, Michael T. Mayers
// Provided under the MIT License (see LICENSE.txt)
///////////////////////////////////////////////////////////////////////////////

#pragma once

///////////////////////////////////////////////////////////////////////////////
#include "math_misc.h"
//#include "kernel.h"
#include "cvector3.h"
#include "cvector2.h"
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
namespace ork{
///////////////////////////////////////////////////////////////////////////////

template <typename T> class TMatrix4;
template <typename T> class TVector3;

//! templatized 4D vector
template <typename T> class  TVector4
{
	
public:

	TVector4();
	explicit TVector4( T _x, T _y, T _z, T _w=T(1.0f));							// constructor from 3 floats
	TVector4( const TVector4 &vec);											// constructor from a vector
	TVector4( const TVector3<T> &vec, T w=T(1.0f));							// constructor from a vector
	TVector4( u32 uval ) { SetRGBAU32( uval ); }
	~TVector4() {};															// default destructor, does nothing

	void				RotateX(T rad);
	void				RotateY(T rad);
	void				RotateZ(T rad);

	TVector4			Saturate() const; 
	T					Dot( const TVector4 &vec) const;					// dot product of two vectors
	TVector4			Cross( const TVector4 &vec) const;					// cross product of two vectors

	void				Normalize(void);									// normalize this vector
	TVector4			Normal() const;

	T					Mag(void) const;									// return magnitude of this vector
	T					MagSquared(void) const;								// return magnitude of this vector squared
	TVector4			Transform( const TMatrix4<T> &matrix) const;		// transform this vector
	void				PerspectiveDivide( void );

	void				Lerp( const TVector4 &from, const TVector4 &to, T par );
	void				Serp( const TVector4 & PA, const TVector4 & PB, const TVector4 & PC, const TVector4 & PD, T Par );

	T					GetX(void) const { return (x); }
	T					GetY(void) const { return (y); }
	T					GetZ(void) const { return (z); }
	T					GetW(void) const { return (w); }

	// sometimes 4-vectors are used for XYWH format
	T					GetWidth(void)  const { return (z); }
	T					GetHeight(void) const { return (w); }

	void			 	Set(T _x,T _y,T _z,T _w) { x = _x; y = _y; z = _z; w = _w;}
	void			 	Set(T _x,T _y,T _z) { x = _x; y = _y; z = _z; w = (T)1.0f;}
	void			 	SetX(T _x) { x = _x; }
	void				SetY(T _y) { y = _y; }
	void				SetZ(T _z) { z = _z; }
	void				SetW(T _w) { w = _w; }

	// sometimes 4-vectors are used for XYWH format
	void				SetWidth(T width)   { z = width;  }
	void				SetHeight(T height) { w = height; }

	TVector3<T>			GetXYZ( void ) const { return TVector3<T>(*this); }
	TVector2<T>			GetXY( void ) const { return TVector2<T>(x,y); }

	static	TVector4	Zero(void) { return TVector4(T(0), T(0), T(0), T(0)); }
	static T			CalcTriArea( const TVector4 &V0, const TVector4 &V1, const TVector4 &V2, const TVector4 & N );

	void SetXYZ( T _x, T _y, T _z )
	{
		x=_x;
		y=_y;
		z=_z;
	}

	inline T &operator[]( u32 i )
	{
		T *v = & x;
		assert( i<4 );
		return v[i];
	}

	inline const T &operator[]( u32 i ) const
	{
		const T *v = & x;
		assert( i<4 );
		return v[i];
	}

	inline TVector4 operator-() const
	{
		return TVector4( -x, -y, -z, -w );
	}

	inline TVector4 operator+( const TVector4 &b ) const
	{
		return TVector4( (x+b.x), (y+b.y), (z+b.z), (w+b.w) );
	}

	inline TVector4 operator*( const TVector4 &b ) const
	{
		return TVector4( (x*b.x), (y*b.y), (z*b.z), (w*b.w) );
	}

	inline TVector4 operator*( T scalar ) const
	{
		return TVector4( (x*scalar), (y*scalar), (z*scalar), (w*scalar) );
	}

	inline TVector4 operator-( const TVector4 &b ) const
	{
		return TVector4( (x-b.x), (y-b.y), (z-b.z), (w-b.w) );
	}

	inline TVector4 operator/( const TVector4 &b ) const
	{
		return TVector4( (x/b.x), (y/b.y), (z/b.z), (w/b.w) );
	}

	inline TVector4 operator/( T scalar ) const
	{
		return TVector4( (x/scalar), (y/scalar), (z/scalar), (w/scalar) );
	}

	inline void operator+=( const TVector4 & b )
	{
		x+=b.x;
		y+=b.y;
		z+=b.z;
		w+=b.w;
	}

	inline void operator-=( const TVector4 & b )
	{
		x-=b.x;
		y-=b.y;
		z-=b.z;
		w-=b.w;
	}

	inline void operator*=( T scalar )
	{
		x*=scalar;
		y*=scalar;
		z*=scalar;
		w*=scalar;
	}

	inline void operator*=( const TVector4 & b )
	{
		x*=b.x;
		y*=b.y;
		z*=b.z;
		w*=b.w;
	}

	inline void operator/=( const TVector4 &b )
	{
		x/=b.x;
		y/=b.y;
		z/=b.z;
		w/=b.w;
	}

	inline void operator/=( T scalar )
	{
		x/=scalar;
		y/=scalar;
		z/=scalar;
		w/=scalar;
	}

	inline bool operator==( const TVector4 &b ) const
	{
		return ( x == b.x && y == b.y && z == b.z && w == b.w );
	}
	inline bool operator!=( const TVector4 &b ) const
	{
		return ( x != b.x || y != b.y || z != b.z || w != b.w );
	}

	void SetHSV( T h, T s, T v );
	void SetRGB( T r, T g, T b ) { SetXYZ( r, g, b ); }
	u32 GetABGRU32( void ) const;
	u32 GetARGBU32( void ) const;
	u32 GetRGBAU32( void ) const;
	u32 GetBGRAU32( void ) const;
	u16 GetRGBU16( void ) const;
	u32 GetVtxColorAsU32(void) const;

	void SetRGBAU32( u32 uval );
	void SetBGRAU32( u32 uval );
	void SetARGBU32( u32 uval );
	void SetABGRU32( u32 uval );

	static const TVector4 & Black( void );
	static const TVector4 & DarkGrey( void );
	static const TVector4 & MediumGrey( void );
	static const TVector4 & LightGrey( void );
	static const TVector4 & White( void );

	static const TVector4 & Red( void );
	static const TVector4 & Green( void );
	static const TVector4 & Blue( void );
	static const TVector4 & Magenta( void );
	static const TVector4 & Cyan( void );
	static const TVector4 & Yellow( void );

	T *GetArray( void ) const { return const_cast<T*>( & x ); }

	template <typename U>
	static TVector4 FromTVector4(TVector4<U> vec)
	{
		return TVector4(T::FromFX(vec.GetX().FXCast()),
						T::FromFX(vec.GetY().FXCast()),
						T::FromFX(vec.GetZ().FXCast()),
						T::FromFX(vec.GetW().FXCast()));
	}

	T					x; // x component of this vector
	T					y; // y component of this vector
	T					z; // z component of this vector
	T					w;

};

typedef TVector4<float> CVector4;
typedef CVector4 CColor4;
typedef TVector4<float> fvec4;
typedef TVector4<double> dvec4;

///////////////////////////////////////////////////////////////////////////////
} // namespace ork
///////////////////////////////////////////////////////////////////////////////

template <typename T>
inline ork::TVector4<T> operator*( T scalar, const ork::TVector4<T> &b )
{
	return ork::TVector4<T>( (scalar*b.GetX()), (scalar*b.GetY()), (scalar*b.GetZ()) );
}

///////////////////////////////////////////////////////////////////////////////

