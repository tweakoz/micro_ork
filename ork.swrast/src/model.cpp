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

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

rend_srcmesh* LoadObj( const char* pfname, RenderContext& rdata )
{
    ork::MeshUtil::toolmesh tmesh;
    tmesh.ReadFromWavefrontObj( pfname );

    int inumsubs = tmesh.GetNumSubMeshes();
    const ork::orklut<std::string, ork::MeshUtil::submesh*>& sublut = tmesh.RefSubMeshLut();

    rend_srcmesh* poutmesh = new rend_srcmesh;

    ork::AABox aabox;
    poutmesh->miNumSubMesh = inumsubs;
    poutmesh->mpSubMeshes = new rend_srcsubmesh[ inumsubs ];

    aabox.BeginGrow();
    for( int is=0; is<inumsubs; is++ )
    {
        ork::MeshUtil::submesh* psrcsub = sublut.GetItemAtIndex(is).second;
        rend_srcsubmesh& outsub = poutmesh->mpSubMeshes[is];

        aabox.Grow( psrcsub->GetAABox().Min() );
        aabox.Grow( psrcsub->GetAABox().Max() );

        //const ork::RgmSubMesh& Sub = prgmmodel->msubmeshes[is];
        //const ork::BakeShader* pbakeshader = Sub.mpShader;
        rend_shader* pshader = new Shader1();
        //rend_shader* pshader = new Shader2();
        pshader->mRenderContext = & rdata;
                
        int inumtri = psrcsub->GetNumPolys(3);

        outsub.miNumTriangles = inumtri;
        outsub.mpTriangles = new rend_srctri[ inumtri ];
        outsub.mpShader = pshader;

        for( int it=0; it<inumtri; it++ )
        {
            rend_srctri& outtri = outsub.mpTriangles[ it ];
            const ork::MeshUtil::poly& inpoly = psrcsub->RefPoly(it);

            int inumv = inpoly.GetNumSides();
            OrkAssert(inumv==3);
            outtri.mFaceNormal = inpoly.ComputeNormal( psrcsub->RefVertexPool() );
            outtri.mSurfaceArea = inpoly.ComputeArea(psrcsub->RefVertexPool(), ork::CMatrix4::Identity );
            int iv0 = inpoly.GetVertexID(0);
            int iv1 = inpoly.GetVertexID(1);
            int iv2 = inpoly.GetVertexID(2);
            const ork::MeshUtil::vertex& v0 = psrcsub->RefVertexPool().GetVertex(iv0);
            const ork::MeshUtil::vertex& v1 = psrcsub->RefVertexPool().GetVertex(iv1);
            const ork::MeshUtil::vertex& v2 = psrcsub->RefVertexPool().GetVertex(iv2);
            
            outtri.mpVertices[0].mPos = v0.mPos;
            outtri.mpVertices[0].mVertexNormal = v0.mNrm;
            outtri.mpVertices[0].mUv = v0.mUV[0].mMapTexCoord;
            outtri.mpVertices[1].mPos = v1.mPos;
            outtri.mpVertices[1].mVertexNormal = v1.mNrm;
            outtri.mpVertices[1].mUv = v1.mUV[0].mMapTexCoord;
            outtri.mpVertices[2].mPos = v2.mPos;
            outtri.mpVertices[2].mVertexNormal = v2.mNrm;
            outtri.mpVertices[2].mUv = v2.mUV[0].mMapTexCoord;

        }
    }
    aabox.EndGrow();

    auto aamin = aabox.Min();
    auto aamax = aabox.Max();
    ork::CVector3 size = (aamax-aamin);
    ork::CVector3 centr = (aamin+aamax)*0.5f;

    printf( "obj::aabox min<%f %f %f> max<%f %f %f>\n", 
            aamin.GetX(), aamin.GetY(), aamin.GetZ(),
            aamax.GetX(), aamax.GetY(), aamax.GetZ()
             );
    printf( "obj::size<%f %f %f> obj::centr<%f %f %f>\n", 
            size.GetX(), size.GetY(), size.GetZ(),
            centr.GetX(), centr.GetY(), centr.GetZ()
             );

    poutmesh->mTarget = centr+ork::CVector3(0.0f,-size.Mag()*0.05,0.0f);
    poutmesh->mEye = poutmesh->mTarget+ork::CVector3(0.0f,size.Mag()*0.4,size.Mag()*2.0f);

    return poutmesh;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#if 0
static rend_srcmesh* LoadRgm( const char* pfname, RenderData& rdata )
{
    rend_srcmesh* poutmesh = new rend_srcmesh;
    ork::Engine  RayEngine;

    ShaderBuilder builder(&RayEngine,&rdata);
    ork::RgmModel* prgmmodel = ork::LoadRgmFile( pfname, builder );
    ork::CVector3 dist = (prgmmodel->mAABox.Max()-prgmmodel->mAABox.Min());

    poutmesh->mTarget = (prgmmodel->mAABox.Min()+prgmmodel->mAABox.Max())*0.5f;
    poutmesh->mEye = poutmesh->mTarget-ork::CVector3(0.0f,0.0f,dist.Mag());

    int inumsub = prgmmodel->minumsubs;

    poutmesh->miNumSubMesh = inumsub;
    poutmesh->mpSubMeshes = new rend_srcsubmesh[ inumsub ];

    for( int is=0; is<inumsub; is++ )
    {
        rend_srcsubmesh& outsub = poutmesh->mpSubMeshes[is];

        const ork::RgmSubMesh& Sub = prgmmodel->msubmeshes[is];
        const ork::BakeShader* pbakeshader = Sub.mpShader;
        rend_shader* pshader = pbakeshader->mPlatformShader.Get<rend_shader*>();

        int inumtri = Sub.minumtris;

        outsub.miNumTriangles = inumtri;
        outsub.mpTriangles = new rend_srctri[ inumtri ];
        outsub.mpShader = pshader;

        for( int it=0; it<inumtri; it++ )
        {
            rend_srctri& outtri = outsub.mpTriangles[ it ];
            const ork::RgmTri& rgmtri = Sub.mtriangles[it];

            outtri.mFaceNormal = rgmtri.mFacePlane.GetNormal();
            outtri.mSurfaceArea = rgmtri.mArea;
            outtri.mpVertices[0].mPos = rgmtri.mpv0->pos;
            outtri.mpVertices[0].mVertexNormal = rgmtri.mpv0->nrm;
            outtri.mpVertices[0].mUv = rgmtri.mpv0->uv;
            outtri.mpVertices[1].mPos = rgmtri.mpv1->pos;
            outtri.mpVertices[1].mVertexNormal = rgmtri.mpv1->nrm;
            outtri.mpVertices[1].mUv = rgmtri.mpv1->uv;
            outtri.mpVertices[2].mPos = rgmtri.mpv2->pos;
            outtri.mpVertices[2].mVertexNormal = rgmtri.mpv2->nrm;
            outtri.mpVertices[2].mUv = rgmtri.mpv2->uv;

        }
    }

    return poutmesh;
}
#endif


} // namespace ork {
