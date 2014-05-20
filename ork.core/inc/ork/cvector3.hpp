///////////////////////////////////////////////////////////////////////////////
// MicroOrk (Orkid)
// Copyright 1996-2013, Michael T. Mayers
// Provided under the MIT License (see LICENSE.txt)
///////////////////////////////////////////////////////////////////////////////

#include "cmatrix4.h"
#include "cmatrix3.h"
#include "cvector4.h"

///////////////////////////////////////////////////////////////////////////////
namespace ork {
///////////////////////////////////////////////////////////////////////////////
bool UsingOpenGl();

///////////////////////////////////////////////////////////////////////////////

template <typename T> TVector3<T> TVector3<T>::Saturate( void ) const
{
	TVector3<T> rval = *this;
	rval.x = (rval.x>1.0f) ? 1.0f : (rval.x<0.0f) ? 0.0f : rval.x;
	rval.y = (rval.y>1.0f) ? 1.0f : (rval.y<0.0f) ? 0.0f : rval.y;
	rval.z = (rval.z>1.0f) ? 1.0f : (rval.z<0.0f) ? 0.0f : rval.z;
	return rval;
}


template <typename T> const TVector3<T> & TVector3<T>::Black( void )
{
	static const TVector3<T> Black( T(0.0f), T(0.0f), T(0.0f) );
	return Black;
}

///////////////////////////////////////////////////////////////////////////////

template <typename T> const TVector3<T> & TVector3<T>::DarkGrey( void )
{
	static const TVector3<T> DarkGrey( T(0.250f), T(0.250f), T(0.250f) );
	return DarkGrey;
}

///////////////////////////////////////////////////////////////////////////////

template <typename T> const TVector3<T> & TVector3<T>::MediumGrey( void )
{
	static const TVector3<T> MediumGrey( T(0.50f), T(0.50f), T(0.50f) );
	return MediumGrey;
}

///////////////////////////////////////////////////////////////////////////////

template <typename T> const TVector3<T> & TVector3<T>::LightGrey( void )
{
	static const TVector3<T> LightGrey(T(0.75f), T(0.75f), T(0.75f) );
	return LightGrey;
}

///////////////////////////////////////////////////////////////////////////////

template <typename T> const TVector3<T> & TVector3<T>::White( void )
{
	static const TVector3<T> White(  T(1.0f), T(1.0f), T(1.0f) );
	return White;
}

///////////////////////////////////////////////////////////////////////////////

template <typename T> const TVector3<T> & TVector3<T>::Red( void )
{
	static const TVector3<T> Red( T(1.0f), T(0.0f), T(0.0f) );
	return Red;
}

///////////////////////////////////////////////////////////////////////////////

template <typename T> const TVector3<T> & TVector3<T>::Green( void )
{
	static const TVector3<T> Green( T(0.0f), T(1.0f), T(0.0f) );
	return Green;
}

///////////////////////////////////////////////////////////////////////////////

template <typename T> const TVector3<T> & TVector3<T>::Blue( void )
{
	static const TVector3<T> Blue( T(0.0f), T(0.0f), T(1.0f) );
	return Blue;
}

///////////////////////////////////////////////////////////////////////////////

template <typename T> const TVector3<T> & TVector3<T>::Magenta( void )
{
	static const TVector3<T> Magenta( T(1.0f), T(0.0f), T(1.0f) );
	return Magenta;
}

///////////////////////////////////////////////////////////////////////////////

template <typename T> const TVector3<T> & TVector3<T>::Cyan( void )
{
	static const TVector3<T> Cyan( T(0.0f), T(1.0f), T(1.0f) );
	return Cyan;
}

///////////////////////////////////////////////////////////////////////////////

template <typename T> const TVector3<T> & TVector3<T>::Yellow( void )
{
	static const TVector3<T> Yellow( T(1.0f), T(1.0f), T(0.0f) );
	return Yellow;
}

///////////////////////////////////////////////////////////////////////////////

template <typename T> TVector3<T>::TVector3()
	: x(T(0.0f))
	, y(T(0.0f))
	, z(T(0.0f))
{
}

///////////////////////////////////////////////////////////////////////////////

template <typename T> TVector3<T>::TVector3( T _x, T _y, T _z)
	: x(_x)
	, y(_y)
	, z(_z)
{
}

template <typename T> TVector3<T>::TVector3( T scalar )
	: x(scalar)
	, y(scalar)
	, z(scalar)
{
}

////////////////////////////////////////////////////////////////////////////////

template <typename T> u32 TVector3<T>::GetVtxColorAsU32( void ) const 
{
	u32 r = u32(GetX()*T(255.0f));
	u32 g = u32(GetY()*T(255.0f));
	u32 b = u32(GetZ()*T(255.0f));
	u32 a = 255;
	
	if( ork::UsingOpenGl() )
		return u32( (a<<24)|(b<<16)|(g<<8)|r );
	else
		return u32( (a<<24)|(r<<16)|(g<<8)|b );
}

////////////////////////////////////////////////////////////////////////////////

template <typename T> u32 TVector3<T>::GetABGRU32( void ) const 
{
	u32 r = u32(GetX()*T(255.0f));
	u32 g = u32(GetY()*T(255.0f));
	u32 b = u32(GetZ()*T(255.0f));
	u32 a = 255;
	
	return u32( (a<<24)|(b<<16)|(g<<8)|r );
}

////////////////////////////////////////////////////////////////////////////////

template <typename T> u32 TVector3<T>::GetARGBU32( void ) const 
{
	u32 r = u32(GetX()*T(255.0f));
	u32 g = u32(GetY()*T(255.0f));
	u32 b = u32(GetZ()*T(255.0f));
	u32 a = 255;
	
	return u32( (a<<24)|(r<<16)|(g<<8)|b );
}

////////////////////////////////////////////////////////////////////////////////

template <typename T> u32 TVector3<T>::GetRGBAU32( void ) const 
{
	u32 r = u32(GetX()*T(255.0f));
	u32 g = u32(GetY()*T(255.0f));
	u32 b = u32(GetZ()*T(255.0f));
	u32 a = 255;

	u32 rval = 0;

#if defined(__sgi)
	rval = ( (b<<24)|(g<<16)|(r<<8)|a );
#else
	rval = ( (a<<24)|(r<<16)|(g<<8)|b );
#endif

	return rval;
}

///////////////////////////////////////////////////////////////////////////////

template <typename T> u32 TVector3<T>::GetBGRAU32( void ) const
{	u32 r = u32(GetX()*T(255.0f));
	u32 g = u32(GetY()*T(255.0f));
	u32 b = u32(GetZ()*T(255.0f));
	u32 a = 255;
	
	return u32( (b<<24)|(g<<16)|(r<<8)|a );
}

///////////////////////////////////////////////////////////////////////////////

template <typename T> u16 TVector3<T>::GetRGBU16() const 
{
	u32 r = u32(GetX() * T(31.0f));
	u32 g = u32(GetY() * T(31.0f));
	u32 b = u32(GetZ() * T(31.0f));

	u16 rval = u16((b<<10)|(g<<5)|r);

	return rval;
}

///////////////////////////////////////////////////////////////////////////////

template <typename T> void TVector3<T>::SetRGBAU32(u32 uval) 
{	
	u32 r = (uval>>24) & 0xff;
	u32 g = (uval>>16) & 0xff;
	u32 b = (uval>>8) & 0xff;

	static const T kfic(1.0f / 255.0f);

	SetX(kfic * T(int(r)));
	SetY(kfic * T(int(g)));
	SetZ(kfic * T(int(b)));
}

///////////////////////////////////////////////////////////////////////////////

template <typename T> void TVector3<T>::SetBGRAU32( u32 uval ) 
{	
	u32 b = (uval>>24) & 0xff;
	u32 g = (uval>>16) & 0xff;
	u32 r = (uval>>8) & 0xff;

	static const T kfic(1.0f / 255.0f);

	SetX(kfic * T(int(r)));
	SetY(kfic * T(int(g)));
	SetZ(kfic * T(int(b)));
}

///////////////////////////////////////////////////////////////////////////////

template <typename T> void TVector3<T>::SetARGBU32(u32 uval) 
{	
	u32 r = (uval>>16) & 0xff;
	u32 g = (uval>>8) & 0xff;
	u32 b = (uval) & 0xff;

	static const T kfic(1.0f / 255.0f);

	SetX(kfic * T(int(r)));
	SetY(kfic * T(int(g)));
	SetZ(kfic * T(int(b)));
}

///////////////////////////////////////////////////////////////////////////////

template <typename T> void TVector3<T>::SetABGRU32(u32 uval) 
{	
	u32 b = (uval>>16) & 0xff;
	u32 g = (uval>>8) & 0xff;
	u32 r = (uval) & 0xff;

	static const T kfic(1.0f / 255.0f);

	SetX(kfic * T(int(r)));
	SetY(kfic * T(int(g)));
	SetZ(kfic * T(int(b)));
}

///////////////////////////////////////////////////////////////////////////////

template <typename T> void TVector3<T>::SetHSV( T h, T s, T v )
{
}

template <typename T> TVector3<T> TVector3<T>::Reflect( const TVector3 &N ) const
{
	const TVector3<T>& I = *this;
	TVector3<T> R = I-(N*2.0f*N.Dot(I));
	return R;
}

///////////////////////////////////////////////////////////////////////////////

template <typename T> TVector3<T>::TVector3( const TVector3<T> &vec)
{
	x = vec.x;
	y = vec.y;
	z = vec.z;
}

///////////////////////////////////////////////////////////////////////////////

template <typename T> TVector3<T>::TVector3( const TVector4<T> &vec)
{
	x = vec.GetX();
	y = vec.GetY();
	z = vec.GetZ();
}

///////////////////////////////////////////////////////////////////////////////

template <typename T> TVector3<T>::TVector3( const TVector2<T> &vec)
{
	x = vec.GetX();
	y = vec.GetY();
	z = T(0);
}

///////////////////////////////////////////////////////////////////////////////

template <typename T> TVector3<T>::TVector3( const TVector2<T> &vec, T w)
{
    x = vec.GetX();
    y = vec.GetY();
    z = w;
}

////////////////////////////////////////////////////////////////////////////////

template <typename T> T TVector3<T>::Dot( const TVector3<T> &vec) const
{
#if defined WII
	return __fmadds(x,vec.x,__fmadds(y,vec.y,__fmadds(z,vec.z,0.0f)));
#else
	return ( (x * vec.x) + (y * vec.y) + (z * vec.z) );
#endif
}

////////////////////////////////////////////////////////////////////////////////

template <typename T> TVector3<T> TVector3<T>::Cross( const TVector3<T> &vec) const // c = this X vec
{
	T vx = ((y * vec.GetZ()) - (z * vec.GetY()));
	T vy = ((z * vec.GetX()) - (x * vec.GetZ()));
	T vz = ((x * vec.GetY()) - (y * vec.GetX()));

	return ( TVector3<T>( vx, vy, vz ) );
}

////////////////////////////////////////////////////////////////////////////////

template <typename T> void TVector3<T>::Normalize(void)
{
	T mag = Mag();
	if( mag > std::numeric_limits<T>::epsilon() )
	{
		T	distance = (T) 1.0f / mag ;

		x *= distance;
		y *= distance;
		z *= distance;
	}
}

////////////////////////////////////////////////////////////////////////////////

template <typename T> TVector3<T> TVector3<T>::Normal() const
{
	TVector3<T> vec(*this);
	vec.Normalize();

	return vec;
}

////////////////////////////////////////////////////////////////////////////////

template <typename T> T TVector3<T>::Mag(void) const
{
	return sqrt(x * x + y * y + z * z);
}

////////////////////////////////////////////////////////////////////////////////

template <typename T> T TVector3<T>::MagSquared(void) const
{
	T mag = (x * x + y * y + z * z);
	return mag;
}

////////////////////////////////////////////////////////////////////////////////

template <typename T> TVector4<T> TVector3<T>::Transform( const TMatrix4<T> &matrix ) const
{
	T	tx,ty,tz,tw;

	T *mp = (T *) matrix.elements;
	T _x = x;
	T _y = y;
	T _z = z;
	T _w = T(1.0f);

#if 0 //defined WII
	tx = __fmadds(x,vec.x,__fmadds(y,vec.y,__fmadds(z,vec.z,0.0f)));
#else
	tx = _x*mp[0] + _y*mp[4] + _z*mp[8] + _w*mp[12];
	ty = _x*mp[1] + _y*mp[5] + _z*mp[9] + _w*mp[13];
	tz = _x*mp[2] + _y*mp[6] + _z*mp[10] + _w*mp[14];
	tw = _x*mp[3] + _y*mp[7] + _z*mp[11] + _w*mp[15];
#endif

	return TVector4<T>( tx, ty, tz, tw );
}


////////////////////////////////////////////////////////////////////////////////

template <typename T> TVector3<T> TVector3<T>::Transform( const TMatrix3<T> &matrix ) const
{
	T	tx,ty,tz;

	T *mp = (T *) matrix.elements;
	T _x = x;
	T _y = y;
	T _z = z;

	tx = _x*mp[0] + _y*mp[3] + _z*mp[6];
	ty = _x*mp[1] + _y*mp[4] + _z*mp[7];
	tz = _x*mp[2] + _y*mp[5] + _z*mp[8];

	return TVector3<T>( tx, ty, tz );
}

////////////////////////////////////////////////////////////////////////////////

template <typename T> TVector3<T> TVector3<T>::Transform3x3( const TMatrix4<T> &matrix ) const
{
	T	tx,ty,tz;
	T *mp = (T *) matrix.elements;
	T _x = x;
	T _y = y;
	T _z = z;

	tx = _x*mp[0] + _y*mp[4] + _z*mp[8];
	ty = _x*mp[1] + _y*mp[5] + _z*mp[9];
	tz = _x*mp[2] + _y*mp[6] + _z*mp[10];

	return TVector3<T>( tx, ty, tz );
}

////////////////////////////////////////////////////////////////////////////////

template <typename T> void TVector3<T>::Serp( const TVector3<T> & PA, const TVector3<T> & PB, const TVector3<T> & PC, const TVector3<T> & PD, T Par )
{
	TVector3<T> PAB, PCD;
	PAB.Lerp( PA, PB, Par );
	PCD.Lerp( PC, PD, Par );
	Lerp( PAB, PCD, Par );
}

////////////////////////////////////////////////////////////////////////////////

template <typename T> void TVector3<T>::RotateX(T rad)
{
	T	oldY = y;
	T	oldZ = z;
	y = (oldY * cos(rad) - oldZ * sin(rad));
	z = (oldY * sin(rad) + oldZ * cos(rad));
}

////////////////////////////////////////////////////////////////////////////////

template <typename T> void TVector3<T>::RotateY(T rad)
{
	T	oldX = x;
	T	oldZ = z;

	x = (oldX * cos(rad) - oldZ * sin(rad));
	z = (oldX * sin(rad) + oldZ * cos(rad));
}

////////////////////////////////////////////////////////////////////////////////

template <typename T> void TVector3<T>::RotateZ(T rad)
{
	T	oldX = x;
	T	oldY = y;

	x = (oldX * cos(rad) - oldY * sin(rad));
	y = (oldX * sin(rad) + oldY * cos(rad));
}

///////////////////////////////////////////////////////////////////////////////

template <typename T> void TVector3<T>::Lerp( const TVector3<T> &from, const TVector3<T> &to, T par )
{
	if( par < T(0.0f) ) par = T(0.0f);
	if( par > T(1.0f) ) par = T(1.0f);
	T ipar = T(1.0f) - par;
	x = (from.x*ipar) + (to.x*par);
	y = (from.y*ipar) + (to.y*par);
	z = (from.z*ipar) + (to.z*par);
}

////////////////////////////////////////////////////////////////////////////////

template <typename T> T TVector3<T>::CalcTriArea( const TVector3<T> &V, const TVector3<T> & N ) 
{
    return T(0);
}

}
