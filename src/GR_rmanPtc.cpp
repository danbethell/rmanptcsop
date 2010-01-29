/*
Copyright (c) 2010, Dan Bethell.
Copyright (c) 2009, Double Negative Visual Effects.
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.

    * Neither the names of RmanPtcSop, Double Negative Visual Effects,
    nor the names of its contributors may be used to endorse or promote products
    derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

// houdini
#include <RE/RE_Render.h>
#include <GEO/GEO_Primitive.h>
#include <GU/GU_Detail.h>
#include <GU/GU_PrimGroupClosure.h>
#include <GR/GR_RenderTable.h>
#include <UT/UT_Interrupt.h>
#include <UT/UT_DMatrix4.h>

// rman point cloud api
#include <pointcloud.h>

// local headers
#include "GR_rmanPtc.h"
#include "rmanPtcDetail.h"
using namespace rmanPtcSop;

void GR_rmanPtc::renderWire( GU_Detail *gdp,
    RE_Render &ren,
    const GR_AttribOffset &ptinfo,
    const GR_DisplayOption *dopt,
    float lod,
    const GU_PrimGroupClosure *hidden_geometry
    )
{
    int			 i, nprim;
    GEO_Primitive 	*prim;
    UT_Vector3		 v3;

    rmanPtcSop::rmanPtcDetail *detail = dynamic_cast<rmanPtcSop::rmanPtcDetail*>(gdp);
    if ( !detail )
        return;

    // rebuild our display list
    if ( detail->redraw )
    {
        srand(0);

        GEO_PointList &points = detail->points();
        int display_channel = detail->display_channel;
        // render as points
        GEO_Point *pt = 0;
        UT_Vector4 pos;
        float col[3];

        if ( !detail->use_disk )
        {
            ren.pushPointSize(detail->point_size);
            ren.beginPoint();
        }

        for ( unsigned int i=0; i<points.entries(); ++i )
        {
            if ( rand()/(float)RAND_MAX<=detail->display_probability )
            {
                // point position
                pt = points[i];
                pos = pt->getPos();

                // display colour
                float *ptr = pt->castAttribData<float>(
                        detail->attributes[display_channel] );
                if ( detail->attribute_size[display_channel]==1)
                    col[0] = col[1] = col[2] = ptr[0];
                else
                {
                    col[0] = ptr[0];
                    col[1] = ptr[1];
                    col[2] = ptr[2];
                }

                // draw point
                if ( !detail->use_cull_bbox ||
                        detail->cull_bbox.isInside( pos ) )
                {
                    if ( !detail->use_disk )
                    {
                        // render as points
                        ren.setColor( col[0], col[1], col[2], 1 );
                        ren.vertex3DW( pos.x(), pos.y(), pos.z() );
                    }
                    else
                    {
                        // render as disks
                        UT_Vector3 n = *pt->castAttribData<UT_Vector3>(detail->N_attrib);
                        float r = *pt->castAttribData<float>(detail->R_attrib);
                        n.normalize();
                        UT_Vector3 ref(1,0,0);
                        UT_Vector3 up = ref;
                        up.cross(n);
                        up.normalize();
                        UT_Vector3 right = up;
                        right.cross(n);
                        right.normalize();
                        UT_DMatrix4 mat(
                                right.x(), right.y(), right.z(), 0,
                                up.x(), up.y(), up.z(), 0,
                                n.x(), n.y(), n.z(), 0,
                                pos.x(), pos.y(), pos.z(), 1 );
                        ren.pushMatrix();
                        ren.multiplyMatrix(mat);
                        ren.pushColor( UT_Color( UT_RGB, col[0], col[1], col[2] ) );
                        ren.circlefW( 0, 0, detail->point_size * r, 8 );
                        ren.popColor();
                        ren.popMatrix();
                    }
                }
            }
        }

        if ( !detail->use_disk )
        {
            ren.endPoint();
        }
    }
}

void GR_rmanPtc::renderShaded(GU_Detail *gdp,
		    RE_Render &ren,
		    const GR_AttribOffset &ptinfo,
		    const GR_DisplayOption *dopt,
		    float lod,
		    const GU_PrimGroupClosure *hidden_geometry)
{
    // We don't want to light the points as they have no normals.
    GR_Detail::toggleLightShading(ren, 0);
    renderWire(gdp, ren, ptinfo, dopt, lod, hidden_geometry);
    GR_Detail::toggleLightShading(ren, 1);
}

void newRenderHook( GR_RenderTable *table )
{
    GR_rmanPtc *hook = new GR_rmanPtc;
    table->addHook(hook);
}
