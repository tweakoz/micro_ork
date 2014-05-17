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
	auto src_mesh = LoadObj("ork.swrast/test/tetra.obj",mRenderContext);
	

	if( src_mesh )
	{
		mRenderContext.mTarget = src_mesh->mTarget;
		mRenderContext.mEye = src_mesh->mEye;

		auto mpm = new MeshPrimModule( mRenderContext, src_mesh );

		mRenderContext.AddPrim(mpm);
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