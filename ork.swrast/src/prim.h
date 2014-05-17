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

namespace ork {

struct IGeoPrim;        // 
class RenderContext;
struct rend_shader;
struct MeshPrimModule;
struct AABuffer;
struct TriangleBatch;
struct rendtri_context;
struct RasterTile;

///////////////////////////////////////////////////////////////////////////////

struct rend_srcvtx
{
    ork::CVector3   mPos;
    ork::CVector3   mVertexNormal;
    ork::CVector3   mUv;
};

struct rend_srctri
{
    rend_srcvtx     mpVertices[3];
    ork::CVector3   mFaceNormal;
    float           mSurfaceArea;
};

struct rend_srcsubmesh
{
    int             miNumTriangles;
    rend_srctri*    mpTriangles;
    rend_shader*    mpShader;

    rend_srcsubmesh() : miNumTriangles(0), mpTriangles(0), mpShader(0) {}
};

struct rend_srcmesh
{
    int                 miNumSubMesh;
    rend_srcsubmesh*    mpSubMeshes;
    ork::AABox          mAABox;
    ork::CVector3       mTarget;
    ork::CVector3       mEye;

    rend_srcmesh() : miNumSubMesh(0), mpSubMeshes(0) {}
};

///////////////////////////////////////////////////////////////////////////////

struct rend_ivtx
{
    float   mSX;
    float   mSY;
    float   mRoZ;
    float   mSoZ;
    float   mToZ;
    float   mfDepth;
    float   mfInvDepth;
    ork::CVector3 mWldSpacePos;
    ork::CVector3 mObjSpacePos;
    ork::CVector3 mWldSpaceNrm;
    ork::CVector3 mObjSpaceNrm;
};
///////////////////////////////////////////////////////////////////////////////
struct ivertex
{
    float iX, iY;
};

struct rend_triangle
{
    rend_triangle(MeshPrimModule& tac );

    rend_ivtx           mSVerts[3];
    ork::CVector4       mSrcPos[3];
    ork::CVector3       mSrcNrm[3];
    ork::CVector3       mFaceNormal;
    float               mfArea;
    const rend_shader*  mpShader;
    bool                mIsVisible;
    MeshPrimModule& mTAC;
    mutable atomic<int> mRefCount;
    int mMinBx, mMinBy;
    int mMaxBx, mMaxBy;
    float mMinX, mMinY;
    float mMaxX, mMaxY;
    ivertex mIV0, mIV1, mIV2;

    int IncRefCount() const;
    int DecRefCount() const;
};
///////////////////////////////////////////////////////////////////////////////
struct rend_subtri
{
    rend_ivtx vtxA;
    rend_ivtx vtxB;
    rend_ivtx vtxC;
    const rend_triangle* mpSourcePrim;
    int miIA;
    int miIB;
    int miIC;
};

///////////////////////////////////////////////////////////////////////////////

struct IGeoPrim // reyes course primitive (pre-diced)
{
    IGeoPrim() {}

    virtual void RenderToTile(AABuffer* aabuf) = 0;
    virtual void TransformAndCull(OpGroup& ogrp) = 0;
    //ork::AABox    mBoundingBoxWorldSpace;
    //ork::AABox    mBoundingBoxScreenSpace;
};

///////////////////////////////////////////////////////////////////////////////

struct GeoPrimList // reyes primitive
{
    // 
    ork::LockedResource<std::vector<IGeoPrim*>> mPrimitives;

    void AddPrim( IGeoPrim* prim ); // thread safe function

    GeoPrimList( const GeoPrimList& oth ) {}
    GeoPrimList() {}
};

///////////////////////////////////////////////////////////////////////////////

struct ClipVert
{   ork::CVector3 pos;
    void Lerp( const ClipVert& va, const ClipVert& vb, float flerp )
    {   pos.Lerp( va.pos, vb.pos, flerp );
    }
    const ork::CVector3& Pos() const { return pos; }
    ClipVert(const ork::CVector3&tp) : pos(tp) {}
    ClipVert() : pos() {}
};

///////////////////////////////////////////////////////////////////////////////

class ClipPoly
{
    static const int kmaxverts = 32;

    ClipVert    mverts[kmaxverts];
    int         minumverts;
public:
    typedef ClipVert VertexType;
    void AddVertex( const ClipVert& v ) { mverts[minumverts++]=v; }
    int GetNumVertices() const { return minumverts; }
    const ClipVert& GetVertex( int idx ) const { return mverts[idx]; }
    ClipPoly() : minumverts(0) {}
    void SetDefault()
    {   minumverts = 0;
    }
};

///////////////////////////////////////////////////////////////////////////////

struct TriangleBatch
{
    std::vector<rend_triangle*>             mPostTransformTriangles;
    ork::atomic<int>                        mTriangleIndex;
    rend_shader*                            mShader;

    rend_triangle* GetTriangle()
    {
        int idx = mTriangleIndex.fetch_and_increment();
        return mPostTransformTriangles[idx];
    }

    void AddTriangle(rend_triangle* rtri)
    {
        int idx = mTriangleIndex.fetch_and_increment();
        mPostTransformTriangles.push_back(rtri);
        rtri->IncRefCount();
    } 

    TriangleBatch()
    {
        Reset();
    }
    void Reset()
    {
        mShader = nullptr;
        mTriangleIndex = 0;
        mPostTransformTriangles.clear();
    }
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

struct MeshPrimModule : public IGeoPrim
{
    MeshPrimModule(RenderContext& rdata, rend_srcmesh* pmesh);

private:

    void RenderToTile(AABuffer* aabuf) override;
    void TransformAndCull(OpGroup& ogrp) override;

    void XfChunk(size_t start, size_t end);
    void RasterizeTri( const rendtri_context& ctx, const rend_triangle* tri );
    void RasterizeSubTri( const rendtri_context& ctx, const rend_subtri& tri );

    void Reset();

    bool IsBoundToRasterTile(const RasterTile* prt ) const;

    ////////////////////////////////////////////////////////////
    static const int krtmap_size = 8192;
    ////////////////////////////////////////////////////////////
    rend_srcmesh*                     mpSrcMesh;
    RenderContext&                    mRenderContext;
    std::vector<rend_triangle*>       mPostXfTris;
    ork::atomic<int>                  mRtMap[krtmap_size];
};

///////////////////////////////////////////////////////////////////////////////
// context information for rasterizing a triangle (which color and z buffer, etc...)
///////////////////////////////////////////////////////////////////////////////
struct rendtri_context
{
    rendtri_context( AABuffer& aabuf ) : mAABuffer( aabuf ) {}
    AABuffer& mAABuffer;
};

} // namespace ork {
