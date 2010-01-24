#ifndef RMANPTC_H_
#define RMANPTC_H_

#include <vector>
#include <SOP/SOP_Node.h>
#include <PRM/PRM_Name.h>

namespace rmanPtcSop
{
    class SOP_rmanPtc : public SOP_Node
    {
        public:
            static OP_Node *myConstructor( OP_Network *net,
                                            const char *name,
                                            OP_Operator *op );
            static PRM_Template		 myParameters[];
            static CH_LocalVariable	 myVariables[];
            static void buildChannelMenu( void *data, PRM_Name *theMenu, int theMaxSize, const PRM_SpareData *, PRM_Parm * );

            std::vector<std::string> mChannelNames; // channel names
        protected:
            // ctor/dtor
            SOP_rmanPtc( OP_Network *net, const char *name, OP_Operator *op );
            virtual ~SOP_rmanPtc();

            // inputs
            virtual const char *inputLabel( unsigned pos ) const;
            virtual int isRefInput( unsigned pos ) const;
 
            // allocate our new detail object
            rmanPtcDetail *allocateNewDetail();

            // cook the sop!
            virtual OP_ERROR cookMySop( OP_Context &context );


            //virtual float getVariableValue( int index, int thread );

            /// Disable parameters according to other parameters.
            //virtual unsigned disableParms();

        private:
            UT_String getPtcFile(float t)
            {
                std::string old(mPtcFile.buffer());
                evalString( mPtcFile, "ptcFile", 0, t );
                if ( std::string(mPtcFile.buffer())!=old )
                    mReload = true;
                return mPtcFile;
            }
            int getLoadPercentage(float t)
            {
                int old = mLoadPercentage;
                mLoadPercentage = evalInt( "perc", 0, t );
                if ( mLoadPercentage!=old )
                    mReload = true;
                return mLoadPercentage;
            }
            int getDisplayPercentage(float t)
            {
                int old = mDisplayPercentage;
                mDisplayPercentage = evalInt( "disp", 0, t );
                if ( mDisplayPercentage!=old )
                    mRedraw = true;
                return mDisplayPercentage;
            }
            int getGlPreview(float t)
            {
                int old = mGlPreview;
                mGlPreview = evalInt( "gl", 0, t );
                if ( mGlPreview!=old )
                    mRedraw = true;
                return mGlPreview;
            }

            bool mReload, mRedraw; // do we need to reload/redraw the ptc?
            UT_String mPtcFile; // our ptc file
            int mLoadPercentage; // % of points to load into memory
            int mDisplayPercentage; // % of points to display
            int mGlPreview; // use gl preview
    };
}

#endif
