///////////////////////////////////////////////////////////////////////////////
// MicroOrk (Orkid)
// Copyright 1996-2013, Michael T. Mayers
// Provided under the MIT License (see LICENSE.txt)
///////////////////////////////////////////////////////////////////////////////

#include <ork/types.h>

#include <math.h>
#include <ork/pool.h>
#include <ork/collision_test.h>
#include <ork/sphere.h>
//#include <ork/raytracer.h>
#include <ork/plane.hpp>
#include "render_graph.h"
#include "shader_funcs.h"
#include <ork/meshutil/meshutil.h>


namespace ork {

rend_srcmesh* LoadObj( const char* pfname, RenderContext& rdata );

///////////////////////////////////////////////////////////////////////////////

NoCLFromHostBuffer::NoCLFromHostBuffer()
{
}
NoCLFromHostBuffer::~NoCLFromHostBuffer()
{
}
NoCLToHostBuffer::NoCLToHostBuffer()
{
}
NoCLToHostBuffer::~NoCLToHostBuffer()
{
}
void NoCLToHostBuffer::resize(int isize)
{
	mpBuffer0 = new char[isize];
	mpBuffer1 = new char[isize];
}
void NoCLFromHostBuffer::resize(int isize)
{
	mpBuffer0 = new char[isize];
	mpBuffer1 = new char[isize];
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

SlicerModule::SlicerModule()
{

}

///////////////////////////////////////////////////////////////////////////////

SlicerModule::~SlicerModule()
{
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

render_graph::render_graph()
	: mTileRend(mRenderContext)
{
	//auto src_mesh = LoadObj("ork.swrast/test/dispm.obj",mRenderContext);
	//auto src_mesh = LoadObj("ork.swrast/test/tetra.obj",mRenderContext);
	auto src_mesh = LoadObj("ork.swrast/test/ico.obj",mRenderContext);
	

	if( src_mesh )
	{
		mRenderContext.mTarget = src_mesh->mTarget;
		mRenderContext.mEye = src_mesh->mEye;

		auto mpm = new MeshPrimModule( mRenderContext, src_mesh );
		mRenderContext.AddPrim(mpm);

		//////////////////////////////////////////////
		// assign test displacement shader
		//////////////////////////////////////////////

		int inumsub = src_mesh->miNumSubMesh;
		int itotaltris = 0;
		for( int is=0; is<inumsub; is++ )
		{
			const rend_srcsubmesh& Sub = src_mesh->mpSubMeshes[is];
			rend_shader* pshader = Sub.mpShader;

			pshader->mDisplacementShader = [=](const DisplacementShaderContext&dsc) ->RasterTri
			{
				float fi = dsc.mRenderContext.miFrame*0.1f;

				const RasterTri& inp = dsc.mRasterTri;
				RasterTri out = inp;

				auto l_disp = [&]( const CVector3& p, const CVector3& n ) -> CVector3
				{
					CVector3 dn = p.Normal();
					float fd = 	sinf((fi+dn.x)*PI2*9.0f)
							 *	cosf(dn.z*PI2*9.0f)
							 *	0.02f; 
					auto nfd = dn*fd;
					return nfd;
				};
				const RasterVtx& iv0 = inp.mVertex[0];
				const RasterVtx& iv1 = inp.mVertex[1];
				const RasterVtx& iv2 = inp.mVertex[2];
				RasterVtx& ov0 = out.mVertex[0];
				RasterVtx& ov1 = out.mVertex[1];
				RasterVtx& ov2 = out.mVertex[2];
				ov0.mObjPos += l_disp(iv0.mObjPos,iv0.mObjNrm);
				ov1.mObjPos += l_disp(iv1.mObjPos,iv1.mObjNrm);
				ov2.mObjPos += l_disp(iv2.mObjPos,iv2.mObjNrm);
				auto d01 = (ov1.mObjPos-ov0.mObjPos).Normal();
				auto d02 = (ov2.mObjPos-ov0.mObjPos).Normal();
				auto nn = d01.Cross(d02).Normal();
				auto bn = (nn*0.5f)+CVector3(0.5f,0.5f,0.5f);
				ov0.mRST = bn;
				ov1.mRST = bn;
				ov2.mRST = bn;
				return out;
			};
		}
	}
}

///////////////////////////////////////////////////////////////////////////////

void render_graph::Resize( int iw, int ih )
{
	mRenderContext.Resize(iw,ih);
}

///////////////////////////////////////////////////////////////////////////////

const u32* render_graph::GetPixels() const
{
	return mRenderContext.mPixelData;
}

///////////////////////////////////////////////////////////////////////////////

void render_graph::Compute(OpGroup& ogrp)
{
	mRenderContext.Update(ogrp);
	////////////////////////////////////////
	mTileRend.QueueTiles(ogrp);
	ogrp.drain();
	////////////////////////////////////////
	mRenderContext.miFrame++;
	////////////////////////////////////////
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

} // namespace ork