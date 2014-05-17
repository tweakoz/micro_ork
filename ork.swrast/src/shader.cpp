///////////////////////////////////////////////////////////////////////////////
// MicroOrk (Orkid)
// Copyright 1996-2013, Michael T. Mayers
// Provided under the MIT License (see LICENSE.txt)
///////////////////////////////////////////////////////////////////////////////

#include <ork/types.h>

#include <math.h>
#include <ork/pool.h>
#include <ork/collision_test.h>
//#include <ork/raytracer.h>
#include <ork/sphere.h>
#include <ork/plane.hpp>
#include "render_graph.h"
#include "shader_funcs.h"
//#include <CL/cl.h>
//#include <CL/clext.h>

namespace ork {

///////////////////////////////////////////////////////////////////////////////

Shader1::Shader1()
{
}

///////////////////////////////////////////////////////////////////////////////

void Shader1::ShadeBlock( AABuffer* aabuf, int ifragbase, int icount, int inumtri ) const
{	
	PreShadedFragmentPool& PFRAGS = aabuf->mPreFrags;
	if( 0 == icount ) return;
	//////////////////////////////////////////////////////
	int ifraginpsize = icount*4*sizeof(float);
	int ifragoutsize = icount*5*sizeof(float);
	//////////////////////////////////////////////////////
	float* pprefragbuffer = (float*) aabuf->mFragInpClBuffer->GetBufferRegion();
	int ifragidx = ifragbase;
	for( int i=0; i<icount; i++ )
	{	const PreShadedFragment& pfrag = PFRAGS.mPreFrags[ifragidx++];
		rend_fragment* frag = aabuf->mpFragments[i];
//		const ork::CVector3 nrm = (frag->mWldSpaceNrm*0.5f)+ork::CVector3(0.5f,0.5f,0.5f);
		frag->mRGBA.SetXYZ( pfrag.mfR, pfrag.mfS, pfrag.mfT );
		//frag->mRGBA.SetXYZ( nrm.GetX(), nrm.GetY(), nrm.GetZ() );
		frag->mRGBA.SetW( 1.0f );
		frag->mZ = pfrag.mfZ;
	}
	//////////////////////////////////////////////////////
}

///////////////////////////////////////////////////////////////////////////////

} // namespace ork {