// houdini
#include <UT/UT_DSOVersion.h>
#include <UT/UT_Math.h>
#include <UT/UT_Interrupt.h>
#include <GU/GU_Detail.h>
#include <GU/GU_PrimPoly.h>
#include <CH/CH_LocalVariable.h>
#include <PRM/PRM_Include.h>
#include <PRM/PRM_Parm.h>
#include <OP/OP_Operator.h>
#include <OP/OP_OperatorTable.h>
#include <GU/GU_PrimPart.h>
#include <GEO/GEO_AttributeHandle.h>

// c++ stuff
#include <sstream>

// rman point cloud
#include <pointcloud.h>

// local
#include "rmanPtcDetail.h"
#include "SOP_rmanPtc.h"

using namespace rmanPtcSop;

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

static PRM_Name ptcFileName("ptcFile", "Ptc File");
static PRM_Name percentageName("perc", "Load %%%");
static PRM_Name boundOnLoadName("bboxload", "Bound On Load");
static PRM_Default boundOnLoadDefault(0.0);
static PRM_Name displayPercentageName("disp", "Output %%%");
static PRM_Default percentageDefault( 100.0 );
static PRM_Range percentageRange( PRM_RANGE_UI, 0, PRM_RANGE_UI, 100 );
static PRM_Name previewName("gl", "OpenGL Preview");
static PRM_Default previewDefault( 1.0 );
static PRM_Name pointSizeName("pointsize", "Point/Disk Size");
static PRM_Default pointSizeDefault( 1.0 );
static PRM_Range pointSizeRange( PRM_RANGE_UI, 0, PRM_RANGE_UI, 10 );
static PRM_Name useDiskName("usedisk", "Preview Disks");
static PRM_Default useDiskDefault( 0 );
static PRM_Name channelName("chan", "Preview Channel");
static PRM_ChoiceList channelMenu( PRM_CHOICELIST_SINGLE, &SOP_rmanPtc::buildChannelMenu );

static PRM_Name sep1("sep1", "Sep1");
static PRM_Name sep2("sep2", "Sep2");

PRM_Template SOP_rmanPtc::myParameters[] = {
    PRM_Template(PRM_FILE, 1, &ptcFileName),
    PRM_Template(PRM_TOGGLE, 1, &boundOnLoadName, &boundOnLoadDefault),
    PRM_Template(PRM_INT, 1, &percentageName, &percentageDefault, 0, &percentageRange),
    PRM_Template(PRM_SEPARATOR, 1, &sep1),
    PRM_Template(PRM_INT, 1, &displayPercentageName, &percentageDefault, 0, &percentageRange),
    PRM_Template(PRM_SEPARATOR, 1, &sep2),
    PRM_Template(PRM_TOGGLE, 1, &previewName, &previewDefault ),
    PRM_Template(PRM_ORD, 1, &channelName, 0, &channelMenu ),
    PRM_Template(PRM_FLT, 1, &pointSizeName, &pointSizeDefault, 0, &pointSizeRange ),
    PRM_Template(PRM_TOGGLE, 1, &useDiskName, &useDiskDefault ),
    PRM_Template()
};

// Here's how we define local variables for the SOP.
enum {
	VAR_PTCFILE,	// Our ptc file
	VAR_PERCENTAGE,	// Percentage to draw
	VAR_BOUNDONLOAD, // Bound on load?
    VAR_DISPLAYPERCENTAGE, // cache points in memory
    VAR_GLPREVIEW, // use gl to preview values
    VAR_CHANNEL, // display channel
    VAR_POINTSIZE,
    VAR_USEDISK,
};

CH_LocalVariable SOP_rmanPtc::myVariables[] = {
    { "PTCFILE",	VAR_PTCFILE, 0 },		// The table provides a mapping
    { "PERCENTAGE",	VAR_PERCENTAGE, 0 },    // from text string to integer token
    { "BOUNDONLOAD", VAR_BOUNDONLOAD, 0},
    { "DISPLAYPERCENTAGE", VAR_DISPLAYPERCENTAGE, 0 },    // from text string to integer token
    { "GLPREVIEW", VAR_GLPREVIEW, 0 },
    { "CHANNEL", VAR_CHANNEL, 0 },
    { "POINTSIZE", VAR_POINTSIZE, 0 },
    { "USEDISK", VAR_USEDISK, 0 },
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
void SOP_rmanPtc::buildChannelMenu( void *data, PRM_Name *theMenu, int theMaxSize, const PRM_SpareData *, PRM_Parm * )
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
      mGlPreview(1),
      mPointSize(1.0),
      mUseDisk(0),
      mHasBBox(false)
{
}

// dtor
SOP_rmanPtc::~SOP_rmanPtc()
{
}

const char *SOP_rmanPtc::inputLabel( unsigned pos ) const
{
    return "Bounding Source";
}

int SOP_rmanPtc::isRefInput( unsigned pos ) const
{
    return 1;
}

rmanPtcDetail *SOP_rmanPtc::allocateNewDetail()
{
    // get our gdp and replace it with a new rmanPtcDetailobject
    GU_DetailHandle &gd_handle = myGdpHandle;
    gd_handle.deleteGdp();
    rmanPtcSop::rmanPtcDetail *ptc_gdp = new rmanPtcSop::rmanPtcDetail;
    gd_handle.allocateAndSet( ptc_gdp );
    return ptc_gdp;
}

// the guts of the Sop
OP_ERROR SOP_rmanPtc::cookMySop(OP_Context &context)
{
    // get parameters
    UT_Interrupt *boss;
    float now = context.myTime;
    UT_String ptcFile = getPtcFile(now);
    int loadPercentage = getLoadPercentage(now);
    int displayPercentage = getDisplayPercentage(now);
    int glPreview = getGlPreview(now);
    float pointSize = getPointSize(now);
    int useDisk = getUseDisk(now);
    int boundOnLoad = getBoundOnLoad(now);

    // lock inputs
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
        boss->opStart("Building rmanPtc output");

        // get our bbox
        bool has_bbox = false;
        const GU_Detail *input_geo = inputGeo( 0, context );
        updateBBox( input_geo );

        // pass information to our detail
        ptc_gdp->use_gl = glPreview;
        ptc_gdp->point_size = pointSize;
        ptc_gdp->use_disk = useDisk;
        ptc_gdp->cull_bbox = mBBox;
        ptc_gdp->use_cull_bbox = (mHasBBox&&(!mBoundOnLoad))?true:false;
        ptc_gdp->display_probability = displayPercentage/100.f;

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
            PtcPointCloud ptc = PtcSafeOpenPointCloudFile( const_cast<char*>(ptcFile.buffer()) );
            if ( !ptc )
            {
                UT_String msg( "Unable to open input file: " );
                msg += ptcFile;
                addError( SOP_MESSAGE, msg);
                boss->opEnd();
                return error();
            }

            // get some information
            ptc_gdp->path = std::string(ptcFile.fileName());
            char **vartypes, **varnames;
            PtcGetPointCloudInfo( ptc, const_cast<char*>("npoints"), &ptc_gdp->nPoints );
            PtcGetPointCloudInfo( ptc, const_cast<char*>("npointvars"), &ptc_gdp->nChannels );
            PtcGetPointCloudInfo( ptc, const_cast<char*>("pointvartypes"), &vartypes );
            PtcGetPointCloudInfo( ptc, const_cast<char*>("pointvarnames"), &varnames );
            PtcGetPointCloudInfo( ptc, const_cast<char*>("datasize"), &ptc_gdp->datasize );
            PtcGetPointCloudInfo( ptc, const_cast<char*>("bbox"), ptc_gdp->bbox );

            // our channel names
            ptc_gdp->types.clear();
            ptc_gdp->names.clear();
            for ( unsigned int i=0; i<ptc_gdp->nChannels; ++i )
            {
                ptc_gdp->types.push_back( std::string(vartypes[i]) );
                ptc_gdp->names.push_back( std::string(varnames[i]) );
                std::string name = ptc_gdp->types[i] + " " + ptc_gdp->names[i];
                mChannelNames.push_back( name );
            }

            // what percentage of points to load?
            float load_probability = loadPercentage/100.f;

            // load a percentage of points into memory
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

        // create our output geometry using the output % parameter (same as GR_ uses to preview) 
        std::vector<UT_Vector3>::const_iterator pos_it = cachePoints.begin();
        std::vector<UT_Vector3>::const_iterator norm_it = cacheNormals.begin();
        std::vector<float>::const_iterator rad_it = cacheRadius.begin();
        std::vector<float>::const_iterator data_it = cacheData.begin();

        // add some attributes
        int n_attrib = ptc_gdp->addPointAttrib( "N", sizeof(UT_Vector3), GB_ATTRIB_VECTOR, 0 );
        int r_attrib = ptc_gdp->addPointAttrib( "radius", sizeof(float), GB_ATTRIB_FLOAT, 0 );

        // our data attributes
        std::vector<GB_AttribType> data_types;
        std::vector<int> data_size;
        std::vector<int> data_attribs;
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
            data_attribs.push_back(
                    ptc_gdp->addPointAttrib( ptc_gdp->names[i].c_str(),
                            sizeof(float)*size, type, 0 ) );
            data_types.push_back( type );
            data_size.push_back( size );
        }

        // add data from our cache, based on display/output probability
        srand(0);
        unsigned int pos = 0;
        while( pos_it!=cachePoints.end() )
        {
            if ( rand()/(float)RAND_MAX<=ptc_gdp->display_probability )
            {
                if ( !ptc_gdp->use_cull_bbox )
                {                   
                    // add to our SOP geometry
                    GEO_Point *pt = ptc_gdp->appendPoint();
                    pt->setPos( *pos_it );
                    (*pt->castAttribData<UT_Vector3>(n_attrib)) = *norm_it;
                    (*pt->castAttribData<float>(r_attrib)) = *rad_it;
                    const float *data = &*data_it;
                    for ( unsigned int i=0; i<data_types.size(); ++i )
                    {
                        float *ptr = pt->castAttribData<float>(data_attribs[i]);
                        memcpy( (void*)ptr, (void*)data,
                                    sizeof(float)*data_size[i] );
                        data += data_size[i];
                    }
                }
                else // we cull with our bbox
                {
                    if ( ptc_gdp->cull_bbox.isInside( *pos_it ) )
                    {
                        GEO_Point *pt = ptc_gdp->appendPoint();
                        pt->setPos( *pos_it );
                        (*pt->castAttribData<UT_Vector3>(n_attrib)) = *norm_it;
                        (*pt->castAttribData<float>(r_attrib)) = *rad_it;
                        const float *data = &*data_it;
                        for ( unsigned int i=0; i<data_types.size(); ++i )
                        {
                            float *ptr = pt->castAttribData<float>(data_attribs[i]);
                            memcpy( (void*)ptr, (void*)data,
                                        sizeof(float)*data_size[i] );
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
            pos++;
        }

        // delete our particle primitive
        ptc_gdp->deletePrimitive(0,0);
        
        // info in sop's message area
        std::stringstream ss;
        ss << "Name: " << ptc_gdp->path << std::endl;
        ss << "Points: " << ptc_gdp->nPoints << " - [ " << ptc_gdp->nLoaded << " loaded ]" << std::endl;
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
