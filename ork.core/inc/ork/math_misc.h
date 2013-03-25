///////////////////////////////////////////////////////////////////////////////
// MicroOrk (Orkid)
// Copyright 1996-2013, Michael T. Mayers
// Provided under the MIT License (see LICENSE.txt)
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <cmath>
#include <limits>
#include <stdint.h>
#include <functional>

typedef uint16_t u16;
typedef uint32_t u32;
typedef int16_t s16;
typedef int32_t s32;

#ifndef PI
static const float PI = 3.14159265;
#endif
static const float DTOR = PI/180.0f;

static constexpr float kPI = 3.14159265f;

float frand( float fscale, int irez );

struct SRect
{
	int miX, miY, miW, miH;

	SRect() : miX(0), miY(0), miW(0), miH(0) {}
};

namespace ork 
{

	template <typename T> T arccos( T inp );
	template <typename T> T sin( T inp );
	template <typename T> T cos( T inp );
	template <typename T> T tan( T inp );
	template <typename T> T abs( T inp );
	template <typename T> T sqrt( T inp );
	float float_epsilon();
	double double_epsilon();

}

struct math_table_1d
{
	//typedef float(^fn_t)(float fin);
	typedef std::function<float(float)> fn_t;


	math_table_1d();

	void fill_in(int isize, float rangeX, fn_t function);
	float operator()( float fin ) const;

	int miSize;
	float mfSizeInvRange;
	float mfRange;
	float* mpTable;
};

extern math_table_1d gsintab;
extern math_table_1d gcostab;
