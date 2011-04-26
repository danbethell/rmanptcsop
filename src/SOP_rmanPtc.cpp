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
#include <UT/UT_DSOVersion.h>
#include <UT/UT_Math.h>
#include <UT/UT_Interrupt.h>
#include <GU/GU_Detail.h>
#include <GU/GU_PrimPoly.h>
#include <CH/CH_LocalVariable.h>
#include <PRM/PRM_Include.h>
#include <PRM/PRM_Parm.h>
#include <PRM/PRM_SpareData.h>
#include <OP/OP_Operator.h>
#include <OP/OP_OperatorTable.h>
#include <GU/GU_PrimPart.h>
#include <GEO/GEO_AttributeHandle.h>
#include <OBJ/OBJ_Camera.h>
#include <OP/OP_Director.h>

// c++ stuff
#include <sstream>

// rman point cloud
#include <pointcloud.h>

// local
#include "rmanPtcDetail.h"
#include "SOP_rmanPtc.h"
using namespace rmanPtcSop;

// our Sop table
void newSopOperator(OP_OperatorTable *table)
{
    table->addOperator(
	    new OP_Operator("rmanPtc",			// Internal name
			    "RmanPtc",			// UI name
			     SOP_rmanPtc::myConstructor,	// How to build the SOP
			     SOP_rmanPtc::myParameters,	// My parameters
			     0,				// Min # of sources
			     1,				// Max # of sources
			     SOP_rmanPtc::myVariables,	// Local variables
			     OP_FLAG_GENERATOR)		// Flag it as generator
	    );
}

// parameter names, ranges & defaults
static PRM_Name ptcFileName("ptcFile", "Ptc File");
static PRM_Name percentageName("perc", "Load %%%");
static PRM_Name boundOnLoadName("bboxload", "Bound On Load");
static PRM_Default boundOnLoadDefault(0.0);
static PRM_Name displayPercentageName("disp", "Display/Output %%%");
static PRM_Default percentageDefault( 100.0 );
static PRM_Range percentageRange( PRM_RANGE_UI, 0, PRM_RANGE_UI, 100 );
static PRM_Name channelName("chan", "Display Channel");
static PRM_ChoiceList channelMenu( PRM_CHOICELIST_SINGLE,
        &SOP_rmanPtc::buildChannelMenu );
static PRM_Name onlyOutputPreviewChannelName( "dispchanonly",
        "Output Display Channel Only" );
static PRM_Default onlyOutputPreviewChannelDefault( 1.0 );
static PRM_Name useDiskName("usedisk", "Display Disks");
static PRM_Default useDiskDefault( 0 );
static PRM_Name pointSizeName("pointsize", "Point/Disk Size");
static PRM_Default pointSizeDefault( 1.0 );
static PRM_Range pointSizeRange( PRM_RANGE_UI, 0, PRM_RANGE_UI, 10 );
static PRM_Name cameraPathName( "cullcamera", "Cull Camera" );
static PRM_Name nearFarDensityName( "nearfardensity", "Near/Far Density" );
static PRM_Default nearFarDensityDefault[2] = { PRM_Default( 1.0 ),
        PRM_Default(1.0) };
static PRM_Range nearFarDensityRange[2] = { PRM_Range(), PRM_Range() };
static PRM_Name sep1( "sep1", "Sep1" );
static PRM_Name sep2( "sep2", "Sep2" );
static PRM_Name sep3( "sep3", "Sep3" );

// our Sop parameters
PRM_Template SOP_rmanPtc::myParameters[] = {
    PRM_Template(PRM_FILE, 1, &ptcFileName),
    PRM_Template(PRM_TOGGLE, 1, &boundOnLoadName, &boundOnLoadDefault),
    PRM_Template(PRM_INT, 1, &percentageName, &percentageDefault, 0,
            &percentageRange),
    PRM_Template(PRM_SEPARATOR, 1, &sep1 ),
/*
  // camera culling still wip
    PRM_Template(PRM_STRING, PRM_TYPE_DYNAMIC_PATH, 1, &cameraPathName,
            0, 0, 0, 0, &PRM_SpareData::objCameraPath),
    PRM_Template(PRM_FLT, 2, &nearFarDensityName, &nearFarDensityDefault[0], 0,
            &nearFarDensityRange[0] ),
    PRM_Template(PRM_SEPARATOR, 1, &sep2 ),
*/
    PRM_Template(PRM_INT, 1, &displayPercentageName, &percentageDefault, 0,
            &percentageRange),
    PRM_Template(PRM_ORD, 1, &channelName, 0, &channelMenu ),
    PRM_Template(PRM_TOGGLE, 1, &onlyOutputPreviewChannelName,
            &onlyOutputPreviewChannelDefault ),
    PRM_Template(PRM_SEPARATOR, 1, &sep3),
    PRM_Template(PRM_FLT, 1, &pointSizeName, &pointSizeDefault, 0,
            &pointSizeRange ),
    PRM_Template(PRM_TOGGLE, 1, &useDiskName, &useDiskDefault ),
    PRM_Template()
};

// Here's how we define local variables and their integer mappings for the SOP.
enum {
	VAR_PTCFILE,	// Our ptc file
	VAR_PERCENTAGE,	// Percentage to draw
	VAR_BOUNDONLOAD, // Bound on load?
    VAR_DISPLAYPERCENTAGE, // cache points in memory
    VAR_CHANNEL, // display channel
    VAR_POINTSIZE, // point/disk size
    VAR_USEDISK, // preview as disks
    VAR_OUTPUTDISPLAYCHANNELONLY, // only output display channel into geo
    VAR_CULLCAMERA, // view frustum culling
    VAR_NEARFARDENSITY, // near/far clip-plane multiplier
};
CH_LocalVariable SOP_rmanPtc::myVariables[] = {
    { "PTCFILE",	VAR_PTCFILE, 0 },
    { "PERCENTAGE",	VAR_PERCENTAGE, 0 },
    { "BOUNDONLOAD", VAR_BOUNDONLOAD, 0},
    { "DISPLAYPERCENTAGE", VAR_DISPLAYPERCENTAGE, 0 },
    { "CHANNEL", VAR_CHANNEL, 0 },
    { "POINTSIZE", VAR_POINTSIZE, 0 },
    { "USEDISK", VAR_USEDISK, 0 },
    { "OUTPUTDISPLAYCHANNELONLY", VAR_OUTPUTDISPLAYCHANNELONLY, 0 },
    { "CULLCAMERA", VAR_CULLCAMERA, 0 },
    { "NEARFARDENSITY", VAR_NEARFARDENSITY, 0 },
    { 0, 0, 0 },
};

// static creator method
OP_Node *SOP_rmanPtc::myConstructor(OP_Network *net,
                                    const char *name,
                                    OP_Operator *op)
{
    return new SOP_rmanPtc(net, name, op);
}

// dynamically build our channel menu
void SOP_rmanPtc::buildChannelMenu( void *data, PRM_Name *theMenu,
        int theMaxSize, const PRM_SpareData *, PRM_Parm * )
{
    SOP_rmanPtc *me = reinterpret_cast<SOP_rmanPtc*>(data);
    if ( !me )
        return;

    unsigned int pos = 0;
    std::vector<std::string>::iterator it;
    for ( it=me->mChannelNames.begin(); it!=me->mChannelNames.end(); ++it )
    {
        std::string &name = *it;
        std::stringstream ss;
        ss << "chan" << pos;
        theMenu[pos].setToken( ss.str().c_str() );
        theMenu[pos].setLabel( name.c_str() );
        pos++;
    }
    if ( pos==0 )
    {
        theMenu[pos].setToken("na");
        theMenu[pos].setLabel("No Channels");
        pos++;        
    }
    theMenu[pos].setToken(0);
}

// ctor
SOP_rmanPtc::SOP_rmanPtc(OP_Network *net, const char *name, OP_Operator *op)
	: SOP_Node(net, name, op),
      mReload(false),
      mRedraw(false),
      mPtcFile("unknown"),
      mBoundOnLoad(false),
      mLoadPercentage(100),
      mDisplayPercentage(100),
      mPointSize(1.0),
      mUseDisk(0),
      mHasBBox(false),
      mOnlyOutputDisplayChannel(true),
      mDisplayChannel(0),
      mCullCamera(""),
      mNearDensity(1.f),
      mFarDensity(1.f)
{
}

// dtor
SOP_rmanPtc::~SOP_rmanPtc()
{
}

// decorate our input label
const char *SOP_rmanPtc::inputLabel( unsigned pos ) const
{
    return "Bounding Source";
}

// make the input a reference
int SOP_rmanPtc::isRefInput( unsigned pos ) const
{
    return 1;
}

// this is used to deallocate the provided GU_Detail object and reassign
// one of our GU_Detail-derived rmanPtcDetail instances
rmanPtcDetail *SOP_rmanPtc::allocateNewDetail()
{
    // get our gdp and replace it with a new rmanPtcDetailobject
    GU_DetailHandle &gd_handle = myGdpHandle;
    gd_handle.deleteGdp();
    rmanPtcSop::rmanPtcDetail *ptc_gdp = new rmanPtcSop::rmanPtcDetail;
    gd_handle.allocateAndSet( ptc_gdp );
    return ptc_gdp;
}

// the bit that does all the work
OP_ERROR SOP_rmanPtc::cookMySop(OP_Context &context)
{
    // get some useful bits & bobs
    UT_Interrupt *boss;
    float now = context.myTime;
    UT_String ptcFile = getPtcFile(now);
    int loadPercentage = getLoadPercentage(now);
    int displayPercentage = getDisplayPercentage(now);
    float pointSize = getPointSize(now);
    int useDisk = getUseDisk(now);
    int boundOnLoad = getBoundOnLoad(now);
    int displayChannel = getDisplayChannel(now);
    int onlyOutputDisplayChannel = getOnlyOutputDisplayChannel(now);
    /*
      float nearDensity = getNearDensity(now);
      float farDensity = getFarDensity(now);
      UT_String cullCamera = getCullCamera(now);
    */

    // lock out inputs
    if ( lockInputs(context) >= UT_ERROR_ABORT)
        error();

    // Check to see that there hasn't been a critical error in cooking the SOP.
    if (error() < UT_ERROR_ABORT)
    {
        boss = UTgetInterrupt();

        // here we make sure our detail is an instance of rmanPtcDetail
        rmanPtcDetail *ptc_gdp = dynamic_cast<rmanPtcSop::rmanPtcDetail *>(gdp);
        if ( !ptc_gdp )
            ptc_gdp = allocateNewDetail();

        // clear our gdp
        ptc_gdp->clearAndDestroy();

        // start our work
        boss->opStart("Loading point cloud");

        // get our bbox
        bool has_bbox = false;
        const GU_Detail *input_geo = inputGeo( 0, context );
        updateBBox( input_geo );

        // pass information to our detail
        ptc_gdp->point_size = pointSize;
        ptc_gdp->use_disk = useDisk;
        ptc_gdp->cull_bbox = mBBox;
        ptc_gdp->use_cull_bbox = (mHasBBox&&(!mBoundOnLoad))?true:false;
        ptc_gdp->display_probability = displayPercentage/100.f;
        ptc_gdp->display_channel = displayChannel;
        if ( onlyOutputDisplayChannel )
            ptc_gdp->display_channel = 0;

        // here we load our ptc
        if ( mReload )
        {            
            // clear everything
            mChannelNames.clear();
            cachePoints.clear();
            cacheNormals.clear();
            cacheRadius.clear();
            cacheData.clear();
            mRedraw = true;
            
            // open the point cloud
            PtcPointCloud ptc = PtcSafeOpenPointCloudFile(
                    const_cast<char*>(ptcFile.buffer()) );
            if ( !ptc )
            {
                UT_String msg( "Unable to open input file: " );
                msg += ptcFile;
                addError( SOP_MESSAGE, msg);
                boss->opEnd();
                return error();
            }

            // get some information from the ptc
            ptc_gdp->path = std::string(ptcFile.fileName());
            char **vartypes, **varnames;
            PtcGetPointCloudInfo( ptc, const_cast<char*>("npoints"),
                    &ptc_gdp->nPoints );
            PtcGetPointCloudInfo( ptc, const_cast<char*>("npointvars"),
                    &ptc_gdp->nChannels );
            PtcGetPointCloudInfo( ptc, const_cast<char*>("pointvartypes"),
                    &vartypes );
            PtcGetPointCloudInfo( ptc, const_cast<char*>("pointvarnames"),
                    &varnames );
            PtcGetPointCloudInfo( ptc, const_cast<char*>("datasize"),
                    &ptc_gdp->datasize );
            PtcGetPointCloudInfo( ptc, const_cast<char*>("bbox"),
                    ptc_gdp->bbox );

            // process our channel names
            ptc_gdp->types.clear();
            ptc_gdp->names.clear();
            for ( unsigned int i=0; i<ptc_gdp->nChannels; ++i )
            {
                ptc_gdp->types.push_back( std::string(vartypes[i]) );
                ptc_gdp->names.push_back( std::string(varnames[i]) );
                std::string name = ptc_gdp->types[i] + " " + ptc_gdp->names[i];
                mChannelNames.push_back( name );
            }

            // what percentage of points should we load?
            float load_probability = loadPercentage/100.f;

            // load points into memory
            float point[3], normal[3];
            float radius;
            float data[ptc_gdp->datasize];
            srand(0);
            for ( unsigned int i=0; i<ptc_gdp->nPoints; ++i )
            {
                PtcReadDataPoint( ptc, point, normal, &radius, data );

                // bound on load
                if ( boundOnLoad )
                {
                    UT_Vector3 pt( point[0], point[1], point[2] );
                    if ( !mBBox.isInside(pt) )
                        continue;
                }

                // discard a percentage of our points
                if ( rand()/(float)RAND_MAX>load_probability )
                    continue;

                // put points into our cache
                cachePoints.push_back( point );
                cacheNormals.push_back( normal );
                cacheRadius.push_back( radius );
                for ( unsigned int j=0; j<ptc_gdp->datasize; ++j )
                    cacheData.push_back( data[j] );

                // break for the interrupt handler (i.e. press ESC)
                if ( boss->opInterrupt() )
                    break;
            }
            ptc_gdp->nLoaded = cachePoints.size();

            // mark our detail as valid and close our ptc
            PtcClosePointCloudFile( ptc );
            mReload = false;

            // force update on channel parameter
            getParm("chan").revertToDefaults(now);
        }

        // build a new primitive
        GU_PrimParticle::build( ptc_gdp, cachePoints.size(), 0 );

        // create our output geometry using the output % parameter
        // this is the same variable as GR_ uses to preview
        std::vector<UT_Vector3>::const_iterator pos_it = cachePoints.begin();
        std::vector<UT_Vector3>::const_iterator norm_it = cacheNormals.begin();
        std::vector<float>::const_iterator rad_it = cacheRadius.begin();
        std::vector<float>::const_iterator data_it = cacheData.begin();

        // add some standard attributes
        GB_AttributeRef n_attrib = ptc_gdp->addPointAttrib( "N", sizeof(UT_Vector3),
                GB_ATTRIB_VECTOR, 0 );
        GB_AttributeRef r_attrib = ptc_gdp->addPointAttrib( "radius", sizeof(float),
                GB_ATTRIB_FLOAT, 0 );
        ptc_gdp->N_attrib = n_attrib;
        ptc_gdp->R_attrib = r_attrib;

        // process the rest of our data attributes
        std::vector<GB_AttribType> data_types;
        std::vector<GB_AttributeRef> data_attribs;
        std::vector<int> data_size, data_offset;
        int offset_total = 0;
        ptc_gdp->attributes.clear();
        ptc_gdp->attribute_size.clear();
        for ( unsigned int i=0; i<ptc_gdp->nChannels; ++i )
        {
            GB_AttribType type = GB_ATTRIB_FLOAT; // float, vector
            int size = 1;
            if ( ptc_gdp->types[i] == "point" ||
                    ptc_gdp->types[i] == "vector" ||
                    ptc_gdp->types[i] == "normal" )
            {
                type = GB_ATTRIB_VECTOR;
                size = 3;
            }
            if ( ptc_gdp->types[i] == "color" )
            {
                size=3;
            }
            if ( ptc_gdp->types[i] == "matrix" )
            {
                size=16;
            }
            offset_total += size;

            data_types.push_back( type );
            data_size.push_back( size );
            data_offset.push_back( offset_total );

            if ( onlyOutputDisplayChannel )
            {
                if ( displayChannel==i )
                {
                    GB_AttributeRef attrib = ptc_gdp->addPointAttrib(
                            ptc_gdp->names[i].c_str(), sizeof(float)*size,
                            type, 0 );
                    ptc_gdp->attributes.push_back( attrib );
                    ptc_gdp->attribute_size.push_back( size );
                    data_attribs.push_back( attrib );
                }
            }
            else
            {
                GB_AttributeRef attrib = ptc_gdp->addPointAttrib( ptc_gdp->names[i].c_str(),
                        sizeof(float)*size, type, 0 );
                ptc_gdp->attributes.push_back( attrib );
                ptc_gdp->attribute_size.push_back( size );
                data_attribs.push_back( attrib );
            }
        }
        cacheDataOffsets = data_offset;

/*
        // cull camera
        bool use_cull_cam = false;
        UT_Vector3 cam_pos(0,0,0);
        float cam_near=0.0, cam_far=0.0;

        if ( cullCamera!="" )
        {
            OBJ_Node *cam_node = OPgetDirector()->findOBJNode( cullCamera );
            if ( cam_node )
            {
                OBJ_Camera *cam = cam_node->castToOBJCamera();
                if ( cam )
                {
                    UT_DMatrix4 mtx;
                    cam->getWorldTransform( mtx, context );
                    std::cerr << "mtx: " << mtx << std::endl;
                    cam_pos *= mtx;
                    std::cerr << "pos: " << cam_pos << std::endl;
                    use_cull_cam = true;
                    cam_near = cam->getNEAR(now);
                    cam_far = cam->getFAR(now);
                    std::cerr << "near: " << cam_near << std::endl;
                    std::cerr << "far: " << cam_far << std::endl;

                    mRedraw = true;
                }
            }
            std::cerr << "cull camera: " << cullCamera << std::endl;
            std::cerr << nearDensity << ", " << farDensity << std::endl;
        }
*/
        // add data from our cached points to geometry
        // based on display/output probability
        srand(0);
        float density_mult = 1.f;
        while( pos_it!=cachePoints.end() )
        {

/*            
            if ( use_cull_cam )
            {
                float dist = ((*pos_it)-cam_pos).length();
                if ( dist<cam_near )
                    density_mult = nearDensity;
                else if ( dist>cam_far )
                    density_mult = farDensity;
                else
                {
                    float normalize_dist =( dist - cam_near ) / ( cam_far - cam_near );
                    density_mult = 1.f - normalize_dist;
                }
            }
*/

            if ( rand()/(float)RAND_MAX <
                    ptc_gdp->display_probability*density_mult )
            {
                if ( (!ptc_gdp->use_cull_bbox) ||
                        (ptc_gdp->use_cull_bbox &&
                                ptc_gdp->cull_bbox.isInside( *pos_it ) ) )
                {
                    // add to our SOP geometry
                    GEO_Point *pt = ptc_gdp->appendPoint();
                    pt->setPos( *pos_it );
                    (*pt->castAttribData<UT_Vector3>(n_attrib)) = *norm_it;
                    (*pt->castAttribData<float>(r_attrib)) = *rad_it;
                    const float *data = &*data_it;
                    for ( unsigned int i=0; i<data_types.size(); ++i )
                    {
                        if ( onlyOutputDisplayChannel )
                        {
                            if ( i==displayChannel )
                            {
                                pt->set( data_attribs[0], data, data_size[i] );
                            }
                            data += data_size[i];
                        }
                        else
                        {
                            pt->set( data_attribs[i], data, data_size[i] );
                            data += data_size[i];
                        }
                    }
                }
            }

            // increment our interators
            pos_it++;
            norm_it++;
            rad_it++;
            data_it+=ptc_gdp->datasize;
        }

        // delete our particle primitive
        ptc_gdp->deletePrimitive(0,0);
        
        // info in sop's message area
        std::stringstream ss;
        ss << "Name: " << ptc_gdp->path << std::endl;
        ss << "Points: " << ptc_gdp->nPoints << " - [ " <<
                ptc_gdp->nLoaded << " loaded ]" << std::endl;
        ss << "Channels: " << ptc_gdp->nChannels << std::endl;
        for ( unsigned int i=0; i<ptc_gdp->nChannels; ++i )
            ss << "  " << i << ": " << mChannelNames[i] << std::endl;
        addMessage( SOP_MESSAGE, ss.str().c_str() );

        // Tell the interrupt server that we've completed
        boss->opEnd();

        // force update?
        ptc_gdp->redraw = mRedraw;
        mRedraw = false;
    }

    // tidy up & go home
    unlockInputs();
    return error();
}
