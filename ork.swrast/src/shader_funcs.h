///////////////////////////////////////////////////////////////////////////////
// MicroOrk (Orkid)
// Copyright 1996-2013, Michael T. Mayers
// Provided under the MIT License (see LICENSE.txt)
///////////////////////////////////////////////////////////////////////////////

#pragma once
#include <ork/perlin_noise.h>

namespace ork {


#if 0

inline ork::CVector4 SphMap( const ork::CVector3& N, const ork::CVector3& EyeToPointDir, const rend_texture2D& tex ) 
{
	ork::CVector3 ref = EyeToPointDir-N*(N.Dot(EyeToPointDir)*2.0f);
	float p = ::sqrtf( ref.GetX()*ref.GetX()+ref.GetY()*ref.GetY()+::powf(ref.GetZ()+1.0f,2.0f) );
	float reflectS = ref.GetX()/(2.0f*p)+0.5f;
	float reflectT = ref.GetY()/(2.0f*p)+0.5f;
	return tex.sample_point( reflectS, reflectT, true, true );
}

inline ork::CVector4 OctaveTex( int inumoctaves, float fu, float fv, float texscale, float texamp, float texscalemodifier, float texampmodifier, const rend_texture2D& tex )
{
	ork::CVector4 tex0;
	for( int i=0; i<inumoctaves; i++ )
	{
		tex0 += tex.sample_point( std::abs(fu*texscale), std::abs(fv*texscale), true, true )*texamp;
		texscale *= texscalemodifier;
		texamp *= texampmodifier;
	}
	return tex0;
}


///////////////////////////////////////////////////////////////////////////////
struct test_volume_shader : public rend_volume_shader
{
	ork::CVector4 ShadeVolume( const ork::CVector3& entrywpos, const ork::CVector3& exitwpos ) const; // virtual 
};
#endif
///////////////////////////////////////////////////////////////////////////////
struct Shader1 : public rend_shader
{
	ork::OldPerlin2D				mPerlin2D;

	eType GetType() const { return EShaderTypeSurface; } // virtual

	void ShadeBlock( AABuffer* aabuf, int ifragbase, int icount ) const; // virtual

	Shader1();
};

} // namespace ork
