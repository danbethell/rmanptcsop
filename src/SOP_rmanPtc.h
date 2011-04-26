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

#ifndef RMANPTC_H_
#define RMANPTC_H_

#include <SOP/SOP_Node.h>
#include <PRM/PRM_Name.h>
#include <vector>

namespace rmanPtcSop
{
    /*! \class SOP_rmanPtc
     * \brief Our SOP for loading & previewing RenderMan point clouds.
     *
     * This class inherits from SOP_node and provides parameters for controlling
     * the loading & previewing of our point cloud.
     */
    class SOP_rmanPtc : public SOP_Node
    {
        public:
            // standard houdini creator and parameter variables
            static OP_Node *myConstructor( OP_Network *net,
                                            const char *name,
                                            OP_Operator *op );
            static PRM_Template		 myParameters[];
            static CH_LocalVariable	 myVariables[];

            // used to dynamically build the output channel menu
            static void buildChannelMenu( void *data, PRM_Name *theMenu,
                    int theMaxSize, const PRM_SpareData *, PRM_Parm * );

        protected:
            // ctor/dtor
            SOP_rmanPtc( OP_Network *net, const char *name, OP_Operator *op );
            virtual ~SOP_rmanPtc();

            // cook the sop!
            virtual OP_ERROR cookMySop( OP_Context &context );

            // decorate our input correctly
            virtual const char *inputLabel( unsigned pos ) const;
            virtual int isRefInput( unsigned pos ) const;

        private:
            // allocate our new detail object
            rmanPtcDetail *allocateNewDetail();

            // access methods which cache parameter values
            UT_String getPtcFile(float t)
            {
                std::string old(mPtcFile.buffer());
                evalString( mPtcFile, "ptcFile", 0, t );
                if ( std::string(mPtcFile.buffer())!=old )
                    mReload = true;
                return mPtcFile;
            }
            UT_String getCullCamera(float t)
            {
                std::string old(mCullCamera.buffer());
                evalString( mCullCamera, "cullcamera", 0, t );
                if ( std::string(mCullCamera.buffer())!=old )
                {
                    mRedraw = true;
                    if ( mBoundOnLoad )
                        mReload = true;
                }
                return mCullCamera;
            }
            int getBoundOnLoad(float t)
            {
                int old = mBoundOnLoad;
                mBoundOnLoad = evalInt( "bboxload", 0, t );
                if ( mBoundOnLoad!=old )
                    mReload = true;
                return mBoundOnLoad;
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
            float getPointSize(float t)
            {
                float old = mPointSize;
                mPointSize = evalFloat( "pointsize", 0, t );
                if ( mPointSize!=old )
                    mRedraw = true;
                return mPointSize;
            }
            float getNearDensity(float t)
            {
                float old = mNearDensity;
                mNearDensity = evalFloat( "nearfardensity", 0, t );
                if ( mNearDensity!=old )
                    mRedraw = true;
                return mNearDensity;
            }
            float getFarDensity(float t)
            {
                float old = mFarDensity;
                mFarDensity = evalFloat( "nearfardensity", 1, t );
                if ( mFarDensity!=old )
                    mRedraw = true;
                return mFarDensity;
            }
            int getUseDisk(float t)
            {
                int old = mUseDisk;
                mUseDisk = evalInt( "usedisk", 0, t );
                if ( mUseDisk!=old )
                    mRedraw = true;
                return mUseDisk;
            }
            int getDisplayChannel(float t)
            {
                int old = mDisplayChannel;
                mDisplayChannel = evalInt( "chan", 0, t );
                if ( mDisplayChannel!=old )
                    mRedraw = true;
                return mDisplayChannel;
            }
            int getOnlyOutputDisplayChannel(float t)
            {
                int old = mOnlyOutputDisplayChannel;
                mOnlyOutputDisplayChannel = evalInt( "dispchanonly", 0, t );
                if ( mOnlyOutputDisplayChannel!=old )
                    mRedraw = true;
                return mOnlyOutputDisplayChannel;
            }
            void updateBBox( const GU_Detail *input )
            {
                if (!input)
                {
                    if ( mHasBBox )
                    {
                        mRedraw = true;
                        if ( mBoundOnLoad )
                            mReload = true;
                    }
                    mHasBBox = false;
                }
                else
                {
                    UT_BoundingBox old = mBBox;
                    input->getBBox( &mBBox );
                    if ( (!mHasBBox) || mBBox!=old )
                    {
                        mRedraw = true;
                        if ( mBoundOnLoad )
                            mReload = true;
                    }
                    mHasBBox = true;
                }
            }

            // member storage of cached parameter values
            bool mReload, mRedraw; // do we need to reload/redraw the ptc?
            UT_String mPtcFile; // our ptc file
            int mBoundOnLoad; // use bbox to cull on load?
            int mDisplayChannel; // which channel to display?
            int mOnlyOutputDisplayChannel; // only output display channel
            int mLoadPercentage; // % of points to load into memory
            int mDisplayPercentage; // % of points to display
            float mPointSize; // point/disk size
            int mUseDisk; // render as disks
            std::vector<std::string> mChannelNames; // channel names
            bool mHasBBox; // do we have a bbox to use?
            UT_BoundingBox mBBox; // our bounding box
            bool mOutputDisplayChannelOnly; // only output the display channel
            float mNearDensity, mFarDensity; // near/far clip plane multiplier
            UT_String mCullCamera;

            // storage for all loaded data
            std::vector<UT_Vector3> cachePoints;
            std::vector<UT_Vector3> cacheNormals;
            std::vector<float> cacheRadius;
            std::vector<float> cacheData;
            std::vector<int> cacheDataOffsets;
    };
}

#endif /* RMANPTC_H_ */
