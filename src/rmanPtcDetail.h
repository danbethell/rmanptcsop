/*
 * rmanPtcDetail.h
 *
 *  Created on: 18/01/2010
 *      Author: dan
 */

#ifndef RMANPTCDETAIL_H_
#define RMANPTCDETAIL_H_

#include <GU/GU_Detail.h>
#include <UT/UT_String.h>
#include <UT/UT_BoundingBox.h>

namespace rmanPtcSop
{
    class ptcVec
    {
    public: 
        float val[3];
    };

    class rmanPtcDetail : public GU_Detail
    {
        public:
            rmanPtcDetail();
            virtual ~rmanPtcDetail();

            bool redraw;

            int nPoints;
            int nLoaded;
            int nChannels;
            int datasize;
            char **vartypes;
            char **varnames;
            std::string path;
            float bbox[6];
            float display_probability;

            // draw with opengl
            bool use_gl;

            // cull bounding box
            bool use_cull_bbox;
            UT_BoundingBox cull_bbox;

            // cached storage
            std::vector<ptcVec> cachePoints;
            std::vector<ptcVec> cacheNormals;
            std::vector<float> cacheRadius;
            std::vector<float> cacheData;
    };
}

#endif /* RMANPTCDETAIL_H_ */
