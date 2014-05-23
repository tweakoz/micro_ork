///////////////////////////////////////////////////////////////////////////////
// MicroOrk (Orkid)
// Copyright 1996-2013, Michael T. Mayers
// Provided under the MIT License (see LICENSE.txt)
///////////////////////////////////////////////////////////////////////////////

#pragma once

///////////////////////////////////////////////////////////////////////////////

#include <ork/types.h>
#include <ork/pool.h>
#include <ork/collision_test.h>
#include <ork/sphere.h>
#include <ork/plane.h>
#include <ork/line.h>

#include <ork/frustum.h>
#include <ork/opq.h>
#include <ork/radixsort.h>
#include <ork/math_misc.h>
#include <ork/ringbuffer.hpp>
#include <ork/box.h>
#include <ork/mutex.h>
#include <ork/atomic.h>

#include <unordered_map>
#include <vector>
#include <map>
#include <math.h>

#include "prim.h"
#include "fragment.h"

namespace ork {

///////////////////////////////////////////////////////////////////////////////

struct RasterTile;
struct SplitAndDice;

///////////////////////////////////////////////////////////////////////////////

class NoCLFromHostBuffer
{
public:
	void resize(int isize);
	char* GetBufferRegion() const { return GetBufferRegionPriv(); }
	NoCLFromHostBuffer();
	~NoCLFromHostBuffer();
	int GetSize() const { return miSize; }
protected:
	char* GetBufferRegionPriv() const { return (miWriteIndex&1)?mpBuffer1:mpBuffer0; }
	int		miSize;
	char*	mpBuffer0;
	char*	mpBuffer1;
	int		miWriteIndex;
};

///////////////////////////////////////////////////////////////////////////////

class NoCLToHostBuffer
{
public:
	void resize(int isize);
	const char* GetBufferRegion() const { return GetBufferRegionPriv(); }
	NoCLToHostBuffer();
	~NoCLToHostBuffer();
	int GetSize() const { return miSize; }
protected:
	char* GetBufferRegionPriv() const { return (miReadIndex&1)?mpBuffer1:mpBuffer0; }
	int		miSize;
	uint32_t		mBufferflags;
	char*	mpBuffer0;
	char*	mpBuffer1;
	int		miReadIndex;
};

///////////////////////////////////////////////////////////////////////////////

struct AABuffer
{
	uint32_t*					mColorBuffer;
	float*						mDepthBuffer;
	int 						mAADim;
	int							miAATileDim;
	int							mTileDim;
	int							mAAFactor;

	rend_fraglist*				mFragmentBuffer;
	FragmentPool				mFragmentPool;
	FragmentCompositorABuffer	mCompositorAB;
	FragmentCompositorZBuffer	mCompositorZB;
	NoCLFromHostBuffer*			mTriangleClBuffer;
	NoCLFromHostBuffer*			mFragInpClBuffer;
	NoCLToHostBuffer*			mFragOutClBuffer;
	const RasterTile*			mRasterTile;
	CVector3					mClearColor;

	PreShadedFragmentPool		mPreFrags;
	static const int kfragallocsize = 1<<20;
	rend_fragment*				mpFragments[kfragallocsize];

	AABuffer(int tile_dim,int aa_dim);
	~AABuffer();

	void Clear(RenderContext& rctx);
	void Resolve(RenderContext& rctx, rend_fraglist_visitor& fragger);
	int CalcAAPixelAddress( int ix, int iy ) const;
};
///////////////////////////////////////////////////////////////////////////////

struct RasterTile
{
	int 					mAddress;
	int						miScreenXBase;
	int 					miScreenYBase;
	int						miWidth, miHeight;

	float					mAAScreenXBase;
	float					mAAScreenYBase;
	float					mAAWidth, mAAHeight;
	ork::Frustum			mFrustum;
	//mutable float			MinZ;
};

///////////////////////////////////////////////////////////////////////////////

struct SplitAndDice //: public ork::threadpool::task
{
	SplitAndDice(RenderContext& rdata);

private:

	////////////////////////////////////////////////////////////
	RenderContext& 						mRenderContext;
	void*								mSourceHash;
};

///////////////////////////////////////////////////////////////////////////////

class RenderContext
{
public:

	typedef std::set<IGeoPrim*> prim_set_t;

	RenderContext();

	static const int 		kTileDim=32;
	static const int		kAABUFTILES = 128;
	int						mAADim;
	int 					mAATileSize;
	int						miNumTilesW;
	int						miNumTilesH;
	int						miImageWidth;
	int						miImageHeight;
	int						miFrame;
	std::vector<RasterTile>	mRasterTiles;
	MpMcRingBuf<AABuffer*,kAABUFTILES>	mAABufTiles;

	int GetTileOvSize() const;

	ork::CVector3			mEye;
	ork::CVector3			mTarget;
	ork::CMatrix4			mMatrixP;
	ork::CMatrix4			mMatrixV;
	ork::CMatrix4			mMatrixM;
	ork::CMatrix4			mMatrixMV;
	ork::CMatrix4			mMatrixMVP;
	ork::Frustum			mFrustum;
	uint32_t*				mPixelData;
	
	SplitAndDice			mSplitAndDice;

	//LockedResource<prim_set_t> mPrimSet;
	prim_set_t mPrimSet;

	int GetTileX( float fx ) const;
	int GetTileY( float fy ) const;
	//int GetBucketIndex( int ix, int iy ) const;

	void Resize( int iw, int ih );
	void Update(OpGroup& ogrp);

	void operator=( const RenderContext& oth );

	const RasterTile& GetTile( int itx, int ity ) const;
	RasterTile& GetTile( int itx, int ity );
	int CalcTileAddress( int itx, int ity ) const;
	int CalcPixelAddress( int ix, int iy ) const;

	AABuffer* BindAATile(int itx,int ity);
	void UnBindAABTile(AABuffer*tile);
	void RenderToTile(AABuffer*aab);
	void AddPrim(IGeoPrim*);
};

///////////////////////////////////////////////////////////////////////////////

struct TileRenderer
{
	TileRenderer(RenderContext& rdata);
	void QueueTiles(OpGroup& ogrp);
	void ProcessTile( int itx, int ity ); 
	RenderContext& 	mRenderContext;
};

///////////////////////////////////////////////////////////////////////////////

struct SlicerData
{};

///////////////////////////////////////////////////////////////////////////////

class SlicerModule // : public ork::threadpool::task
{
public:
	SlicerModule();
	~SlicerModule();
private:
	////////////////////////////////////////////////////////////
};

///////////////////////////////////////////////////////////////////////////////

class render_graph 
{
public:
	render_graph();
	void Compute(OpGroup& ogrp);

	void Resize( int iw, int ih );
	const uint32_t* GetPixels() const;
	
	static const int ktrinringsize = 65536;
	MpMcRingBuf<Stage1Tri*,ktrinringsize> mTriRing;

private:
	RenderContext			mRenderContext;
	//MeshPrimModule			mMeshMod;
	TileRenderer			mTileRend;

};
///////////////////////////////////////////////////////////////////////////////

} // namespace ork