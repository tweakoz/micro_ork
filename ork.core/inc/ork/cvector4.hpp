///////////////////////////////////////////////////////////////////////////////
// MicroOrk (Orkid)
// Copyright 1996-2013, Michael T. Mayers
// Provided under the MIT License (see LICENSE.txt)
///////////////////////////////////////////////////////////////////////////////

#pragma once

#if defined(_WIN32) && ! defined(_XBOX)
#include <pmmintrin.h>
#endif

namespace ork {

///////////////////////////////////////////////////////////////////////////////

template <typename T> TVector4<T> TVector4<T>::Saturate( void ) const
{
	TVector4<T> rval = *this;
	rval.x = (rval.x>1.0f) ? 1.0f : (rval.x<0.0f) ? 0.0f : rval.x;
	rval.y = (rval.y>1.0f) ? 1.0f : (rval.y<0.0f) ? 0.0f : rval.y;
	rval.z = (rval.z>1.0f) ? 1.0f : (rval.z<0.0f) ? 0.0f : rval.z;
	rval.w = (rval.w>1.0f) ? 1.0f : (rval.w<0.0f) ? 0.0f : rval.w;
	return rval;
}

///////////////////////////////////////////////////////////////////////////////

template <typename T> const TVector4<T> & TVector4<T>::Black( void )
{
	static const TVector4<T> Black( T(0.0f), T(0.0f), T(0.0f), T(1.0f) );
	return Black;
}

///////////////////////////////////////////////////////////////////////////////

template <typename T> const TVector4<T> & TVector4<T>::DarkGrey( void )
{
	static const TVector4<T> DarkGrey( T(0.250f), T(0.250f), T(0.250f), T(1.0f) );
	return DarkGrey;
}

///////////////////////////////////////////////////////////////////////////////

template <typename T> const TVector4<T> & TVector4<T>::MediumGrey( void )
{
	static const TVector4<T> MediumGrey( T(0.50f), T(0.50f), T(0.50f), T(1.0f) );
	return MediumGrey;
}

///////////////////////////////////////////////////////////////////////////////

template <typename T> const TVector4<T> & TVector4<T>::LightGrey( void )
{
	static const TVector4<T> LightGrey(T(0.75f), T(0.75f), T(0.75f), T(1.0f) );
	return LightGrey;
}

///////////////////////////////////////////////////////////////////////////////

template <typename T> const TVector4<T> & TVector4<T>::White( void )
{
	static const TVector4<T> White(  T(1.0f), T(1.0f), T(1.0f), T(1.0f) );
	return White;
}

///////////////////////////////////////////////////////////////////////////////

template <typename T> const TVector4<T> & TVector4<T>::Red( void )
{
	static const TVector4<T> Red( T(1.0f), T(0.0f), T(0.0f), T(1.0f) );
	return Red;
}

///////////////////////////////////////////////////////////////////////////////

template <typename T> const TVector4<T> & TVector4<T>::Green( void )
{
	static const TVector4<T> Green( T(0.0f), T(1.0f), T(0.0f), T(1.0f)  );
	return Green;
}

///////////////////////////////////////////////////////////////////////////////

template <typename T> const TVector4<T> & TVector4<T>::Blue( void )
{
	static const TVector4<T> Blue( T(0.0f), T(0.0f), T(1.0f), T(1.0f) );
	return Blue;
}

///////////////////////////////////////////////////////////////////////////////

template <typename T> const TVector4<T> & TVector4<T>::Magenta( void )
{
	static const TVector4<T> Magenta( T(1.0f), T(0.0f), T(1.0f), T(1.0f) );
	return Magenta;
}

///////////////////////////////////////////////////////////////////////////////

template <typename T> const TVector4<T> & TVector4<T>::Cyan( void )
{
	static const TVector4<T> Cyan( T(0.0f), T(1.0f), T(1.0f), T(1.0f));
	return Cyan;
}

///////////////////////////////////////////////////////////////////////////////

template <typename T> const TVector4<T> & TVector4<T>::Yellow( void )
{
	static const TVector4<T> Yellow( T(1.0f), T(1.0f), T(0.0f), T(1.0f) );
	return Yellow;
}

///////////////////////////////////////////////////////////////////////////////

template <typename T> TVector4<T>::TVector4()
	: x(T(0.0f))
	, y(T(0.0f))
	, z(T(0.0f))
	, w(T(1.0f))
{
}

///////////////////////////////////////////////////////////////////////////////

template <typename T> TVector4<T>::TVector4( T _x, T _y, T _z, T _w)

	: x(_x)
	, y(_y)
	, z(_z)
	, w(_w)
{
}

///////////////////////////////////////////////////////////////////////////////

template <typename T> TVector4<T>::TVector4( const TVector3<T> & in, T w )
	: x(in.GetX())
	, y(in.GetY())
	, z(in.GetZ())
	, w(w)
{
}

////////////////////////////////////////////////////////////////////////////////

template <typename T> u32 TVector4<T>::GetVtxColorAsU32( void ) const
{	u32 r = u32(GetX()*T(255.0f));
	u32 g = u32(GetY()*T(255.0f));
	u32 b = u32(GetZ()*T(255.0f));
	u32 a = u32(GetW()*T(255.0f));
	
#if 1 //defined(_DARWIN)//GL
	return u32( (a<<24)|(b<<16)|(g<<8)|r );
#else // WIN32/DX
	return u32( (a<<24)|(r<<16)|(g<<8)|b );
#endif
}

////////////////////////////////////////////////////////////////////////////////

template <typename T> u32 TVector4<T>::GetABGRU32( void ) const
{	u32 r = u32(GetX()*T(255.0f));
	u32 g = u32(GetY()*T(255.0f));
	u32 b = u32(GetZ()*T(255.0f));
	u32 a = u32(GetW()*T(255.0f));
	
	return u32( (a<<24)|(b<<16)|(g<<8)|r );
}

///////////////////////////////////////////////////////////////////////////////

template <typename T> u32 TVector4<T>::GetARGBU32( void ) const
{	u32 r = u32(GetX()*T(255.0f));
	u32 g = u32(GetY()*T(255.0f));
	u32 b = u32(GetZ()*T(255.0f));
	u32 a = u32(GetW()*T(255.0f));
	
	return u32( (a<<24)|(r<<16)|(g<<8)|b );
}

///////////////////////////////////////////////////////////////////////////////

template <typename T> u32 TVector4<T>::GetRGBAU32( void ) const 
{	
	s32 r = u32(GetX()*T(255.0f));
	s32 g = u32(GetY()*T(255.0f));
	s32 b = u32(GetZ()*T(255.0f));
	s32 a = u32(GetW()*T(255.0f));

	if( r<0 ) r=0;
	if( g<0 ) g=0;
	if( b<0 ) b=0;
	if( a<0 ) a=0;

	u32 rval = 0;

#if defined(__sgi)
	rval = ( (b<<24)|(g<<16)|(r<<8)|a );
#else
	//rval = ( (a<<24)|(r<<16)|(g<<8)|b );
	rval = ( (a<<24) );
#endif

	return rval;
}

///////////////////////////////////////////////////////////////////////////////

template <typename T> u32 TVector4<T>::GetBGRAU32( void ) const
{	u32 r = u32(GetX()*T(255.0f));
	u32 g = u32(GetY()*T(255.0f));
	u32 b = u32(GetZ()*T(255.0f));
	u32 a = u32(GetW()*T(255.0f));
	
	return u32( (b<<24)|(g<<16)|(r<<8)|a );
}

///////////////////////////////////////////////////////////////////////////////

template <typename T> u16 TVector4<T>::GetRGBU16( void ) const 
{
	u32 r = u32(GetX() * T(31.0f));
	u32 g = u32(GetY() * T(31.0f));
	u32 b = u32(GetZ() * T(31.0f));
								
	u16 rval = u16((b<<10)|(g<<5)|r);

	return rval;
}

///////////////////////////////////////////////////////////////////////////////

template <typename T> void TVector4<T>::SetRGBAU32( u32 uval ) 
{	
	u32 r = (uval>>24)&0xff;
	u32 g = (uval>>16)&0xff;
	u32 b = (uval>>8)&0xff;
	u32 a = (uval)&0xff;

	static const T kfic( 1.0f / 255.0f );

	SetX(kfic * T(int(r)));
	SetY(kfic * T(int(g)));
	SetZ(kfic * T(int(b)));
	SetW(kfic * T(int(a)));
}

///////////////////////////////////////////////////////////////////////////////

template <typename T> void TVector4<T>::SetBGRAU32( u32 uval ) 
{	
	u32 b = (uval>>24)&0xff;
	u32 g = (uval>>16)&0xff;
	u32 r = (uval>>8)&0xff;
	u32 a = (uval)&0xff;

	static const T kfic( 1.0f / 255.0f );

	SetX(kfic * T(int(r)));
	SetY(kfic * T(int(g)));
	SetZ(kfic * T(int(b)));
	SetW(kfic * T(int(a)));
}

///////////////////////////////////////////////////////////////////////////////

template <typename T> void TVector4<T>::SetARGBU32( u32 uval ) 
{	
	u32 a = (uval>>24)&0xff;
	u32 r = (uval>>16)&0xff;
	u32 g = (uval>>8)&0xff;
	u32 b = (uval)&0xff;

	static const T kfic( 1.0f / 255.0f );

	SetX(kfic * T(int(r)));
	SetY(kfic * T(int(g)));
	SetZ(kfic * T(int(b)));
	SetW(kfic * T(int(a)));
}

///////////////////////////////////////////////////////////////////////////////

template <typename T> void TVector4<T>::SetABGRU32( u32 uval ) 
{	
	u32 a = (uval>>24)&0xff;
	u32 b = (uval>>16)&0xff;
	u32 g = (uval>>8)&0xff;
	u32 r = (uval)&0xff;

	static const T kfic( 1.0f / 255.0f );

	SetX(kfic * T(int(r)));
	SetY(kfic * T(int(g)));
	SetZ(kfic * T(int(b)));
	SetW(kfic * T(int(a)));
}

///////////////////////////////////////////////////////////////////////////////

template <typename T> void TVector4<T>::SetHSV( T h, T s, T v )
{
//	hsv.x = saturate(hsv.x);
//	hsv.y = saturate(hsv.y);
//	hsv.z = saturate(hsv.z);

	if ( s == 0.0f )
	{
		// Grayscale 
		SetX(v);
		SetY(v);
		SetZ(v);
	}
	else
	{
		const T kone(1.0f);

		if (kone <= h) h -= kone;
		h *= 6.0f;
		T i = T(floor(h));
		T f = h - i;
		T aa = v * (kone - s);
		T bb = v * (kone - (s * f));
		T cc = v * (kone - (s * (kone-f)));
		if( i < kone )
		{
			SetX( v );
			SetY( cc );
			SetZ( aa );
		}
		else if( i < 2.0f )
		{
			SetX( bb );
			SetY( v );
			SetZ( aa );
		}
		else if( i < 3.0f )
		{
			SetX( aa );
			SetY( v );
			SetZ( cc );
		}
		else if( i < 4.0f )
		{
			SetX( aa );
			SetY( bb );
			SetZ( v );
		}
		else if( i < 5.0f )
		{
			SetX( cc );
			SetY( aa );
			SetZ( v );
		}
		else
		{
			SetX( v );
			SetY( aa );
			SetZ( bb );
		}
	}
}

///////////////////////////////////////////////////////////////////////////////

template <typename T> void TVector4<T>::PerspectiveDivide( void )
{
	T iw = T(1.0f) / w;
	x *= iw;
	y *= iw;
	z *= iw;
	w = T(1.0f);
}

////////////////////////////////////////////////////////////////////////////////

template <typename T> TVector4<T>::TVector4( const TVector4<T> &vec)
{
	x = vec.x;
	y = vec.y;
	z = vec.z;
	w = vec.w;
}

////////////////////////////////////////////////////////////////////////////////

template <typename T> T TVector4<T>::Dot( const TVector4<T> &vec) const
{
	return ( (x * vec.x) + (y * vec.y) + (z * vec.z) );
}

////////////////////////////////////////////////////////////////////////////////

template <typename T> TVector4<T> TVector4<T>::Cross( const TVector4<T> &vec) const // c = this X vec
{
	T vx = ((y * vec.GetZ()) - (z * vec.GetY()));
	T vy = ((z * vec.GetX()) - (x * vec.GetZ()));
	T vz = ((x * vec.GetY()) - (y * vec.GetX()));

	return ( TVector4<T>( vx, vy, vz ) );
}

////////////////////////////////////////////////////////////////////////////////

template <typename T> void TVector4<T>::Normalize(void)
{
	T	distance = (T) 1.0f / Mag() ;

	x *= distance;
	y *= distance;
	z *= distance;
	
}

////////////////////////////////////////////////////////////////////////////////

template <typename T> TVector4<T> TVector4<T>::Normal() const
{
	T fmag = Mag();
	fmag = (fmag==(T)0.0f) ? (T)0.00001f : fmag;
	T	s = (T) 1.0f / fmag;
	return TVector4<T>(x * s, y * s, z * s, w);
}

////////////////////////////////////////////////////////////////////////////////

template <typename T> T TVector4<T>::Mag(void) const
{
	return sqrt(x * x + y * y + z * z);
}

////////////////////////////////////////////////////////////////////////////////

template <typename T> T TVector4<T>::MagSquared(void) const
{
	T mag = (x * x + y * y + z * z);
	return mag;
}

////////////////////////////////////////////////////////////////////////////////

template <typename T> TVector4<T> TVector4<T>::Transform( const TMatrix4<T> &matrix ) const
{
	T	tx,ty,tz,tw;

	T *mp = (T *) matrix.elements;
	T _x = x;
	T _y = y;
	T _z = z;
	T _w = w;

	tx = _x*mp[0] + _y*mp[4] + _z*mp[8] + _w*mp[12];
	ty = _x*mp[1] + _y*mp[5] + _z*mp[9] + _w*mp[13];
	tz = _x*mp[2] + _y*mp[6] + _z*mp[10] + _w*mp[14];
	tw = _x*mp[3] + _y*mp[7] + _z*mp[11] + _w*mp[15];

	return( TVector4<T>( tx,ty,tz, tw ) );
}

////////////////////////////////////////////////////////////////////////////////

template <typename T> void TVector4<T>::Serp( const TVector4<T> & PA, const TVector4<T> & PB, const TVector4<T> & PC, const TVector4<T> & PD, T Par )
{
	TVector4<T> PAB, PCD;
	PAB.Lerp( PA, PB, Par );
	PCD.Lerp( PC, PD, Par );
	Lerp( PAB, PCD, Par );
}

////////////////////////////////////////////////////////////////////////////////

template <typename T> void TVector4<T>::RotateX(T rad)
{
	T	oldY = y;
	T	oldZ = z;
	y = (oldY * cos(rad) - oldZ * sin(rad));
	z = (oldY * sin(rad) + oldZ * cos(rad));
}

////////////////////////////////////////////////////////////////////////////////

template <typename T> void TVector4<T>::RotateY(T rad)
{
	T	oldX = x;
	T	oldZ = z;

	x = (oldX * cos(rad) - oldZ * sin(rad));
	z = (oldX * sin(rad) + oldZ * cos(rad));
}

////////////////////////////////////////////////////////////////////////////////

template <typename T> void TVector4<T>::RotateZ(T rad)
{
	T	oldX = x;
	T	oldY = y;

	x = (oldX * cos(rad) - oldY * sin(rad));
	y = (oldX * sin(rad) + oldY * cos(rad));
}

///////////////////////////////////////////////////////////////////////////////

template <typename T> void TVector4<T>::Lerp( const TVector4<T> &from, const TVector4<T> &to, T par )
{
	if( par < T(0.0f) ) par = T(0.0f);
	if( par > T(1.0f) ) par = T(1.0f);
	T ipar = T(1.0f) - par;
	x = (from.x*ipar) + (to.x*par);
	y = (from.y*ipar) + (to.y*par);
	z = (from.z*ipar) + (to.z*par);
	w = (from.w*ipar) + (to.w*par);
}

////////////////////////////////////////////////////////////////////////////////

template <typename T> T TVector4<T>::CalcTriArea( const TVector4<T> &V0, const TVector4<T> &V1, const TVector4<T> &V2, const TVector4<T> & N )
{
	// select largest abs coordinate to ignore for projection
	T ax = abs( N.GetX() );
	T ay = abs( N.GetY() );
	T az = abs( N.GetZ() );

	int coord = (ax>ay) ? ((ax>az) ? 1 : 3) : ((ay>az) ? 2 : 3);

	// compute area of the 2D projection

	TVector4<T> Ary[3];
	Ary[0] = V0;
	Ary[1] = V1;
	Ary[2] = V2;
	T area(0.0f);

	for(	int i=1,
			j=2,
			k=0;
				i<=3;
					i++,
					j++,
					k++ )
	{
		int ii = i%3;
		int jj = j%3;
		int kk = k%3;

		switch (coord)
		{
			case 1:
				area += (Ary[ii].GetY() * (Ary[jj].GetZ() - Ary[kk].GetZ()));
				continue;
			case 2:
				area += (Ary[ii].GetX() * (Ary[jj].GetZ() - Ary[kk].GetZ()));
				continue;
			case 3:
				area += (Ary[ii].GetX() * (Ary[jj].GetY() - Ary[kk].GetY()));
				continue;
		}
	}

	T an = sqrt( (T) (ax*ax + ay*ay + az*az) );

	switch (coord)
	{
		case 1:
			assert( ax != T(0) );
			area *= (an / (T(2.0f)*ax));
			break;
		case 2:
			assert( ay != T(0) );
			area *= (an / (T(2.0f)*ay));
			break;
		case 3:
			assert( az != T(0) );
			area *= (an / (T(2.0f)*az));
			break;
	}

	return abs( area );
}

///////////////////////////////////////////////////////////////////////////////
}
///////////////////////////////////////////////////////////////////////////////
