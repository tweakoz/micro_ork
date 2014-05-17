///////////////////////////////////////////////////////////////////////////////
// MicroOrk (Orkid)
// Copyright 1996-2013, Michael T. Mayers
// Provided under the MIT License (see LICENSE.txt)
///////////////////////////////////////////////////////////////////////////////

#include <ork/types.h>
#include "render_graph.h"

namespace ork {

TileRenderer::TileRenderer(RenderContext& rdata)
    : mRenderContext(rdata)
{

}
///////////////////////////////////////////////////////////////////////////////
void TileRenderer::QueueTiles( OpGroup& ogrp)
{
    for( int ih=0; ih<mRenderContext.miNumTilesH; ih++ )
    {
        for( int iw=0; iw<mRenderContext.miNumTilesW; iw++ )
        {
            auto l = [=]()
            {
                this->ProcessTile(iw,ih);
            };
            ogrp.push(Op(l,"tile"));
        }
    }
    //printf( "donequin\n");
}
void TileRenderer::ProcessTile( int iworkx, int iworky )
{
    AABuffer* aabuf = mRenderContext.BindAATile(iworkx,iworky);

    /////////////////////////////////////////////////////////
    // RASTERIZE
    /////////////////////////////////////////////////////////

    mRenderContext.RenderToTile(aabuf);

    /////////////////////////////////////////////////////////
    // AA resolve
    /////////////////////////////////////////////////////////

    //FragmentCompositorREYES& sorter = aabuf->mCompositorREYES;
    FragmentCompositorZBuffer& sorter = aabuf->mCompositorZB;
    aabuf->Resolve( mRenderContext, sorter );

    /////////////////////////////////////////////////////////

    mRenderContext.UnBindAABTile(aabuf);
}

} // namespace ork {
