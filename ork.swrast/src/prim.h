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

struct RasterVtx
{
    ork::CVector3       mObjPos;
    ork::CVector3       mObjNrm;
    ork::CVector3       mRST;

    RasterVtx operator+( const RasterVtx& o ) const
    {
        RasterVtx r;
        r.mObjPos = mObjPos+o.mObjPos;
        r.mObjNrm = mObjNrm+o.mObjNrm;
        r.mRST = mRST+o.mRST;
        return r;
    }
    RasterVtx operator*( float f ) const
    {
        RasterVtx r;
        r.mObjPos = mObjPos*f;
        r.mObjNrm = mObjNrm*f;
        r.mRST = mRST*f;
        return r;
    }
};

struct RasterTri
{
    RasterVtx           mVertex[3];

    void SubDivAtCenter(RasterTri&t0,
                        RasterTri&t1,
                        RasterTri&t2,
                        RasterTri&t3 ) const;

    /*void SubDivNrm( RasterTri&t0,
                    RasterTri&t1,
                    RasterTri&t2,
                    RasterTri&t3 ) const;*/
 
};

struct ScrSpcTri
{
    ScrSpcTri( const RasterTri& rt, const CMatrix4& mtxmvp, float fsw, float fsh );
    const RasterTri& mRasterTri;
    ork::CVector4 mScrPosA;
    ork::CVector4 mScrPosB;
    ork::CVector4 mScrPosC;
    ork::CVector3 mScrCenter;
    float mMinX;
    float mMinY;
    float mMaxX;
    float mMaxY;

    const ork::CVector4& GetScrPos(int idx) const;

    float mScreenArea2D;

    //float ComputeScreenArea(float fsw,float fsh) const;
};


struct Stage1Tri
{
    Stage1Tri(MeshPrimModule& tac );

    RasterTri mBaseTriangle;
    float               mfArea;
    const rend_shader*  mpShader;
    bool                mIsVisible;
    MeshPrimModule& mTAC;
    mutable atomic<int> mRefCount;
    float mMinX, mMinY;
    float mMaxX, mMaxY;

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
///////////////////////////////////////////////////////////////////////////////

struct SsOrdTri
{
    SsOrdTri(const ScrSpcTri&ss);
    const ScrSpcTri& mSSTri;
    int mIndexA;
    int mIndexB;
    int mIndexC;
};

struct MeshPrimModule : public IGeoPrim
{
    MeshPrimModule(RenderContext& rdata, rend_srcmesh* pmesh);

private:

    void RenderToTile(AABuffer* aabuf) override;
    void TransformAndCull(OpGroup& ogrp) override;

    void XfChunk(size_t start, size_t end);
    void SubdivideTri( const rendtri_context& ctx, const RasterTri& tri );

    void Reset();

    bool IsBoundToRasterTile(const RasterTile* prt ) const;

    void RasterizeTri( AABuffer& aab, const SsOrdTri& sstri );
    //void RrSubTri( const rendtri_context& ctx, const rend_subtri& subtri );

    ////////////////////////////////////////////////////////////
    static const int krtmap_size = 8192;
    ////////////////////////////////////////////////////////////
    rend_srcmesh*                     mpSrcMesh;
    RenderContext&                    mRenderContext;
    std::vector<Stage1Tri*>           mStage1Triangles;
    ork::atomic<int>                  mRtMap[krtmap_size];
};

///////////////////////////////////////////////////////////////////////////////
// context information for rasterizing a triangle (which color and z buffer, etc...)
///////////////////////////////////////////////////////////////////////////////
struct rendtri_context
{
    rendtri_context( AABuffer& aabuf ) 
        : mAABuffer( aabuf )
        , mpShader(nullptr)
    {}

    AABuffer& mAABuffer;
    const rend_shader* mpShader;
};

} // namespace ork {
