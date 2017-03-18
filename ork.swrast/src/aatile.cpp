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

AABuffer::AABuffer(int tile_dim,int aa_dim)
    : mTriangleClBuffer(0)
    , mFragInpClBuffer(0)
    , mFragOutClBuffer(0)
    , mRasterTile(nullptr)
    , mAADim(aa_dim)
    , mTileDim(tile_dim)
    , miAATileDim(aa_dim*tile_dim)
    , mAAFactor(aa_dim*aa_dim)
{
    mTriangleClBuffer = new NoCLFromHostBuffer();
    mFragInpClBuffer = new NoCLFromHostBuffer();
    mFragOutClBuffer = new NoCLToHostBuffer();

    int iinptribufsize = (1<<16)*(sizeof(float)*18);
    int iinpfragbufsize = (1<<16)*(sizeof(float)*8);
    int ioutfragbufsize = (1<<16)*(sizeof(float)*8);

    mTriangleClBuffer->resize( iinptribufsize );
    mFragInpClBuffer->resize( iinpfragbufsize );
    mFragOutClBuffer->resize( ioutfragbufsize );

    int inumpixpertile = tile_dim*tile_dim*mAAFactor;
    mColorBuffer = new uint32_t[ inumpixpertile ];
    mDepthBuffer = new float[ inumpixpertile ];
    mFragmentBuffer = new rend_fraglist[ inumpixpertile ];

}

///////////////////////////////////////////////////////////////////////////////

AABuffer::~AABuffer()
{
    if( mTriangleClBuffer ) delete mTriangleClBuffer;
    if( mFragInpClBuffer ) delete mFragInpClBuffer;
    if( mFragOutClBuffer ) delete mFragOutClBuffer;
}

///////////////////////////////////////////////////////////////////////////////

void AABuffer::Clear(RenderContext& rctx)
{
    static atomic_counter giatomcounter(0);

    const int gicounter = int(giatomcounter.fetch_and_increment());

    int ishift = (0==((gicounter&8)>>3));
    u8 ucr = (gicounter&1)<<(ishift+5);
    u8 ucg = (gicounter&2)<<(ishift+4);
    u8 ucb = (gicounter&4)<<(ishift+3);
    u32 upix = (ucb<<0)|u32(ucg<<8)|u32(ucr<<16);
    mClearColor = ork::CVector3( 0,0,0 );
    //mClearColor = ork::CVector3( float(ucr)/255.0f, float(ucg)/255.0f, float(ucb)/255.0f );

    for( int iy=0; iy<miAATileDim; iy++ )
    {
        for( int ix=0; ix<miAATileDim; ix++ )
        {
            int ipixidxAA = CalcAAPixelAddress(ix,iy);
            mColorBuffer[ipixidxAA] = upix;
            mDepthBuffer[ipixidxAA] = 1.0e30f;
            mFragmentBuffer[ipixidxAA].Reset();
        }
    }
    mFragmentPool.Reset();
}

///////////////////////////////////////////////////////////////////////////////

void AABuffer::Resolve(RenderContext& rctx, rend_fraglist_visitor& fragger)
{
    int imul = mAADim;
    int idiv = mAAFactor;
    float fdepthcomplexitysum = 0.0f;
    float fdepthcomplexityctr = 0.0f;

    int itw = mRasterTile->miWidth;
    int ith = mRasterTile->miHeight;
    int itx = mRasterTile->miScreenXBase;
    int ity = mRasterTile->miScreenYBase;

    fragger.Reset();
    for( int iy=0; iy<ith; iy++ )
    {
        int iabsy = (ity+iy);
        int iy2 = (iy*imul);

        for( int ix=0; ix<itw; ix++ )
        {
            int ix2 = (ix*imul);
            u32 u0=0;
            u32 u1=0;
            u32 u2=0;
            u32 u3=0;
            for( int iaay=0; iaay<mAADim; iaay++ )
            for( int iaax=0; iaax<mAADim; iaax++ )
            {   
                int iox = ix2+iaax;
                int ioy = iy2+iaay;
                int iaddress = CalcAAPixelAddress(iox,ioy);
                ////////////////////////////////////////////////////////////
                auto& fragitem = this->mFragmentBuffer[ iaddress ];
                fragitem.Visit( fragger );
                u32 upA = fragger.Composite(mClearColor).GetRGBAU32();
                //printf( "upa<%08x>\n", upA );
                ////////////////////////////////////////////////////////////
                u0 += ((upA)&0xff);
                u1 += ((upA>>8)&0xff)>>1;
                u2 += ((upA>>16)&0xff)>>1;
                u3 += ((upA>>24)&0xff)>>1;
            }
            ////////////////////////////////////////////////////////////
            // avg samples
            ////////////////////////////////////////////////////////////
            u0 = u0/idiv;
            u1 = u1/idiv;
            u2 = u2/idiv;
            u3 = u3/idiv;
            U32 upix = (u0)|(u1<<8)|(u2<<16)|(u3<<24);
            ////////////////////////////////////////////////////////////
            // store resolved pixel to framebuffer
            ////////////////////////////////////////////////////////////
            int iabsx = (itx+ix);
            int ipixidx = rctx.CalcPixelAddress(iabsx,iabsy);
            rctx.mPixelData[ipixidx] = upix;
            ////////////////////////////////////////////////////////////
        }
    }    
}

///////////////////////////////////////////////////////////////////////////////

int AABuffer::CalcAAPixelAddress( int ix, int iy ) const
{
    //OrkAssert( ix<miAATileDim );
    //OrkAssert( iy<miAATileDim );
    //OrkAssert( ix>=0 );
    //OrkAssert( iy>=0 );

    if( ix<0 ) ix = 0;
    if( iy<0 ) iy = 0;
    if( ix>=miAATileDim ) ix = (miAATileDim-1);
    if( iy>=miAATileDim ) iy = (miAATileDim-1);

    return (iy*miAATileDim)+ix;
}

///////////////////////////////////////////////////////////////////////////////

/*
void AABuffer::InitCl( const CLengine& eng )
{
    mTriangleClBuffer = new CLFromHostBuffer();
    mFragInpClBuffer = new CLFromHostBuffer();
    mFragOutClBuffer = new CLToHostBuffer();

    int iinptribufsize = (1<<16)*(sizeof(float)*18);
    mTriangleClBuffer->resize( iinptribufsize, eng.GetDevice() );

    int iinpfragbufsize = (1<<17)*(sizeof(float)*8);
    mFragInpClBuffer->resize( iinpfragbufsize, eng.GetDevice() );

    int ioutfragbufsize = (1<<18)*(sizeof(float)*8);
    mFragOutClBuffer->resize( ioutfragbufsize, eng.GetDevice() );


}*/


} // namespace ork {
