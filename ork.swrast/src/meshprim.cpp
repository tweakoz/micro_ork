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
			const rend_srcvtx& SrcTriV0 = SrcTri.mpVertices[0];
			const rend_srcvtx& SrcTriV1 = SrcTri.mpVertices[1];
			const rend_srcvtx& SrcTriV2 = SrcTri.mpVertices[2];
			const ork::CVector4& o0 = SrcTriV0.mPos;
			const ork::CVector4& o1 = SrcTriV1.mPos;
			const ork::CVector4& o2 = SrcTriV2.mPos;
			const ork::CVector3& on0 = SrcTriV0.mVertexNormal;
			const ork::CVector3& on1 = SrcTriV1.mVertexNormal;
			const ork::CVector3& on2 = SrcTriV2.mVertexNormal;

			auto rtri = new rend_triangle(*this);

			rtri->mfArea = SrcTri.mSurfaceArea;
			rtri->mpShader = pshader;
			rtri->mFaceNormal = SrcTri.mFaceNormal;

			rtri->mSrcPos[0] = SrcTriV0.mPos;
			rtri->mSrcPos[1] = SrcTriV1.mPos;
			rtri->mSrcPos[2] = SrcTriV2.mPos;
			rtri->mSrcNrm[0] = SrcTriV0.mVertexNormal;
			rtri->mSrcNrm[1] = SrcTriV1.mVertexNormal;
			rtri->mSrcNrm[2] = SrcTriV2.mVertexNormal;

			mPostXfTris.push_back(rtri);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////

void MeshPrimModule::XfChunk(size_t start, size_t end)
{
	const ork::CMatrix4& mtxM = mRenderContext.mMatrixM;
	const ork::CMatrix4& mtxMV = mRenderContext.mMatrixMV;
	const ork::CMatrix4& mtxMVP = mRenderContext.mMatrixMVP;

	float fiW = float(mRenderContext.miImageWidth);
	float fiH = float(mRenderContext.miImageHeight);
	float fwd4 = fiW*0.25f;
	float fhd4 = fiH*0.25f;

	float faadim(mRenderContext.mAADim);

	for( size_t i=start; i<end; i++ )
	{
		auto rtri = this->mPostXfTris[i];

		rtri->mIsVisible = false;

		const ork::CVector4& o0 = rtri->mSrcPos[0];
		const ork::CVector4& o1 = rtri->mSrcPos[1];
		const ork::CVector4& o2 = rtri->mSrcPos[2];
		ork::CVector4 w0 = o0.Transform(mtxM);
		ork::CVector4 w1 = o1.Transform(mtxM);
		ork::CVector4 w2 = o2.Transform(mtxM);
		const ork::CVector3& on0 = rtri->mSrcNrm[0];
		const ork::CVector3& on1 = rtri->mSrcNrm[1];
		const ork::CVector3& on2 = rtri->mSrcNrm[2];
		const ork::CVector3 wn0 = on0.Transform3x3( mtxM ).Normal();
		const ork::CVector3 wn1 = on1.Transform3x3( mtxM ).Normal();
		const ork::CVector3 wn2 = on2.Transform3x3( mtxM ).Normal();

		//////////////////////////////////////

		// TODO:
		//  displacement bounds checking here!
		// (make sure to pass prims that might be visible if displaced..)

		//////////////////////////////////////
		// trivial reject
		//////////////////////////////////////

		ork::CVector4 h0 = o0.Transform(mtxMVP);
		ork::CVector4 h1 = o1.Transform(mtxMVP);
		ork::CVector4 h2 = o2.Transform(mtxMVP);

		ork::CVector4 hd0 = h0;
		ork::CVector4 hd1 = h1;
		ork::CVector4 hd2 = h2;
		hd0.PerspectiveDivide();
		hd1.PerspectiveDivide();
		hd2.PerspectiveDivide();
		ork::CVector3 d0 = (hd1.GetXYZ()-hd0.GetXYZ());
		ork::CVector3 d1 = (hd2.GetXYZ()-hd1.GetXYZ());

		ork::CVector3 dX = d0.Cross(d1);

		bool bFRONTFACE = (dX.GetZ()<=0.0f);

		if( true == bFRONTFACE ) continue;

		int inuminside = 0;

		inuminside += (		((hd0.GetX()>=-1.0f) || (hd0.GetX()<=1.0f))
			&&	((hd0.GetY()>=-1.0f) || (hd0.GetY()<=1.0f))
			&&	((hd0.GetZ()>=-1.0f) || (hd0.GetZ()<=1.0f)) );

		inuminside += (		((hd1.GetX()>=-1.0f) || (hd1.GetX()<=1.0f))
			&&	((hd1.GetY()>=-1.0f) || (hd1.GetY()<=1.0f))
			&&	((hd1.GetZ()>=-1.0f) || (hd1.GetZ()<=1.0f)) );

		inuminside += (		((hd2.GetX()>=-1.0f) || (hd2.GetX()<=1.0f))
			&&	((hd2.GetY()>=-1.0f) || (hd2.GetY()<=1.0f))
			&&	((hd2.GetZ()>=-1.0f) || (hd2.GetZ()<=1.0f)) );

		if( 0 == inuminside )
		{
			continue;
		}

		rtri->mIsVisible = true;

		//////////////////////////////////////
		// the triangle passed the backface cull and trivial reject, queue it
		//////////////////////////////////////

		float fX0 = (0.5f+hd0.GetX()*0.5f)*fiW;
		float fY0 = (0.5f+hd0.GetY()*0.5f)*fiH;
		float fX1 = (0.5f+hd1.GetX()*0.5f)*fiW;
		float fY1 = (0.5f+hd1.GetY()*0.5f)*fiH;
		float fX2 = (0.5f+hd2.GetX()*0.5f)*fiW;
		float fY2 = (0.5f+hd2.GetY()*0.5f)*fiH;
		float fZ0 = hd0.GetZ();
		float fZ1 = hd1.GetZ();
		float fZ2 = hd2.GetZ();
		float fiZ0 = 1.0f/fZ0;
		float fiZ1 = 1.0f/fZ1;
		float fiZ2 = 1.0f/fZ2;

		rend_ivtx& v0 = rtri->mSVerts[0];
		rend_ivtx& v1 = rtri->mSVerts[1];
		rend_ivtx& v2 = rtri->mSVerts[2];


		v0.mSX = fX0;
		v0.mSY = fY0;
		v0.mfDepth = fZ0;
		v0.mfInvDepth = fiZ0;
		v0.mRoZ = fiZ0; 
		v0.mSoZ = 0.0f; 
		v0.mToZ = 0.0f; 
		v0.mWldSpacePos = w0;
		v0.mObjSpacePos = o0;
		v0.mObjSpaceNrm = on0;
		v0.mWldSpaceNrm = wn0;
		v1.mSX = fX1;
		v1.mSY = fY1;
		v1.mfDepth = fZ1;
		v1.mfInvDepth = fiZ1;
		v1.mRoZ = 0.0f; 
		v1.mSoZ = fiZ1; 
		v1.mToZ = 0.0f; 
		v1.mWldSpacePos = w1;
		v1.mObjSpacePos = o1;
		v1.mObjSpaceNrm = on1;
		v1.mWldSpaceNrm = wn1;
		v2.mSX = fX2;
		v2.mSY = fY2;
		v2.mfDepth = fZ2;
		v2.mfInvDepth = fiZ2;
		v2.mRoZ = 0.0f; 
		v2.mSoZ = 0.0f; 
		v2.mToZ = fiZ2; 
		v2.mWldSpacePos = w2;
		v2.mObjSpacePos = o2;
		v2.mObjSpaceNrm = on2;
		v2.mWldSpaceNrm = wn2;

		///////////////////////////////////////////////////
		ivertex iV0, iV1, iV2;
		iV0.iX = (fX0);
		iV0.iY = (fY0);
		iV1.iX = (fX1);
		iV1.iY = (fY1);
		iV2.iX = (fX2);
		iV2.iY = (fY2);
		float MinX = std::min(fX0,std::min(fX1,fX2));
		float MaxX = std::max(fX0,std::max(fX1,fX2));
		float MinY = std::min(fY0,std::min(fY1,fY2));
		float MaxY = std::max(fY0,std::max(fY1,fY2));

		int iminbx = mRenderContext.GetTileX(MinX)-1;
		int imaxbx = mRenderContext.GetTileX(MaxX)+1;
		int iminby = mRenderContext.GetTileY(MinY)-1;
		int imaxby = mRenderContext.GetTileY(MaxY)+1;

		if( iminbx<0 ) iminbx = 0;
		if( iminby<0 ) iminby = 0;
		if( imaxbx>=mRenderContext.miNumTilesW ) imaxbx = mRenderContext.miNumTilesW-1;
		if( imaxby>=mRenderContext.miNumTilesH ) imaxby = mRenderContext.miNumTilesH-1;

		rtri->mMinBx = iminbx;
		rtri->mMinBy = iminby;
		rtri->mMaxBx = imaxbx;
		rtri->mMaxBy = imaxby;
		rtri->mIV0 = iV0;
		rtri->mIV1 = iV1;
		rtri->mIV2 = iV2;
		rtri->mMinX = MinX;
		rtri->mMinY = MinY;
		rtri->mMaxX = MaxX;
		rtri->mMaxY = MaxY;

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
	size_t num_tris = mPostXfTris.size();
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

rend_triangle::rend_triangle(MeshPrimModule& tac )
	: mTAC(tac)
{
	mRefCount = 0;
}

///////////////////////////////////////////////////////////////////////////////

int rend_triangle::IncRefCount() const
{
	return mRefCount.fetch_and_increment();
}

///////////////////////////////////////////////////////////////////////////////

int rend_triangle::DecRefCount() const
{
	return mRefCount.fetch_and_decrement();
	//printf( "ir<%d>\n", ir );
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

static bool isInsideTriangle( const irect& ir, const ivertex& v0, const ivertex& v1, const ivertex& v2)
{ 
	float l = ir.l;
	float r = ir.r;
	float t = ir.t;
	float b = ir.b;
	float x0=v0.iX;
	float y0=v0.iY;
	float x1=v1.iX;
	float y1=v1.iY;
	float x2=v2.iX;
	float y2=v2.iY;

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

static bool triangleTest( const rend_triangle* rtri, const irect& ir )
{
	const ivertex& v0 = rtri->mIV0;
	const ivertex& v1 = rtri->mIV1;
	const ivertex& v2 = rtri->mIV2;

	float x0 = v0.iX; 
	float y0 = v0.iY; 
	float x1 = v1.iX; 
	float y1 = v1.iY; 
	float x2 = v2.iX; 
	float y2 = v2.iY; 
	float l = ir.l;
	float r = ir.r;
	float t = ir.t;
	float b = ir.b;

	//printf( "v0<%f %f>\n", v0.iX, v0.iY );
	//printf( "v1<%f %f>\n", v1.iX, v1.iY );
	//printf( "v2<%f %f>\n", v2.iX, v2.iY );
	//printf( "l<%f> r<%f> t<%f> b<%f>\n", l, r, t, b );

	if (x0 > l) if(x0 < r) if(y0 > t) if(y0 < b) return true;
	if (x1 > l) if( x1 < r ) if( y1 > t ) if( y1 < b) return true;
	if (x2 > l ) if( x2 < r ) if( y2 > t ) if( y2 < b) return true;

	if (isInsideTriangle(ir,v0,v1,v2)) return true;

	return(		isIntersecting(ir,x0, y0, x1, y1) 
			|| 	isIntersecting(ir,x1, y1, x2, y2)
			||	isIntersecting(ir,x0, y0, x2, y2)
	);

	//return false;
}

///////////////////////////////////////////////////////////////////////////////

bool boxisect(	const rend_triangle* rtri,
				//float fax0, float fay0, float fax1, float fay1,
				float fbx0, float fby0, float fbx1, float fby1 )
{	if (rtri->mMaxY < fby0) return false;
	if (rtri->mMinY > fby1) return false;
	if (rtri->mMaxX < fbx0) return false;
	if (rtri->mMinX > fbx1) return false;
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

	float fbucketX0 = float(tile->miScreenXBase);
	float fbucketY0 = float(tile->miScreenYBase);
	float fbucketX1 = fbucketX0+float(tile->miWidth);
	float fbucketY1 = fbucketY0+float(tile->miHeight);

    int itw = tile->miWidth;
    int ith = tile->miHeight;
    int itx = tile->miScreenXBase;
    int ity = tile->miScreenYBase;

    rendtri_context ctx( *aabuf );

	int accept = 0;
	int visible = 0;
	int reject = 0;

	const rend_shader* pshader = nullptr;

	for( auto rtri : mPostXfTris )
	{
		////////////////////////
		// skip backface culled 
		////////////////////////

		if( false == rtri->mIsVisible )
			continue;

		visible++;

		////////////////////////

		int iminbx = rtri->mMinBx;
		int iminby = rtri->mMinBy;
		int imaxbx = rtri->mMaxBx;
		int imaxby = rtri->mMaxBy;

		if( false==boxisect( rtri, fbucketX0, fbucketY0, fbucketX1, fbucketY1 ) )
		{
			reject++;
			continue;
		}
		//printf( "st1\n");
		bool bsectT = triangleTest(rtri,irect(fbucketX0,fbucketX1,fbucketY0,fbucketY1));
		if( false == bsectT )
		{
			reject++;
			continue;
		}
		//printf( "st2\n");

		accept++;

		// todo - multishader/mesh support
		pshader = rtri->mpShader;

		RasterizeTri( ctx, rtri );
		rtri->DecRefCount();

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
			
			pshader->ShadeBlock( aabuf, ipfragbase, icount, itx );
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

///////////////////////////////////////////////////////////////////////////////

void MeshPrimModule::RasterizeTri( const rendtri_context& ctx, const rend_triangle* tri )
{
	float sy0 = tri->mSVerts[0].mSY;
	float sy1 = tri->mSVerts[1].mSY;
	float sy2 = tri->mSVerts[2].mSY;
	////////////////////////////////
	int or0 =			 (sy0<sy1)	// min Y	
					?	((sy0<sy2)	? 0 : 2)
					:	((sy1<sy2)	? 1 : 2);
	////////////////////////////////
	int or2 =			 (sy0>sy1)	// max Y
					?	((sy0>sy2)	? 0 : 2)
					:	((sy1>sy2)	? 1 : 2);
	////////////////////////////////
	int or1 = 3 - (or0 + or2); 
	////////////////////////////////
	const rend_ivtx& vtxA = tri->mSVerts[or0];
	const rend_ivtx& vtxB = tri->mSVerts[or1];
	const rend_ivtx& vtxC = tri->mSVerts[or2];
	////////////////////////////////
	// trivially reject 
	if( int(vtxA.mSY - vtxC.mSY) == 0 ) return; 
	////////////////////////////////
	rend_subtri stri;
	stri.mpSourcePrim = tri;
	stri.vtxA = vtxA;
	stri.vtxB = vtxB;
	stri.vtxC = vtxC;
	stri.miIA = or0;
	stri.miIB = or1;
	stri.miIC = or2;
	RasterizeSubTri( ctx, stri );
	//////////////////////////////////
}
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void MeshPrimModule::RasterizeSubTri( const rendtri_context& ctx, const rend_subtri& tri )
{	AABuffer& aabuf = ctx.mAABuffer;
	auto tile = aabuf.mRasterTile;
	int idim1 = aabuf.mAADim;
	float fdim1(idim1);
	////////////////////////////////
	// guarantees
	////////////////////////////////
	// vtxA always guaranteed to be the top
	// vtxB.mSY >= vtxA.mSY
	// vtxC.mSY >= vtxB.mSY
	// vtxC.mSY > vtxA.mSY
	// no guarantees on left/right ordering
	////////////////////////////////
	const rend_ivtx& vtxA = tri.vtxA;
	const rend_ivtx& vtxB = tri.vtxB;
	const rend_ivtx& vtxC = tri.vtxC;
	////////////////////////////////
	float VtxAX_AaScreenSpace = vtxA.mSX*fdim1;	// vtxA AA-ScreenSpace X
	float VtxAY_AaScreenSpace = vtxA.mSY*fdim1;	// vtxA AA-ScreenSpace Y
	float VtxBX_AaScreenSpace = vtxB.mSX*fdim1;	// vtxB AA-ScreenSpace X
	float VtxBY_AaScreenSpace = vtxB.mSY*fdim1;	// vtxB AA-ScreenSpace Y
	float VtxCX_AaScreenSpace = vtxC.mSX*fdim1;	// vtxC AA-ScreenSpace X
	float VtxCY_AaScreenSpace = vtxC.mSY*fdim1;	// vtxC AA-ScreenSpace Y
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
	float dzAB = (vtxB.mfInvDepth - vtxA.mfInvDepth); // depth distance between A and B in fp pixel coordinates
	float dzAC = (vtxC.mfInvDepth - vtxA.mfInvDepth); // depth distance between A and C in fp pixel coordinates
	float dzBC = (vtxC.mfInvDepth - vtxB.mfInvDepth); // depth distance between B and C in fp pixel coordinates
	////////////////////////////////
	float drAB = (vtxB.mRoZ - vtxA.mRoZ); // depth distance between A and B in fp pixel coordinates
	float drAC = (vtxC.mRoZ - vtxA.mRoZ); // depth distance between A and C in fp pixel coordinates
	float drBC = (vtxC.mRoZ - vtxB.mRoZ); // depth distance between B and C in fp pixel coordinates
	////////////////////////////////
	float dsAB = (vtxB.mSoZ - vtxA.mSoZ); // depth distance between A and B in fp pixel coordinates
	float dsAC = (vtxC.mSoZ - vtxA.mSoZ); // depth distance between A and C in fp pixel coordinates
	float dsBC = (vtxC.mSoZ - vtxB.mSoZ); // depth distance between B and C in fp pixel coordinates
	////////////////////////////////
	float dtAB = (vtxB.mToZ - vtxA.mToZ); // depth distance between A and B in fp pixel coordinates
	float dtAC = (vtxC.mToZ - vtxA.mToZ); // depth distance between A and C in fp pixel coordinates
	float dtBC = (vtxC.mToZ - vtxB.mToZ); // depth distance between B and C in fp pixel coordinates
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

	int tileY0_AaScreenspace = (tile->miScreenYBase*idim1);
	int tileY1_AaScreenspace = ((tile->miScreenYBase+tile->miHeight)*idim1);
	int tileX0_AaScreenspace = tile->miScreenXBase*idim1;
	int tileX1_AaScreenspace = ((tile->miScreenXBase+tile->miWidth)*idim1);

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
		float zB = bAB ? vtxA.mfInvDepth : vtxB.mfInvDepth;
		float rB = bAB ? vtxA.mRoZ : vtxB.mRoZ;
		float sB = bAB ? vtxA.mSoZ : vtxB.mSoZ;
		float tB = bAB ? vtxA.mToZ : vtxB.mToZ;
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
		float fzLEFT = vtxA.mfInvDepth + dzACdy*dyA;
		float fzRIGHT = zB + dZdyB*dyB;
		float frLEFT = vtxA.mRoZ + drACdy*dyA;
		float frRIGHT = rB + dRdyB*dyB;
		float fsLEFT = vtxA.mSoZ + dsACdy*dyA;
		float fsRIGHT = sB + dSdyB*dyB;
		float ftLEFT = vtxA.mToZ + dtACdy*dyA;
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
		const rend_ivtx& srcvtxR = tri.mpSourcePrim->mSVerts[0];
		const rend_ivtx& srcvtxS = tri.mpSourcePrim->mSVerts[1];
		const rend_ivtx& srcvtxT = tri.mpSourcePrim->mSVerts[2];
		PreShadedFragmentPool& pfgs = aabuf.mPreFrags;
		//const rend_triangle* psrctri = tri.mpSourcePrim;
		//const rend_shader* pshader = psrctri->mpShader;
		///////////////////////////////////
		int iAAY = (iY)%imod;
		//if( iAAY>=0 && iAAY<imod )
		for( int iX=ixLEFT; iX<ixRIGHT; iX++ )
		{	
			int iAAX = (iX)%imod;
			//if( iAAX>=0 && iAAX<imod )
			{	
				PreShadedFragment& prefrag = pfgs.AllocPreFrag();
				
				float rz = 1.0f/fZ;
				prefrag.mfR = fR*rz;
				prefrag.mfS = fS*rz;
				prefrag.mfT = fT*rz;
				prefrag.mfZ = rz;
				prefrag.srcvtxR = & srcvtxR;
				prefrag.srcvtxS = & srcvtxS;
				prefrag.srcvtxT = & srcvtxT;
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
