#include <RE/RE_Render.h>
#include <GEO/GEO_Primitive.h>
#include <GU/GU_Detail.h>
#include <GU/GU_PrimGroupClosure.h>
#include <GR/GR_RenderTable.h>
#include <UT/UT_Interrupt.h>

// rman point cloud
#include <pointcloud.h>

#include "GR_rmanPtc.h"
#include "rmanPtcDetail.h"

using namespace rmanPtcSop;

/*
int GR_rmanPtc::getWireMask( GU_Detail* gdp,
        const GR_DisplayOption *opt ) const
{
    rmanPtcSop::rmanPtcDetail *detail = dynamic_cast<rmanPtcSop::rmanPtcDetail*>(gdp);
if ( detail )
    return ~GEOPRIMALL;
else 
    return GEOPRIMALL;
}

int GR_rmanPtc::getShadedMask( GU_Detail* gdp,
    const GR_DisplayOption *opt ) const
{
    return getWireMask( gdp, opt );
}
*/

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

    if ( !detail->use_gl )
        return;

    glPushAttrib(GL_ALL_ATTRIB_BITS);

    if ( detail->redraw )
    {
        UT_Interrupt *boss = UTgetInterrupt();
        boss->opStart("Building rmanPtc gl pointcloud");

        // build display list
        glDeleteLists(mPtcId,1);
        mPtcId = glGenLists(1);
        glNewList( mPtcId, GL_COMPILE_AND_EXECUTE );

        srand(0);
        glBegin(GL_POINTS);
        std::vector<ptcVec>::const_iterator pos_it = detail->cachePoints.begin();
        std::vector<ptcVec>::const_iterator norm_it = detail->cacheNormals.begin();
        std::vector<float>::const_iterator rad_it = detail->cacheRadius.begin();
        std::vector<float>::const_iterator data_it = detail->cacheData.begin();
        float data[detail->datasize];
        
        for ( pos_it=detail->cachePoints.begin(); pos_it!=detail->cachePoints.end(); ++pos_it )
        {                    
            memcpy( data, &*data_it, sizeof(float)*detail->datasize );
            
            // only display a certain percentage (based on probability)
            if ( rand()/(float)RAND_MAX<=detail->display_probability )
            {
                if ( !detail->use_cull_bbox )
                {                    
                    glColor3f( data[0], data[1], data[2] );
                    glVertex3f( pos_it->val[0], pos_it->val[1], pos_it->val[2] );
                }
                else // we cull with a bbox
                {
                    UT_Vector3 pt( pos_it->val[0], pos_it->val[1], pos_it->val[2] );
                    if ( detail->cull_bbox.isInside( pt ) )
                    {
                        glColor3f( data[0], data[1], data[2] );
                        glVertex3f( pos_it->val[0], pos_it->val[1], pos_it->val[2] );
                    }
                }
            }
            norm_it++;
            rad_it++;
            data_it+=detail->datasize;
        }
        
        glEnd();
        
        glEndList();
        boss->opEnd();
    }
    
    // render display list
    glCallList( mPtcId );
    glPopAttrib();
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
