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

RenderContext::RenderContext()
    : miNumTilesW(0)
    , miNumTilesH(0)
    , miImageWidth(1)
    , miImageHeight(1)
    , mPixelData(0)
    , miFrame(0)
    , mSplitAndDice(*this)
    , mAADim(1)
    , mAATileSize(mAADim*kTileDim)
{
    for( int i=0; i<kAABUFTILES; i++ )
    {
        auto aab = new AABuffer(kTileDim,mAADim);
        mAABufTiles.push(aab);
    }
}

AABuffer* RenderContext::BindAATile(int itx,int ity)
{
    const RasterTile& rtile = this->GetTile(itx,ity);

    AABuffer* aabuf = nullptr;

    while( false == mAABufTiles.try_pop(aabuf) )
    {
        usleep(1<<9);
    }
    aabuf->mRasterTile = &rtile;

    return aabuf;

}
void RenderContext::UnBindAABTile(AABuffer*tile)
{
    tile->mRasterTile = nullptr;
    mAABufTiles.push(tile);
}
    
///////////////////////////////////////////////////////////////////////////////

void RenderContext::operator=( const RenderContext& oth )
{
}

///////////////////////////////////////////////////////////////////////////////

void RenderContext::Update(OpGroup& ogrp)
{
    //printf( "miImageWidth<%d> miImageHeight<%d>\n", miImageWidth, miImageHeight );
    float faspect = float(miImageWidth)/float(miImageHeight);
    mMatrixV = ork::CMatrix4::Identity;
    mMatrixP = ork::CMatrix4::Identity;
//  mMatrixV.LookAt( mEye+ork::CVector3(0.0f,750.0f,0.0f), mTarget, -ork::CVector3::Green() );
    mMatrixV.LookAt( mEye, mTarget, ork::CVector3::Green() );
    //mMatrixP.Perspective( 25.0f, faspect, 1500.0f, 10000.0f );
    mMatrixP.Perspective( 25.0f, faspect, 1500.0f, 10000.0f );
//  mMatrixP.Perspective( 25.0f, faspect, 0.1f, 10.0f );
    mFrustum.Set( mMatrixV, mMatrixP );

    float fi = float(miFrame)/1800.0f;

    mMatrixM.SetToIdentity();
    mMatrixM.RotateY(fi*PI2);

    mMatrixMV = (mMatrixM*mMatrixV);
    mMatrixMVP = mMatrixMV*mMatrixP;

    ork::CVector3 root_topn = mFrustum.mTopPlane.GetNormal();
    ork::CVector3 root_botn = mFrustum.mBottomPlane.GetNormal();
    ork::CVector3 root_lftn = mFrustum.mLeftPlane.GetNormal();
    ork::CVector3 root_rhtn = mFrustum.mRightPlane.GetNormal();
    //ork::CVector3 root_topc = mFrustum.mFarCorners

    ork::Ray3 Ray[4];
    for( int i=0; i<4; i++ )
    {
        Ray[i] = ork::Ray3( mFrustum.mNearCorners[i], (mFrustum.mFarCorners[i]-mFrustum.mNearCorners[i]).Normal() );
    }

    /////////////////////////////////////////////////////////////////

    float faam = 0.0f; //float(mAADim>>1);
    float faap = 0.0f; //float(mAADim>>1);

    /////////////////////////////////////////////////////////////////

    const int ktovsize = GetTileOvSize();

    for( int iy=0; iy<miNumTilesH; iy++ )
    {
        /////////////////////////////////////////////////////
        // or maybe we want the pixel centers ?
        int ity = iy*ktovsize;
        int iby = ity+ktovsize; // or (kTileDim-1) ?
        float fity = (float(ity)+faam)/float(miImageHeight);
        float fiby = (float(iby)+faap)/float(miImageHeight);
        /////////////////////////////////////////////////////
        
        ork::CVector3 topn;
        ork::CVector3 botn;
        topn.Lerp( root_topn, -root_botn, fity );
        botn.Lerp( -root_topn, root_botn, fiby );
        topn.Normalize();
        botn.Normalize();

        /////////////////////////////////////////////////

        ork::CVector3 LN[2];
        ork::CVector3 RN[2];
        ork::CVector3 LF[2];
        ork::CVector3 RF[2];

        LN[0].Lerp( mFrustum.mNearCorners[0], mFrustum.mNearCorners[3], fity ); // left near top
        LN[1].Lerp( mFrustum.mNearCorners[0], mFrustum.mNearCorners[3], fiby ); // left near bot
        RN[0].Lerp( mFrustum.mNearCorners[1], mFrustum.mNearCorners[2], fity ); // right near top
        RN[1].Lerp( mFrustum.mNearCorners[1], mFrustum.mNearCorners[2], fiby ); // right near bot
        
        LF[0].Lerp( mFrustum.mFarCorners[0], mFrustum.mFarCorners[3], fity );       // left far top
        LF[1].Lerp( mFrustum.mFarCorners[0], mFrustum.mFarCorners[3], fiby );       // left far bot
        RF[0].Lerp( mFrustum.mFarCorners[1], mFrustum.mFarCorners[2], fity );       // right far top
        RF[1].Lerp( mFrustum.mFarCorners[1], mFrustum.mFarCorners[2], fiby );       // right far bot

        ork::CPlane NFCenterPlane( mFrustum.mNearPlane.GetNormal(), mFrustum.mCenter );

        /////////////////////////////////////////////////

        for( int ix=0; ix<miNumTilesW; ix++ )
        {
            /////////////////////////////////////////////////////
            // or maybe we want the pixel centers ?
            int ilx = ix*ktovsize;
            int irx = ilx+ktovsize; //-1);
            float filx = (float(ilx)+faam)/float(miImageWidth);
            float firx = (float(irx)+faap)/float(miImageWidth);
            /////////////////////////////////////////////////////

            ork::Ray3 RayTL; RayTL.BiLerp( Ray[0], Ray[1], Ray[3], Ray[2], filx, fity );
            ork::Ray3 RayTR; RayTR.BiLerp( Ray[0], Ray[1], Ray[3], Ray[2], firx, fity );
            ork::Ray3 RayBL; RayBL.BiLerp( Ray[0], Ray[1], Ray[3], Ray[2], filx, fiby );
            ork::Ray3 RayBR; RayBR.BiLerp( Ray[0], Ray[1], Ray[3], Ray[2], firx, fiby );

            float distTL, distTR, distBL, distBR;

            bool bisectTL = NFCenterPlane.Intersect( RayTL, distTL );
            bool bisectTR = NFCenterPlane.Intersect( RayTR, distTR );
            bool bisectBL = NFCenterPlane.Intersect( RayBL, distBL );
            bool bisectBR = NFCenterPlane.Intersect( RayBR, distBR );

            ork::CVector3 vCTL = RayTL.mOrigin + RayTL.mDirection*distTL;
            ork::CVector3 vCTR = RayTR.mOrigin + RayTR.mDirection*distTR;
            ork::CVector3 vCBL = RayBL.mOrigin + RayBL.mDirection*distBL;
            ork::CVector3 vCBR = RayBR.mOrigin + RayBR.mDirection*distBR;

            /////////////////////////////////////////////////////

            ork::CVector3 N0; N0.Lerp( LN[0], RN[0], filx );
            ork::CVector3 N1; N1.Lerp( LN[0], RN[0], firx );
            ork::CVector3 N2; N2.Lerp( LN[1], RN[1], firx );
            ork::CVector3 N3; N3.Lerp( LN[1], RN[1], filx );
            ork::CVector3 F0; F0.Lerp( LF[0], RF[0], filx );
            ork::CVector3 F1; F1.Lerp( LF[0], RF[0], firx );
            ork::CVector3 F2; F2.Lerp( LF[1], RF[1], firx );
            ork::CVector3 F3; F3.Lerp( LF[1], RF[1], filx );

            /////////////////////////////////////////////////////

            ork::CVector3 lftn;
            ork::CVector3 rhtn;
            lftn.Lerp( root_lftn, -root_rhtn, filx );
            rhtn.Lerp( -root_lftn, root_rhtn, firx );
            lftn.Normalize();
            rhtn.Normalize();

            int idx = CalcTileAddress(ix,iy);
            RasterTile& the_tile = mRasterTiles[idx];

            /////////////////////////////////////////////////////

            the_tile.mFrustum.mTopPlane.CalcFromNormalAndOrigin( topn, vCTL );
            the_tile.mFrustum.mBottomPlane.CalcFromNormalAndOrigin( botn, vCBL );
            the_tile.mFrustum.mLeftPlane.CalcFromNormalAndOrigin( lftn, vCTL );
            the_tile.mFrustum.mRightPlane.CalcFromNormalAndOrigin( rhtn, vCBR );

            /////////////////////////////////////////////////
            // near and far planes on the tiles are identical to the main image
            /////////////////////////////////////////////////

            the_tile.mFrustum.mNearPlane = mFrustum.mNearPlane;
            the_tile.mFrustum.mFarPlane = mFrustum.mFarPlane;

            the_tile.mFrustum.CalcCorners();

            /////////////////////////////////////////////////

            the_tile.mFrustum.mNearCorners[0].Lerp( LN[0], RN[0], filx );
            the_tile.mFrustum.mNearCorners[1].Lerp( LN[0], RN[0], firx );
            the_tile.mFrustum.mNearCorners[2].Lerp( LN[1], RN[1], firx );
            the_tile.mFrustum.mNearCorners[3].Lerp( LN[1], RN[1], filx );
            the_tile.mFrustum.mFarCorners[0].Lerp( LF[0], RF[0], filx );
            the_tile.mFrustum.mFarCorners[1].Lerp( LF[0], RF[0], firx );
            the_tile.mFrustum.mFarCorners[2].Lerp( LF[1], RF[1], firx );
            the_tile.mFrustum.mFarCorners[3].Lerp( LF[1], RF[1], filx );

            ork::CVector3 C;

            for( int i=0; i<4; i++ )
            {   C += the_tile.mFrustum.mNearCorners[i];
                C += the_tile.mFrustum.mFarCorners[i];
            }

            the_tile.mFrustum.mCenter = C*0.125f;

            float dT = the_tile.mFrustum.mTopPlane.GetPointDistance( the_tile.mFrustum.mCenter );
            float dB = the_tile.mFrustum.mBottomPlane.GetPointDistance( the_tile.mFrustum.mCenter );
            float dL = the_tile.mFrustum.mLeftPlane.GetPointDistance( the_tile.mFrustum.mCenter );
            float dR = the_tile.mFrustum.mRightPlane.GetPointDistance( the_tile.mFrustum.mCenter );
            float dN = the_tile.mFrustum.mNearPlane.GetPointDistance( the_tile.mFrustum.mCenter );
            float dF = the_tile.mFrustum.mFarPlane.GetPointDistance( the_tile.mFrustum.mCenter );

            //printf( "dT<%f> dB<%f> dL<%f> dR<%f> dN<%f> dF<%f> \n",
            //  dT, dB, dL, dR, dN, dF );

            OrkAssert( dT>0.0f );
            OrkAssert( dB>0.0f );
            OrkAssert( dL>0.0f );
            OrkAssert( dR>0.0f );
            OrkAssert( dN>0.0f );
            OrkAssert( dF>0.0f );

            /////////////////////////////////////////////////

        }
    }

    //auto prims = mPrimSet.AtomicCopy();

    //ogrp.drain();

    for( const auto& p : mPrimSet )
    {
        p->TransformAndCull(ogrp);
    }

    ogrp.drain();
}

///////////////////////////////////////////////////////////////////////////////

int RenderContext::GetTileX( float fx ) const
{
    int ix = int(std::floor(fx));
    int ibx = ix/mAATileSize;
    return ibx;
}
int RenderContext::GetTileY( float fy ) const
{
    int iy = int(std::floor(fy));
    int iby = iy/mAATileSize;
    return iby;
}

int RenderContext::GetTileOvSize() const
{
    return (kTileDim);
}

///////////////////////////////////////////////////////////////////////////////

void RenderContext::Resize( int iw, int ih )
{
    //////////////////////////////////
    // realloc buffer
    //////////////////////////////////
    if( 0 != mPixelData )
    {
        delete[] mPixelData;
    }
    mPixelData = new u32[ (iw*ih) ];
    //////////////////////////////////

    int ktovsize = GetTileOvSize();

    miImageWidth = iw;
    miImageHeight = ih;
    miNumTilesW = ((iw+ktovsize-1)/ktovsize);
    miNumTilesH = ((ih+ktovsize-1)/ktovsize);
    int inumtiles = miNumTilesW*miNumTilesH;
    mRasterTiles.resize(inumtiles);

    for( int iy=0; iy<miNumTilesH; iy++ )
    {
        int ity = iy*ktovsize;
        int iby = ity+(ktovsize-1);

        for( int ix=0; ix<miNumTilesW; ix++ )
        {
            int ilx = ix*ktovsize;
            int irx = ilx+(ktovsize-1);

            int idx = CalcTileAddress(ix,iy);

            mRasterTiles[idx].mAddress = idx;
            mRasterTiles[idx].miWidth = (irx<iw) ? ktovsize : (ktovsize-(1+irx-iw));
            mRasterTiles[idx].miHeight = (iby<ih) ? ktovsize : (ktovsize-(1+iby-ih));
            mRasterTiles[idx].miScreenXBase = ilx;
            mRasterTiles[idx].miScreenYBase = ity;

            mRasterTiles[idx].mAAScreenXBase = float(ilx)*float(mAADim);
            mRasterTiles[idx].mAAScreenYBase = float(ity)*float(mAADim);
            mRasterTiles[idx].mAAWidth = float(mRasterTiles[idx].miWidth)*float(mAADim);
            mRasterTiles[idx].mAAHeight = float(mRasterTiles[idx].miHeight)*float(mAADim);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////

const RasterTile& RenderContext::GetTile( int itx, int ity ) const
{
    OrkAssert( itx<miNumTilesW );
    OrkAssert( ity<miNumTilesH );
    OrkAssert( itx>=0 );
    OrkAssert( ity>=0 );

    int idx = CalcTileAddress(itx,ity);

    return mRasterTiles[idx];
}

///////////////////////////////////////////////////////////////////////////////

RasterTile& RenderContext::GetTile( int itx, int ity )
{
    OrkAssert( itx<miNumTilesW );
    OrkAssert( ity<miNumTilesH );
    OrkAssert( itx>=0 );
    OrkAssert( ity>=0 );

    int idx = CalcTileAddress(itx,ity);

    return mRasterTiles[idx];
}

///////////////////////////////////////////////////////////////////////////////

int RenderContext::CalcTileAddress( int itx, int ity ) const
{
    int idx = ity*miNumTilesW+itx;
    return idx;
}

///////////////////////////////////////////////////////////////////////////////

int RenderContext::CalcPixelAddress( int ix, int iy ) const
{
    if( ix>=miImageWidth ) ix=miImageWidth-1;
    if( iy>=miImageHeight ) iy=miImageHeight-1;
    if( ix<0 ) ix=0;
    if( iy<0 ) iy=0;
    return (iy*miImageWidth)+ix;
}

///////////////////////////////////////////////////////////////////////////////

void RenderContext::AddPrim(IGeoPrim*prim)
{
    //auto& s = mPrimSet.LockForWrite();
    mPrimSet.insert(prim);
    //mPrimSet.Unlock();
}

///////////////////////////////////////////////////////////////////////////////

void RenderContext::RenderToTile(AABuffer* aabuf)
{
    aabuf->Clear(*this);

    //auto prims = mPrimSet.AtomicCopy();

    for( const auto& p : mPrimSet )
    {
        p->RenderToTile(aabuf);
    }

    //mMeshMod.RenderOnTile(aabuf);
}

} // namespace ork {

