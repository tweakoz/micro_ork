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

struct PreShadedFragment;
struct rend_fragment;
struct rend_shader;
struct rend_volume_shader;
struct AABuffer;
struct rend_prefragsubgroup;
struct RasterTri;


struct DisplacementShaderContext
{
    DisplacementShaderContext(const RenderContext& rctx, const RasterTri&inp)
        : mRenderContext(rctx)
        , mRasterTri(inp)
    {

    }

    const RenderContext& mRenderContext;
    const RasterTri& mRasterTri;
};

typedef std::function<RasterTri(const DisplacementShaderContext&dsc)> displacement_lambda_t;

///////////////////////////////////////////////////////////////////////////////

struct rend_shader
{
    enum eType
    {
        EShaderTypeSurface = 0,
        EShaderTypeAtmosphere,
        EShaderTypeDisplacement,
    };

    const RenderContext*            mRenderContext;
    const rend_volume_shader*       mpVolumeShader;
    displacement_lambda_t           mDisplacementShader;

    ////////////////////////////////////////////

    rend_shader()
        : mRenderContext(nullptr)
        , mpVolumeShader(nullptr)
        , mDisplacementShader(nullptr)
    {}
    virtual eType GetType() const = 0;
    virtual void ShadeBlock( AABuffer* aabuf, int ifragbase, int icount ) const = 0;

};

///////////////////////////////////////////////////////////////////////////////

struct PreShadedFragment
{
    float                   mfR;
    float                   mfS;
    float                   mfT;
    float                   mfZ;
    CVector3                mON;
    int                     miPixIdxAA;
};

///////////////////////////////////////////////////////////////////////////////

struct PreShadedFragmentPool
{
    std::vector<PreShadedFragment>   mPreFrags;
    int                         miNumPreFrags;
    int                         miMaxPreFrags;
    PreShadedFragment& AllocPreFrag();
    void Reset();
    PreShadedFragmentPool();
};

///////////////////////////////////////////////////////////////////////////////

struct rend_fragment
{
    ork::CVector4           mRGBA;
    ork::CVector3           mWorldPos;  // needed for volume shaders
    ork::CVector3           mWldSpaceNrm;
    float                   mZ;
    const rend_fragment*    mpNext;
    const rend_shader*      mpShader;

    rend_fragment()
        : mpNext(0)
        , mZ(0.0f)
        , mRGBA(0.0f,0.0f,0.0f,1.0f)
        , mpShader(0)
    {
    }
};

///////////////////////////////////////////////////////////////////////////////

struct rend_texture2D
{
    ork::CVector4*  mpData;
    int             miWidth;
    int             miHeight;
    //mutable cl_mem    mCLhandle;
    rend_texture2D();
    ~rend_texture2D();
    ork::CVector4 sample_point( float u, float v, bool wrapu, bool wrapv ) const;

    void Load( const std::string& pth );
    void Init( /*const CLDevice* pdev*/ ) const;
};

///////////////////////////////////////////////////////////////////////////////

struct rend_surface_shader : public rend_shader
{
    virtual eType GetType() const { return EShaderTypeSurface; }
    virtual void Compute( rend_fragment* pfrag ) = 0;
};

///////////////////////////////////////////////////////////////////////////////

struct rend_volume_shader 
{
//  virtual void Compute( rend_fragment* pfrag ) = 0;
    uint32_t    mBooleanKey;
    rend_volume_shader() : mBooleanKey(0) {}
    virtual ork::CVector4 ShadeVolume( const ork::CVector3& entrywpos, const ork::CVector3& exitwpos ) const = 0;
};

///////////////////////////////////////////////////////////////////////////////

struct rend_fraglist_visitor
{
    virtual void Visit( const rend_fragment* pnode ) = 0;
    virtual void Reset() = 0;
    virtual ork::CVector3 Composite(const ork::CVector3&clrcolor) = 0;
};

///////////////////////////////////////////////////////////////////////////////

struct rend_fraglist
{
    rend_fragment* mpHead;

    void AddFragment( rend_fragment* pfrag );

    int DepthComplexity() const;
    void Visit( rend_fraglist_visitor& visitor ) const;
    
    rend_fraglist();
    void Reset();
};

///////////////////////////////////////////////////////////////////////////////

struct FragmentPoolNode
{
    static const int kNumFragments = 1<<14;
    rend_fragment                       mFragments[kNumFragments];
    int                                 mFragmentIndex;
    ///////////////////////////////////////////////////////////
    rend_fragment* AllocFragment();
    void AllocFragments(rend_fragment** ppfrag, int icount);
    void Reset();
    int GetNumAvailable() const { return (kNumFragments-mFragmentIndex); }
    FragmentPoolNode();
};

///////////////////////////////////////////////////////////////////////////////

struct FragmentPool
{
    std::vector<FragmentPoolNode*>      mFragmentPoolNodes;
    std::vector<rend_fragment*>         mFreeFragments;
    int                                 miPoolNodeIndex;
    FragmentPoolNode*                   mpCurNode;
    ///////////////////////////////////////////////////////////
    rend_fragment* AllocFragment();
    bool AllocFragments( rend_fragment**, int icount );
    void FreeFragment(rend_fragment*);
    void Reset();
    FragmentPool();
};

///////////////////////////////////////////////////////////////////////////////
// reyes style a-buffer
///////////////////////////////////////////////////////////////////////////////

struct FragmentCompositorABuffer : public rend_fraglist_visitor
{
    static const int kmaxfrags=256; // max depth complexity
    int miNumFragments;
    const rend_fragment*    mpFragments[kmaxfrags];
    const rend_fragment*    mpSortedFragments[kmaxfrags];
    float                   mFragmentZ[kmaxfrags];
    float                   opaqueZ;
    const rend_fragment*    mpOpaqueFragment;
    ork::RadixSort          mRadixSorter;
    int                     miThreadID;
    FragmentCompositorABuffer() : miThreadID(0), opaqueZ(1.0e30f), miNumFragments(0), mpOpaqueFragment(0) {}
    void Visit( const rend_fragment* pfrag ); // virtual
    void SortAndHide(); // Sort and Hide occluded
    void Reset() final;
    ork::CVector3 Composite(const ork::CVector3&clrcolor) final;
    ////////////////////////////////////////////
};

///////////////////////////////////////////////////////////////////////////////
// std zbuffer
///////////////////////////////////////////////////////////////////////////////

struct FragmentCompositorZBuffer : public rend_fraglist_visitor
{
    float                   opaqueZ;
    const rend_fragment*    mpOpaqueFragment;
    FragmentCompositorZBuffer() : opaqueZ(1.0e30f), mpOpaqueFragment(0) {}
    void Visit( const rend_fragment* pfrag ) final; 
    void Reset() final;
    atomic<int> miNumFragments;
    ork::CVector3 Composite(const ork::CVector3&clrcolor) final;
    ////////////////////////////////////////////
};

} // namespace ork {
