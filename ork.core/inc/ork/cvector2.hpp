///////////////////////////////////////////////////////////////////////////////
// MicroOrk (Orkid)
// Copyright 1996-2013, Michael T. Mayers
// Provided under the MIT License (see LICENSE.txt)
///////////////////////////////////////////////////////////////////////////////

#pragma once
#include "cvector3.h"

///////////////////////////////////////////////////////////////////////////////

template <typename T> ork::TVector2<T>::TVector2()
	: x(T(0))
	, y(T(0))
{
}

///////////////////////////////////////////////////////////////////////////////

template <typename T> ork::TVector2<T>::TVector2( T _x, T _y)
	: x(_x)
	, y(_y)
{
}

////////////////////////////////////////////////////////////////////////////////

template <typename T> ork::TVector2<T>::TVector2( const TVector2<T>& vec)
	: x( vec.GetX() )
	, y( vec.GetY() )
{
}

///////////////////////////////////////////////////////////////////////////////

template <typename T> ork::TVector2<T>::TVector2( const TVector3<T>& vec)
	: x( vec.GetX() )
	, y( vec.GetY() )
{
}

////////////////////////////////////////////////////////////////////////////////

template <typename T> T ork::TVector2<T>::Dot( const TVector2<T>& vec) const
{
	return ( (x * vec.x) + (y * vec.y) );
}

////////////////////////////////////////////////////////////////////////////////

template <typename T> T ork::TVector2<T>::PerpDot( const TVector2<T>& oth) const
{
	return (x * oth.y)-(y * oth.x);
}

////////////////////////////////////////////////////////////////////////////////

template <typename T> void ork::TVector2<T>::Normalize(void)
{
	T	distance = T(1) / Mag() ;

	x *= distance;
	y *= distance;
}

////////////////////////////////////////////////////////////////////////////////

template <typename T> ork::TVector2<T> ork::TVector2<T>::Normal() const
{
	T fmag = Mag();
	fmag = (fmag==T(0)) ? std::numeric_limits<T>::epsilon() : fmag;
	T	s = T(1) / fmag;
	return TVector2<T>(x * s, y * s);
}

////////////////////////////////////////////////////////////////////////////////

template <typename T> T ork::TVector2<T>::Mag(void) const
{
	return sqrt<T>(x * x + y * y);
}

////////////////////////////////////////////////////////////////////////////////

template <typename T> T ork::TVector2<T>::MagSquared(void) const
{
	T mag = (x * x + y * y);
	return mag;
}

////////////////////////////////////////////////////////////////////////////////

template <typename T> void ork::TVector2<T>::Serp( const TVector2<T> & PA, const TVector2<T> & PB, const TVector2<T> & PC, const TVector2<T> & PD, T Par )
{
	TVector2<T> PAB, PCD;
	PAB.Lerp( PA, PB, Par );
	PCD.Lerp( PC, PD, Par );
	Lerp( PAB, PCD, Par );
}

////////////////////////////////////////////////////////////////////////////////

template <typename T> void ork::TVector2<T>::Rotate(T rad)
{
	T	oldX = x;
	T	oldY = y;

	x = (oldX * sin<T>(rad) - oldY * cos<T>(rad));
	y = (oldX * cos<T>(rad) + oldY * sin<T>(rad));
}

///////////////////////////////////////////////////////////////////////////////

template <typename T> void ork::TVector2<T>::Lerp( const TVector2<T> &from, const TVector2<T> &to, T par )
{
	if( par < T(0) ) par = T(0);
	if( par > T(1) ) par = T(1);
	T ipar = T(1) - par;
	x = (from.x*ipar) + (to.x*par);
	y = (from.y*ipar) + (to.y*par);
}

///////////////////////////////////////////////////////////////////////////////
