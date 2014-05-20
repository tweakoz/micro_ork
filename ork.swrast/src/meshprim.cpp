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
#include <ork/plane.hpp>
#include "render_graph.h"

namespace ork {

static const float one_third = 0.333333333f;

///////////////////////////////////////////////////////////////////////////////

MeshPrimModule::MeshPrimModule(RenderContext& rdata,rend_srcmesh*pmsh)
	: mpSrcMesh(pmsh)
	, mRenderContext(rdata)
{
	int inumsub = mpSrcMesh->miNumSubMesh;
	int itotaltris = 0;
	for( int is=0; is<inumsub; is++ )
	{
		const rend_srcsubmesh& Sub = mpSrcMesh->mpSubMeshes[is];
		rend_shader* pshader = Sub.mpShader;
		int inumtri = (Sub.miNumTriangles);
		for( int it=0; it<inumtri; it++ )
		{
			const rend_srctri& SrcTri = Sub.mpTriangles[it];
			const auto& src_vtx0 = SrcTri.mpVertices[0];
			const auto& src_vtx1 = SrcTri.mpVertices[1];
			const auto& src_vtx2 = SrcTri.mpVertices[2];
			////////////////////////////////////////////////
			auto rtri = new Stage1Tri(*this);
			auto& dst_vtx0 = rtri->mBaseTriangle.mVertex[0];
			auto& dst_vtx1 = rtri->mBaseTriangle.mVertex[1];
			auto& dst_vtx2 = rtri->mBaseTriangle.mVertex[2];
			dst_vtx0.mObjPos = src_vtx0.mPos;
			dst_vtx1.mObjPos = src_vtx1.mPos;
			dst_vtx2.mObjPos = src_vtx2.mPos;
			dst_vtx0.mObjNrm = src_vtx0.mVertexNormal;
			dst_vtx1.mObjNrm = src_vtx1.mVertexNormal;
			dst_vtx2.mObjNrm = src_vtx2.mVertexNormal;
			dst_vtx0.mRST = CVector3(1.0f,0.0f,0.0f);
			dst_vtx1.mRST = CVector3(0.0f,1.0f,0.0f);
			dst_vtx2.mRST = CVector3(0.0f,0.0f,1.0f);
			rtri->mfArea = SrcTri.mSurfaceArea;
			rtri->mpShader = pshader;
			mStage1Triangles.push_back(rtri);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////

void MeshPrimModule::XfChunk(size_t start, size_t end)
{
	const ork::CMatrix4& mtxM = mRenderContext.mMatrixM;
	const ork::CMatrix4& mtxMV = mRenderContext.mMatrixMV;
	const ork::CMatrix4& mtxMVP = mRenderContext.mMatrixMVP;

	float faadim(mRenderContext.mAADim);

	float fiW = float(mRenderContext.miImageWidth)*faadim;
	float fiH = float(mRenderContext.miImageHeight)*faadim;


	for( size_t i=start; i<end; i++ )
	{
		auto rtri = this->mStage1Triangles[i];

		const auto& btri = rtri->mBaseTriangle;

		rtri->mIsVisible = false;

		//////////////////////////////////////

		// TODO:
		//  displacement bounds checking here!
		// (make sure to pass prims that might be visible if displaced..)

		//////////////////////////////////////
		// trivial reject
		//////////////////////////////////////

		ScrSpcTri sst(btri,mtxMVP,fiW,fiH);
		const ork::CVector4& hd0 = sst.mScrPosA;
		const ork::CVector4& hd1 = sst.mScrPosB;
		const ork::CVector4& hd2 = sst.mScrPosC;
		ork::CVector3 d0 = (hd1.GetXYZ()-hd0.GetXYZ());
		ork::CVector3 d1 = (hd2.GetXYZ()-hd1.GetXYZ());
		ork::CVector3 dX = d0.Cross(d1);

		if( true == (dX.z<=0.0f) ) continue; // is it frontfacing ?

		int inuminside = 0;

		inuminside += (		((hd0.x>=0.0f) || (hd0.x<fiW))
						&&	((hd0.y>=0.0f) || (hd0.y<fiH))
						&&	((hd0.z>=0.0f) || (hd0.z<=1.0f)) );

		inuminside += (		((hd1.x>=0.0f) || (hd1.x<fiW))
						&&	((hd1.y>=0.0f) || (hd1.y<fiH))
						&&	((hd1.z>=0.0f) || (hd1.z<=1.0f)) );

		inuminside += (		((hd2.x>=0.0f) || (hd2.x<fiW))
						&&	((hd2.y>=0.0f) || (hd2.y<fiH))
						&&	((hd2.z>=0.0f) || (hd2.z<=1.0f)) );

		if( 0 == inuminside ) // is it in the screen bounds ?
		{
			// todo : this is probably better done by the subdivider
			continue;
		}

		rtri->mIsVisible = true;

		//////////////////////////////////////
		// the triangle passed the backface cull and trivial reject, queue it
		//////////////////////////////////////

		rtri->mMinX = sst.mMinX;
		rtri->mMinY = sst.mMinY;
		rtri->mMaxX = sst.mMaxX;
		rtri->mMaxY = sst.mMaxY;

		int iminbx = mRenderContext.GetTileX(sst.mMinX)-1;
		int imaxbx = mRenderContext.GetTileX(sst.mMaxX)+1;
		int iminby = mRenderContext.GetTileY(sst.mMinY)-1;
		int imaxby = mRenderContext.GetTileY(sst.mMaxY)+1;

		if( iminbx<0 ) iminbx = 0;
		if( iminby<0 ) iminby = 0;
		if( imaxbx>=mRenderContext.miNumTilesW ) imaxbx = mRenderContext.miNumTilesW-1;
		if( imaxby>=mRenderContext.miNumTilesH ) imaxby = mRenderContext.miNumTilesH-1;

		///////////////////////////////////////////////////
		// bind to intersecting rastertiles
		///////////////////////////////////////////////////

		for( int ity=iminby; ity<=imaxby; ity++ )
		{
			for( int itx=iminbx; itx<=imaxbx; itx++ )
			{
				int idx = mRenderContext.CalcTileAddress(itx,ity);
				mRtMap[idx].fetch_and_increment();
			}
		}

		///////////////////////////////////////////////////

	}
}

///////////////////////////////////////////////////////////////////////////////

void MeshPrimModule::TransformAndCull(OpGroup& ogrp)
{
	/////////////////////////////////////////////////////////////////////////
	// clear raster tile binding map
	/////////////////////////////////////////////////////////////////////////

	for( int i=0; i<krtmap_size; i++ )
		mRtMap[i] = 0;

	/////////////////////////////////////////////////////////////////////////
	//	submit chunks to opq
	/////////////////////////////////////////////////////////////////////////

	const size_t kchunksize = 1024;
	size_t num_tris = mStage1Triangles.size();
	size_t tri_base = 0;

	while( num_tris )
	{
		size_t this_chunk_size  = (kchunksize<num_tris)
								?  kchunksize
								:  num_tris;

		auto l=[=](){
			this->XfChunk(tri_base,tri_base+this_chunk_size);
		};
		
		ogrp.push(Op(l,"xform_mesh"));

		num_tris -= this_chunk_size;
		tri_base += this_chunk_size;
	}

}

///////////////////////////////////////////////////////////////////////////////

Stage1Tri::Stage1Tri(MeshPrimModule& tac )
	: mTAC(tac)
{
	mRefCount = 0;
}

///////////////////////////////////////////////////////////////////////////////

struct irect
{
	irect( float fl, float fr, float ft, float fb ) : l(fl), r(fr), t(ft), b(fb) {}
	float l,r,t,b;

};

///////////////////////////////////////////////////////////////////////////////

static bool isIntersecting(const irect& ir, float xa, float ya, float xb, float yb)
{ 
	// Calculate m and c for the equation for the line (y = mx+c)
	float m = (yb-ya) / (xb-xa); 
	float c = ya - (m * xa);

	if ((ya - ir.t) * (yb - ir.t) < 0.0f)
	{
		float s = (ir.t - c) / m;
		if ( s > ir.l && s < ir.r) return true;    
	}

	if ((ya - ir.b) * (yb - ir.b) < 0.0f)
	{
		float s = (ir.b - c) / m;
		if ( s > ir.l && s < ir.r) return true;    
	}

	if ((xa - ir.l) * (xb - ir.l) < 0.0f)
	{
		float s = m * ir.l + c;
		if ( s > ir.t && s < ir.b) return true;
	}

	if ((xa - ir.r) * (xb - ir.r) < 0.0f)
	{
		float s = m * ir.r + c;
		if ( s > ir.t && s < ir.b) return true;
	}

	return false;

}

///////////////////////////////////////////////////////////////////////////////

static bool isInsideTriangle( const irect& ir, const ork::CVector4& v0, const ork::CVector4& v1, const ork::CVector4& v2)
{ 
	float l = ir.l;
	float r = ir.r;
	float t = ir.t;
	float b = ir.b;
	float x0=v0.x;
	float y0=v0.y;
	float x1=v1.x;
	float y1=v1.y;
	float x2=v2.x;
	float y2=v2.y;

	// Calculate m and c for the equation for the line (y = mx+c)
	float m0 = (y2-y1) / (x2-x1); 
	float c0 = y2 -(m0 * x2);
	float m1 = (y2-y0) / (x2-x0); 
	float c1 = y0 -(m1 * x0);
	float m2 = (y1-y0) / (x1-x0); 
	float c2 = y0 -(m2 * x0);
	int t0 = int(x0 * m0 + c0 > y0);
	int t1 = int(x1 * m1 + c1 > y1);
	int t2 = int(x2 * m2 + c2 > y2);
	float lm0 = l * m0 + c0;
	float lm1 = l * m1 + c1;
	float lm2 = l * m2 + c2;
	float rm0 = r * m0 + c0;
	float rm1 = r * m1 + c1;
	float rm2 = r * m2 + c2;

	if ( !(t0 ^ int( lm0 > t)) && !(t1 ^ int(lm1 > t)) && !(t2 ^ int(lm2 > t))) return true;
	if (!(t0 ^ int( lm0 > b)) && !(t1 ^ int(lm1 > b)) && !(t2 ^ int(lm2 > b))) return true;
	if ( !(t0 ^ int( rm0 > t)) && !(t1 ^ int(rm1 > t)) && !(t2 ^ int(rm2 > t))) return true;
	if (!(t0 ^ int( rm0 > b)) && !(t1 ^ int(rm1 > b)) && !(t2 ^ int(rm2 > b))) return true;

	return false;

}

///////////////////////////////////////////////////////////////////////////////

static bool triangleTest( const ScrSpcTri& sstri, const irect& ir )
{
	float x0 = sstri.mScrPosA.x; 
	float y0 = sstri.mScrPosA.y; 
	float x1 = sstri.mScrPosB.x; 
	float y1 = sstri.mScrPosB.y; 
	float x2 = sstri.mScrPosC.x; 
	float y2 = sstri.mScrPosC.y; 
	float l = ir.l;
	float r = ir.r;
	float t = ir.t;
	float b = ir.b;

	if (x0 > l) if(x0 < r) if(y0 > t) if(y0 < b) return true;
	if (x1 > l) if( x1 < r ) if( y1 > t ) if( y1 < b) return true;
	if (x2 > l ) if( x2 < r ) if( y2 > t ) if( y2 < b) return true;

	if (isInsideTriangle(ir,sstri.mScrPosA,sstri.mScrPosB,sstri.mScrPosC)) return true;

	return(		isIntersecting(ir,x0, y0, x1, y1) 
			|| 	isIntersecting(ir,x1, y1, x2, y2)
			||	isIntersecting(ir,x0, y0, x2, y2)
	);
}

///////////////////////////////////////////////////////////////////////////////

bool boxisect(	const ScrSpcTri& sstri,
				//float fax0, float fay0, float fax1, float fay1,
				float fbx0, float fby0, float fbx1, float fby1 )
{	if (sstri.mMaxY < fby0) return false;
	if (sstri.mMinY > fby1) return false;
	if (sstri.mMaxX < fbx0) return false;
	if (sstri.mMinX > fbx1) return false;
	//printf( "boxisect minxy<%f %f> maxxy<%f %f>\n", rtri->mMinX, rtri->mMinY, rtri->mMaxX, rtri->mMaxY );
	return true;
}

///////////////////////////////////////////////////////////////////////////////

bool MeshPrimModule::IsBoundToRasterTile(const RasterTile* prt ) const
{
	int addr = prt->mAddress;
	assert(addr<krtmap_size);
    return mRtMap[addr].load()>0;
}

void MeshPrimModule::RenderToTile(AABuffer* aabuf)
{
    const RasterTile* tile = aabuf->mRasterTile;

    /////////////////////////////////////////////////
    // was not marked as bound to this rastertile 
    //  in the xfrom step => no intersection with this tile possible
    /////////////////////////////////////////////////

	if( false == IsBoundToRasterTile(tile) )
		return;

    /////////////////////////////////////////////////

    rendtri_context ctx( *aabuf );

	const rend_shader* pshader = nullptr;

	for( auto rtri : mStage1Triangles )
	{
		if( rtri->mIsVisible )
		{
			pshader = rtri->mpShader;
			ctx.mpShader = pshader;
			SubdivideTri( ctx, rtri->mBaseTriangle );
			////////////////////////
			// todo - multishader/mesh support
		}
	}

	/////////////////////////////////////////////////////////
	// shade fragments in per material blocks
	//  the block method will allow shading by GP/GPU
	/////////////////////////////////////////////////////////
    
    if( pshader )
    {
    	const PreShadedFragmentPool& pfgs = aabuf->mPreFrags;
	    auto& fpool = aabuf->mFragmentPool;
	    rend_fraglist* pFBUFFER = aabuf->mFragmentBuffer;

		const int knumfrag = pfgs.miNumPreFrags;
		int i=0;
		int ipfragbase = 0;
		int ifragmentindex = 0;

		aabuf->mPreFrags.Reset();
		while( i<knumfrag )
		{	
			int iremaining = knumfrag-i;
			int icount = (iremaining>=AABuffer::kfragallocsize) 
							? AABuffer::kfragallocsize
							: iremaining;

			OrkAssert( ifragmentindex+icount < AABuffer::kfragallocsize );
			bool bOK = fpool.AllocFragments( aabuf->mpFragments+ifragmentindex, icount );
			///////////////////////////////////////////////
			// CL shadeblock will write a CL fragment buffer, call a shader kernel, and read the resulting colorz fragments
			
			pshader->ShadeBlock( aabuf, ipfragbase, icount );
			///////////
			for( int j=0; j<icount; j++ )
			{	const PreShadedFragment& pfrag = pfgs.mPreFrags[ipfragbase++];
				rend_fragment* frag = aabuf->mpFragments[ifragmentindex++];
				pFBUFFER[pfrag.miPixIdxAA].AddFragment( frag );
			}
			i += icount;
			ipfragbase += icount;
		}
	}
}

ScrSpcTri::ScrSpcTri( const RasterTri& rastri, const CMatrix4& mtxmvp, float fsw, float fsh )
	: mRasterTri(rastri)
	, mScrPosA(rastri.mVertex[0].mObjPos.Transform(mtxmvp))
	, mScrPosB(rastri.mVertex[1].mObjPos.Transform(mtxmvp))
	, mScrPosC(rastri.mVertex[2].mObjPos.Transform(mtxmvp))
{
	mScrPosA.PerspectiveDivide();
	mScrPosB.PerspectiveDivide();
	mScrPosC.PerspectiveDivide();

	CVector3 bias(0.5f,0.5f,0.5f);
	CVector3 scali(0.5f,0.5f,0.5f);
	CVector3 scalo(fsw,fsh,1.0f);

	mScrPosA = (bias+mScrPosA*scali)*scalo;
	mScrPosB = (bias+mScrPosB*scali)*scalo;
	mScrPosC = (bias+mScrPosC*scali)*scalo;

	mScrCenter = (mScrPosA+mScrPosB+mScrPosC)*one_third;

	auto v0 = CVector3(mScrPosA.GetXY(),0.0f);
	auto v1 = CVector3(mScrPosB.GetXY(),0.0f);
	auto v2 = CVector3(mScrPosC.GetXY(),0.0f);
	mScreenArea2D = (v1-v0).Cross(v2-v0).Mag() * 0.5f;

	mMinX = std::min(mScrPosA.x,std::min(mScrPosB.x,mScrPosC.x));
    mMinY = std::min(mScrPosA.y,std::min(mScrPosB.y,mScrPosC.y));
    mMaxX = std::max(mScrPosA.x,std::max(mScrPosB.x,mScrPosC.x));
    mMaxY = std::max(mScrPosA.y,std::max(mScrPosB.y,mScrPosC.y));

}

const ork::CVector4& ScrSpcTri::GetScrPos(int idx) const
{
	switch( idx )
	{
		case 0:
			return mScrPosA;
		case 1:
			return mScrPosB;
		case 2:
		default:
			return mScrPosC;
	}
}

///////////////////////////////////////////////////////////////////////////////

void MeshPrimModule::SubdivideTri( const rendtri_context& ctx, const RasterTri& rastri )
{	
	assert (ctx.mpShader != nullptr);

	//printf( "fscrarea<%f>\n", fscrarea );
	const ork::CMatrix4& mtxMVP = mRenderContext.mMatrixMVP;

	AABuffer& aabuf = ctx.mAABuffer;
	auto tile = aabuf.mRasterTile;
	int idim1 = aabuf.miAATileDim;
	int imod = aabuf.miAATileDim;
	float faadim(aabuf.mAADim);
	float fiW = float(mRenderContext.miImageWidth)*faadim;
	float fiH = float(mRenderContext.miImageHeight)*faadim;

	const float disp_bounds = 48;
	const float disp_bounds2 = 2*disp_bounds;

	float fbucketDX0 = float(tile->miScreenXBase-disp_bounds)*float(aabuf.mAADim);
	float fbucketDY0 = float(tile->miScreenYBase-disp_bounds)*float(aabuf.mAADim);
	float fbucketDX1 = fbucketDX0+float(tile->miWidth+disp_bounds2)*float(aabuf.mAADim);
	float fbucketDY1 = fbucketDY0+float(tile->miHeight+disp_bounds2)*float(aabuf.mAADim);

	//printf( "st2\n");

	//////////////////////////////////////////
	
	ScrSpcTri sst(rastri,mtxMVP,fiW,fiH);

	if( false==boxisect( sst, fbucketDX0, fbucketDY0, fbucketDX1, fbucketDY1 ) )
		return;
	bool bsectT = triangleTest(sst,irect(fbucketDX0,fbucketDX1,fbucketDY0,fbucketDY1));
	if( false  == bsectT )
		return;

	//////////////////////////////////////////
	// if screen area is small enough - actually rasterize it
	//////////////////////////////////////////

	bool do_subdivide = false;

	const bool use_scanline_renderer = true;

	float scrthresh = use_scanline_renderer ? 8.0f : 0.25f;

	if( sst.mScreenArea2D < scrthresh )  // screen area < pixel thresh ?
	{
    	float fi = float(mRenderContext.miFrame)/10.0f;

		float fbucketX0 = float(tile->miScreenXBase)*float(aabuf.mAADim);
		float fbucketY0 = float(tile->miScreenYBase)*float(aabuf.mAADim);
		float fbucketX1 = fbucketX0+float(tile->miWidth)*float(aabuf.mAADim);
		float fbucketY1 = fbucketY0+float(tile->miHeight)*float(aabuf.mAADim);

		RasterTri rastri1 = rastri;

		if( ctx.mpShader->mDisplacementShader )
		{	
			DisplacementShaderContext dsc(mRenderContext,rastri);
			rastri1 = ctx.mpShader->mDisplacementShader(dsc);
		}
		ScrSpcTri sst2(rastri1,mtxMVP,fiW,fiH);

		if( false==boxisect( sst2, fbucketX0, fbucketY0, fbucketX1, fbucketY1 ) )
			return;
		bool bsectT = triangleTest(sst2,irect(fbucketX0,fbucketX1,fbucketY0,fbucketY1));
		if( false  == bsectT )
			return;

		auto l_is_inside_tile = [&](const CVector4&v) -> bool
		{
			return ( 		v.x>=fbucketX0
						&&	v.x<fbucketX1
						&&	v.y>=fbucketY0
						&&	v.y<fbucketY1 );
		};

		//////////////////////////////////////////


		if( 	use_scanline_renderer ||
				(	l_is_inside_tile(sst2.mScrPosA) &&
					l_is_inside_tile(sst2.mScrPosB) &&
					l_is_inside_tile(sst2.mScrPosC)
				)
		)
		{

			if( use_scanline_renderer )
			{
				SsOrdTri sot(sst2);
				RasterizeTri(aabuf,sot);
			}
			else // 
			{
				const CVector3& rst0 = rastri1.mVertex[0].mRST;
				const CVector3& rst1 = rastri1.mVertex[1].mRST;
				const CVector3& rst2 = rastri1.mVertex[2].mRST;

				const CVector3 rstC = (rst0+rst1+rst2)*one_third;

				float fctrx = sst2.mScrCenter.x;
				float fctry = sst2.mScrCenter.y;
				float fctrz = sst2.mScrCenter.z;
				int iax = int(fctrx);
				int iay = int(fctry);

				PreShadedFragmentPool& pfgs = aabuf.mPreFrags;

				PreShadedFragment& prefrag = pfgs.AllocPreFrag();
				prefrag.mfR = rstC.x;
				prefrag.mfS = rstC.y;
				prefrag.mfT = rstC.z;
				prefrag.mfZ = fctrz;
				prefrag.miPixIdxAA = aabuf.CalcAAPixelAddress(iax%imod,iay%imod);
			}
		}
		else
			do_subdivide = sst.mScreenArea2D>0.01f;
	}
	else
		do_subdivide = true;

	//////////////////////////////////////////
	if( do_subdivide ) 
	{
		// TODO : subdivide in screen space
		RasterTri t0,t1,t2,t3;
		rastri.SubDivAtCenter(t0,t1,t2,t3);
		SubdivideTri( ctx, t0 );
		SubdivideTri( ctx, t1 );
		SubdivideTri( ctx, t2 );
		SubdivideTri( ctx, t3 );
	}
}
void RasterTri::SubDivAtCenter(	RasterTri&t0,
		                        	RasterTri&t1,
        		                	RasterTri&t2,
                		        	RasterTri&t3 ) const
{
	auto l = [&](	RasterVtx&mid, 
					const RasterVtx& a,
					const RasterVtx& b )
	{
		mid.mObjPos=(a.mObjPos+b.mObjPos)*0.5f;
		mid.mObjNrm=(a.mObjNrm+b.mObjNrm).Normal();
		mid.mRST=(a.mRST+b.mRST)*0.5f;
	};
	auto l2 = [&](	RasterTri&t, 
					const RasterVtx& a,
					const RasterVtx& b,
					const RasterVtx& c )
	{
		t.mVertex[0]=a;
		t.mVertex[1]=b;
		t.mVertex[2]=c;
	};

	const RasterVtx& v0 = mVertex[0];
	const RasterVtx& v1 = mVertex[1];
	const RasterVtx& v2 = mVertex[2];
	RasterVtx v01, v02, v12;
	l( v01, v0, v1 );
	l( v02, v0, v2 );
	l( v12, v1, v2 );
	l2( t0, v02, v0, v01 );
	l2( t1, v2, v02, v12 );
	l2( t2, v02, v01, v12 );
	l2( t3, v12, v01, v1 );
}
/*
void Stage1Tri::SubDivNrm(	RasterTri&t0,
		                        RasterTri&t1,
        		                RasterTri&t2,
                		        RasterTri&t3 ) const
{
	auto l = [&](	rend_ivtx&mid, 
					const rend_ivtx& a,
					const rend_ivtx& b )
	{
		mid.mSX=(a.mSX+b.mSX)*0.5f;
		mid.mSY=(a.mSY+b.mSY)*0.5f;
		mid.mR=(a.mR+b.mR)*0.5f;
		mid.mS=(a.mS+b.mS)*0.5f;
		mid.mT=(a.mT+b.mT)*0.5f;
		mid.mfDepth=(a.mfDepth+b.mfDepth)*0.5f;
	};
	auto l2 = [&](	rend_triangle&t, 
					const rend_ivtx& a,
					const rend_ivtx& b,
					const rend_ivtx& c )
	{
		t.mSVerts[0]=a;
		t.mSVerts[1]=b;
		t.mSVerts[2]=c;
	};

	const rend_ivtx& v0 = mSVerts[0];
	const rend_ivtx& v1 = mSVerts[1];
	const rend_ivtx& v2 = mSVerts[2];
	rend_ivtx v01, v02, v12;
	l( v01, v0, v1 );
	l( v02, v0, v2 );
	l( v12, v1, v2 );
	l2( t0, v02, v0, v01 );
	l2( t1, v2, v02, v12 );
	l2( t2, v02, v01, v12 );
	l2( t3, v12, v01, v1 );
}*/

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/*float Stage1Tri::ComputeScreenArea() const
{	auto v0 = CVector3(mSVerts[0].mSX,mSVerts[0].mSY,0.0f);
	auto v1 = CVector3(mSVerts[1].mSX,mSVerts[1].mSY,0.0f);
	auto v2 = CVector3(mSVerts[2].mSX,mSVerts[2].mSY,0.0f);
	// area of triangle 1/2 length of cross product the vector of any two edges
	float farea = (v1-v0).Cross(v2-v0).Mag() * 0.5f;
	return farea;
}
float Stage1Tri::ComputeSurfaceArea() const
{	const auto& v0 = mSrcPos[0];
	const auto& v1 = mSrcPos[1];
	const auto& v2 = mSrcPos[2];
	// area of triangle 1/2 length of cross product the vector of any two edges
	float farea = (v1-v0).Cross(v2-v0).Mag() * 0.5f;
	return farea;
}*/


///////////////////////////////////////////////////////////////////////////////

SsOrdTri::SsOrdTri( const ScrSpcTri& sstri )
	: mSSTri(sstri)
{
	float sy0 = sstri.mScrPosA.y;
	float sy1 = sstri.mScrPosB.y;
	float sy2 = sstri.mScrPosC.y;
	////////////////////////////////
	mIndexA =			 (sy0<sy1)	// min Y	
					?	((sy0<sy2)	? 0 : 2)
					:	((sy1<sy2)	? 1 : 2);
	////////////////////////////////
	mIndexC =			 (sy0>sy1)	// max Y
					?	((sy0>sy2)	? 0 : 2)
					:	((sy1>sy2)	? 1 : 2);
	////////////////////////////////
	mIndexB = 3 - (mIndexA + mIndexC); 
	////////////////////////////////
}
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void MeshPrimModule::RasterizeTri( AABuffer& aabuf, const SsOrdTri& sotri )
{	auto tile = aabuf.mRasterTile;

	const ScrSpcTri& sst = sotri.mSSTri;

	const auto& vtxA = sst.GetScrPos(sotri.mIndexA);
	const auto& vtxB = sst.GetScrPos(sotri.mIndexB);
	const auto& vtxC = sst.GetScrPos(sotri.mIndexC);

	float iza = 1.0f/vtxA.z;
	float izb = 1.0f/vtxB.z;
	float izc = 1.0f/vtxC.z;

	float roza = iza;
	float soza = 0.0f;
	float toza = 0.0f;

	float rozb = 0.0f;
	float sozb = izb;
	float tozb = 0.0f;

	float rozc = 0.0f;
	float sozc = 0.0f;
	float tozc = izc;

	////////////////////////////////
	// trivially reject 
	////////////////////////////////
	
	if( int(vtxA.y - vtxC.y) == 0 ) return; 

	////////////////////////////////
	// guarantees
	////////////////////////////////
	// vtxA always guaranteed to be the top
	// vtxB.mSY >= vtxA.mSY
	// vtxC.mSY >= vtxB.mSY
	// vtxC.mSY > vtxA.mSY
	// no guarantees on left/right ordering
	////////////////////////////////
	float VtxAX_AaScreenSpace = vtxA.x;	// vtxA AA-ScreenSpace X
	float VtxAY_AaScreenSpace = vtxA.y;	// vtxA AA-ScreenSpace Y
	float VtxBX_AaScreenSpace = vtxB.x;	// vtxB AA-ScreenSpace X
	float VtxBY_AaScreenSpace = vtxB.y;	// vtxB AA-ScreenSpace Y
	float VtxCX_AaScreenSpace = vtxC.x;	// vtxC AA-ScreenSpace X
	float VtxCY_AaScreenSpace = vtxC.y;	// vtxC AA-ScreenSpace Y
	////////////////////////////////
	// gradient set up
	////////////////////////////////
	float dyAB = (VtxBY_AaScreenSpace - VtxAY_AaScreenSpace); // vertical distance between A and B in fp pixel coordinates
	float dyAC = (VtxCY_AaScreenSpace - VtxAY_AaScreenSpace); // vertical distance between A and C in fp pixel coordinates
	float dyBC = (VtxCY_AaScreenSpace - VtxBY_AaScreenSpace); // vertical distance between B and C in fp pixel coordinates
	////////////////////////////////
	float dxAB = (VtxBX_AaScreenSpace - VtxAX_AaScreenSpace); // horizontal distance between A and B in fp pixel coordinates
	float dxAC = (VtxCX_AaScreenSpace - VtxAX_AaScreenSpace); // horizontal distance between A and C in fp pixel coordinates
	float dxBC = (VtxCX_AaScreenSpace - VtxBX_AaScreenSpace); // horizontal distance between B and C in fp pixel coordinates
	////////////////////////////////
	float dzAB = (izb - iza); // depth distance between A and B in fp pixel coordinates
	float dzAC = (izc - iza); // depth distance between A and C in fp pixel coordinates
	float dzBC = (izc - izb); // depth distance between B and C in fp pixel coordinates
	////////////////////////////////
	float drAB = (rozb - roza); // depth distance between A and B in fp pixel coordinates
	float drAC = (rozc - roza); // depth distance between A and C in fp pixel coordinates
	float drBC = (rozc - rozb); // depth distance between B and C in fp pixel coordinates
	////////////////////////////////
	float dsAB = (sozb - soza); // depth distance between A and B in fp pixel coordinates
	float dsAC = (sozc - soza); // depth distance between A and C in fp pixel coordinates
	float dsBC = (sozc - sozb); // depth distance between B and C in fp pixel coordinates
	////////////////////////////////
	float dtAB = (tozb - toza); // depth distance between A and B in fp pixel coordinates
	float dtAC = (tozc - toza); // depth distance between A and C in fp pixel coordinates
	float dtBC = (tozc - tozb); // depth distance between B and C in fp pixel coordinates
	////////////////////////////////
	bool bABZERO = (dyAB==0.0f); // prevent division by zero
	bool bACZERO = (dyAC==0.0f); // prevent division by zero
	bool bBCZERO = (dyBC==0.0f); // prevent division by zero
	////////////////////////////////
	float dxABdy = bABZERO ? 0.0f : dxAB / dyAB;
	float dxACdy = bACZERO ? 0.0f : dxAC / dyAC;
	float dxBCdy = bBCZERO ? 0.0f : dxBC / dyBC;
	//////////////////////////////////
	float dzABdy = bABZERO ? 0.0f : dzAB / dyAB;
	float dzACdy = bACZERO ? 0.0f : dzAC / dyAC;
	float dzBCdy = bBCZERO ? 0.0f : dzBC / dyBC;
	//////////////////////////////////
	float drABdy = bABZERO ? 0.0f : drAB / dyAB;
	float drACdy = bACZERO ? 0.0f : drAC / dyAC;
	float drBCdy = bBCZERO ? 0.0f : drBC / dyBC;
	//////////////////////////////////
	float dsABdy = bABZERO ? 0.0f : dsAB / dyAB;
	float dsACdy = bACZERO ? 0.0f : dsAC / dyAC;
	float dsBCdy = bBCZERO ? 0.0f : dsBC / dyBC;
	//////////////////////////////////
	float dtABdy = bABZERO ? 0.0f : dtAB / dyAB;
	float dtACdy = bACZERO ? 0.0f : dtAC / dyAC;
	float dtBCdy = bBCZERO ? 0.0f : dtBC / dyBC;

	//////////////////////////////////
	// raster loop
	// y loop
	///////////////////////////////////

	int imod = aabuf.miAATileDim;

	int tileY0_AaScreenspace = (tile->miScreenYBase*aabuf.mAADim);
	int tileY1_AaScreenspace = ((tile->miScreenYBase+tile->miHeight)*aabuf.mAADim);
	int tileX0_AaScreenspace = tile->miScreenXBase*aabuf.mAADim;
	int tileX1_AaScreenspace = ((tile->miScreenXBase+tile->miWidth)*aabuf.mAADim);

	////////////////////////////////

	int iYA = int(std::floor(VtxAY_AaScreenSpace+0.5f)); // iYA, iYC, iy are in pixel coordinate
	int iYC = int(std::floor(VtxCY_AaScreenSpace+0.5f)); // iYA, iYC, iy are in pixel coordinate
	OrkAssert(iYC>=iYA);

	if( iYA<tileY0_AaScreenspace ) 
		iYA = tileY0_AaScreenspace;
	if( iYA>tileY1_AaScreenspace )
		iYA = tileY1_AaScreenspace;
	if( iYC<tileY0_AaScreenspace )
		iYC = tileY0_AaScreenspace;
	if( iYC>tileY1_AaScreenspace )
		iYC = tileY1_AaScreenspace;

	for( int iY=iYA; iY<iYC; iY++ )
	{	
		float pixel_center_Y = float(iY)+0.5f;
		///////////////////////////////////
		// edge selection (AC always active, AB or BC depending upon y)
		///////////////////////////////////
		bool bAB = pixel_center_Y<=VtxBY_AaScreenSpace;
		float yB = bAB ? VtxAY_AaScreenSpace : VtxBY_AaScreenSpace;
		float xB = bAB ? VtxAX_AaScreenSpace : VtxBX_AaScreenSpace;
		float zB = bAB ? iza : izb;
		float rB = bAB ? roza : rozb;
		float sB = bAB ? soza : sozb;
		float tB = bAB ? toza : tozb;
		float dXdyB = bAB ? dxABdy : dxBCdy;
		float dZdyB = bAB ? dzABdy : dzBCdy;
		float dRdyB = bAB ? drABdy : drBCdy;
		float dSdyB = bAB ? dsABdy : dsBCdy;
		float dTdyB = bAB ? dtABdy : dtBCdy;
		///////////////////////////////////
		float dyA = pixel_center_Y-VtxAY_AaScreenSpace;
		float dyB = pixel_center_Y-yB;
		///////////////////////////////////
		// calc left and right boundaries
		float fxLEFT = VtxAX_AaScreenSpace + dxACdy*dyA;
		float fxRIGHT = xB + dXdyB*dyB;
		float fzLEFT = iza + dzACdy*dyA;
		float fzRIGHT = zB + dZdyB*dyB;
		float frLEFT = roza + drACdy*dyA;
		float frRIGHT = rB + dRdyB*dyB;
		float fsLEFT = soza + dsACdy*dyA;
		float fsRIGHT = sB + dSdyB*dyB;
		float ftLEFT = toza + dtACdy*dyA;
		float ftRIGHT = tB + dTdyB*dyB;
		///////////////////////////////////
		// enforce left to right
		///////////////////////////////////
		if( fxLEFT>fxRIGHT ) 
		{	std::swap(fxLEFT,fxRIGHT);
			std::swap(fzLEFT,fzRIGHT);
			std::swap(frLEFT,frRIGHT);
			std::swap(fsLEFT,fsRIGHT);
			std::swap(ftLEFT,ftRIGHT);
		}
		int ixLEFT = int(std::floor(fxLEFT+0.5f));
		int ixRIGHT = int(std::floor(fxRIGHT+0.5f));
		///////////////////////////////////
		float fdZdX = (fzRIGHT-fzLEFT)/float(ixRIGHT-ixLEFT);
		float fdRdX = (frRIGHT-frLEFT)/float(ixRIGHT-ixLEFT);
		float fdSdX = (fsRIGHT-fsLEFT)/float(ixRIGHT-ixLEFT);
		float fdTdX = (ftRIGHT-ftLEFT)/float(ixRIGHT-ixLEFT);
		float fZ = fzLEFT; 
		float fR = frLEFT; 
		float fS = fsLEFT; 
		float fT = ftLEFT; 
		///////////////////////////////////
		ixRIGHT = ( ixRIGHT>tileX1_AaScreenspace ) 
				? tileX1_AaScreenspace 
				: ixRIGHT;
		///////////////////////////////////
		// prestep X
		///////////////////////////////////
		if( ixLEFT<tileX0_AaScreenspace )
		{
			float fxprestep = float(tileX0_AaScreenspace-ixLEFT);
			ixLEFT = tileX0_AaScreenspace;
			fZ += fdZdX*fxprestep;
			fR += fdRdX*fxprestep;
			fS += fdSdX*fxprestep;
			fT += fdTdX*fxprestep;
		}
		///////////////////////////////////
		// X loop
		///////////////////////////////////
		PreShadedFragmentPool& pfgs = aabuf.mPreFrags;
		//const rend_triangle* psrctri = tri.mpSourcePrim;
		//const rend_shader* pshader = psrctri->mpShader;
		///////////////////////////////////
		int iAAY = (iY)%imod;
		for( int iX=ixLEFT; iX<ixRIGHT; iX++ )
		{	
			int iAAX = (iX)%imod;
			{	
				PreShadedFragment& prefrag = pfgs.AllocPreFrag();

				float rz = 1.0f/fZ;
				prefrag.mfR = fR*rz;
				prefrag.mfS = fS*rz;
				prefrag.mfT = fT*rz;
				prefrag.mfZ = rz;
				prefrag.miPixIdxAA = aabuf.CalcAAPixelAddress(iAAX,iAAY);
			}
			///////////////////////////////////////////////
			fZ += fdZdX;
			fR += fdRdX;
			fS += fdSdX;
			fT += fdTdX;
		}
		///////////////////////////////////
	}
}

///////////////////////////////////////////////////////////////////////////////
} // namespace ork {
