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
static PRM_Name displayPercentageName("disp", "Output %%%");
static PRM_Default percentageDefault( 100.0 );
static PRM_Range percentageRange( PRM_RANGE_UI, 0, PRM_RANGE_UI, 100 );
static PRM_Name previewName("gl", "OpenGL Preview");
static PRM_Default previewDefault( 1.0 );
static PRM_Name channelName("chan", "Preview Channel");
static PRM_ChoiceList channelMenu( PRM_CHOICELIST_SINGLE, &SOP_rmanPtc::buildChannelMenu );

PRM_Template SOP_rmanPtc::myParameters[] = {
    PRM_Template(PRM_FILE, 1, &ptcFileName),
    PRM_Template(PRM_INT, 1, &percentageName, &percentageDefault, 0, &percentageRange),
    PRM_Template(PRM_INT, 1, &displayPercentageName, &percentageDefault, 0, &percentageRange),
    PRM_Template(PRM_SEPARATOR),
    PRM_Template(PRM_TOGGLE, 1, &previewName, &previewDefault ),
    PRM_Template(PRM_ORD, 1, &channelName, 0, &channelMenu ),
    PRM_Template()
};

// Here's how we define local variables for the SOP.
enum {
	VAR_PTCFILE,	// Our ptc file
	VAR_PERCENTAGE,	// Percentage to draw
    VAR_DISPLAYPERCENTAGE, // cache points in memory
    VAR_GLPREVIEW, // use gl to preview values
    VAR_CHANNEL, // display channel
};

CH_LocalVariable SOP_rmanPtc::myVariables[] = {
    { "PTCFILE",	VAR_PTCFILE, 0 },		// The table provides a mapping
    { "PERCENTAGE",	VAR_PERCENTAGE, 0 },    // from text string to integer token
    { "DISPLAYPERCENTAGE", VAR_DISPLAYPERCENTAGE, 0 },    // from text string to integer token
    { "GLPREVIEW", VAR_GLPREVIEW, 0 },
    { "CHANNEL", VAR_CHANNEL, 0 },
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
      mPtcFile(""),
      mLoadPercentage(100),
      mDisplayPercentage(100)
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

        // are we going to draw using openGl?
        ptc_gdp->use_gl = glPreview;

        // get our bbox
        const GU_Detail *input_geo = inputGeo( 0, context );
        if ( input_geo )
        {
            input_geo->getBBox( &ptc_gdp->cull_bbox );
            ptc_gdp->use_cull_bbox = true;
            mRedraw = true; // this should be cached so only true when bbox changes
        }
        else
        {
            if ( ptc_gdp->use_cull_bbox )
            {
                ptc_gdp->use_cull_bbox = false;
                mRedraw = true;
            }
        }

        // what percentage of points to display?
        ptc_gdp->display_probability = displayPercentage/100.f;

        // here we load our ptc
        if ( mReload )
        {            
            // clear everything
            mChannelNames.clear();
            ptc_gdp->cachePoints.clear();
            ptc_gdp->cacheNormals.clear();
            ptc_gdp->cacheRadius.clear();
            ptc_gdp->cacheData.clear();
            mRedraw = true;
            
            if ( mPtcFile!="" )
            {
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
                PtcGetPointCloudInfo( ptc, const_cast<char*>("npoints"), &ptc_gdp->nPoints );
                PtcGetPointCloudInfo( ptc, const_cast<char*>("npointvars"), &ptc_gdp->nChannels );
                PtcGetPointCloudInfo( ptc, const_cast<char*>("pointvartypes"), &ptc_gdp->vartypes );
                PtcGetPointCloudInfo( ptc, const_cast<char*>("pointvarnames"), &ptc_gdp->varnames );
                PtcGetPointCloudInfo( ptc, const_cast<char*>("datasize"), &ptc_gdp->datasize );
                PtcGetPointCloudInfo( ptc, const_cast<char*>("bbox"), ptc_gdp->bbox );

                // our channel names
                for ( unsigned int i=0; i<ptc_gdp->nChannels; ++i )
                {
                    std::string name( std::string(ptc_gdp->vartypes[i]) + std::string(" ") + std::string(ptc_gdp->varnames[i]) );
                    mChannelNames.push_back( name );
                }

                // what percentage of points to load?
                float load_probability = loadPercentage/100.f;

                // load a percentage of points into memory
                ptcVec point, normal;
                float radius;
                float data[ptc_gdp->datasize];            
                srand(0);
                for ( unsigned int i=0; i<ptc_gdp->nPoints; ++i )
                {
                    PtcReadDataPoint( ptc, point.val, normal.val, &radius, data );
                    
                    // discard a percentage of our points
                    if ( rand()/(float)RAND_MAX>load_probability )
                        continue;
                    
                    ptc_gdp->cachePoints.push_back( point );
                    ptc_gdp->cacheNormals.push_back( normal );
                    ptc_gdp->cacheRadius.push_back( radius );
                    for ( unsigned int j=0; j<ptc_gdp->datasize; ++j )
                        ptc_gdp->cacheData.push_back( data[j] );

                    if ( boss->opInterrupt() )
                        break;
                }
                ptc_gdp->nLoaded = ptc_gdp->cachePoints.size();

                // mark our detail as valid and close our ptc
                PtcClosePointCloudFile( ptc );
                mReload = false;
            }

            // force update on channel parameter
            getParm("chan").revertToDefaults(now);
        }

        // create our output geometry using the output % parameter (same as GR_ uses to preview) 
        std::vector<ptcVec>::const_iterator pos_it = ptc_gdp->cachePoints.begin();   
        GU_PrimParticle::build( ptc_gdp, ptc_gdp->cachePoints.size(), 0 );
        srand(0);
        for ( pos_it=ptc_gdp->cachePoints.begin(); pos_it!=ptc_gdp->cachePoints.end(); ++pos_it )
        {               
            if ( rand()/(float)RAND_MAX<=ptc_gdp->display_probability )
            {
                if ( !ptc_gdp->use_cull_bbox )
                {                   
                    // add to our SOP geometry
                    GEO_Point *pt = ptc_gdp->appendPoint();
                    pt->setPos( UT_Vector3( pos_it->val[0], pos_it->val[1], pos_it->val[2] ) ); 
                }
                else // we cull with a bbox
                {
                    UT_Vector3 pt_pos( pos_it->val[0], pos_it->val[1], pos_it->val[2] );
                    if ( ptc_gdp->cull_bbox.isInside( pt_pos ) )
                    {
                        GEO_Point *pt = ptc_gdp->appendPoint();
                        pt->setPos( pt_pos );
                    }
                }

            }
        }
        ptc_gdp->deletePrimitive(0,0);
        
        // info in sop's message area
        std::stringstream ss;
        ss << "Name: " << ptc_gdp->path << std::endl;
        ss << "Points: " << ptc_gdp->nPoints << " - [ " << ptc_gdp->nLoaded << " loaded ]" << std::endl;
        ss << "Channels: " << ptc_gdp->nChannels;
        addMessage( SOP_MESSAGE, ss.str().c_str() );

        // Select our geometry
        //select(GU_SPrimitive);

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

/*
float SOP_rmanPtc::getVariableValue(int index, int thread)
{
    switch (index)
    {
    case VAR_PERCENTAGE:   
        return mPercentage;
    }

    return SOP_Node::getVariableValue(index, thread);
}
*/
