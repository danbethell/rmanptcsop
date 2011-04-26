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

#ifndef RMANPTCDETAIL_H_
#define RMANPTCDETAIL_H_

#include <GU/GU_Detail.h>
#include <UT/UT_String.h>
#include <UT/UT_BoundingBox.h>

namespace rmanPtcSop
{
    /*! \class rmanPtcDetail
     * \brief Inherits from GU_Detail and describes a renderman point cloud.
     *
     * This is a lightweight class derived from GU_Detail which contains a
     * little bit of information that we need to render using GL in our
     * GR_RenderHook.
     */
    class rmanPtcDetail : public GU_Detail
    {
        public:
            rmanPtcDetail() :
                redraw(false),
                nPoints(0),
                nChannels(0),
                datasize(0),
                display_probability(1.f),
                use_cull_bbox(false)
            {;}
            virtual ~rmanPtcDetail()
            {;}

            // force a redraw
            bool redraw;

            // information about our pointcloud
            int nPoints;
            int nLoaded;
            int nChannels;
            int datasize;
            std::vector<std::string> types;
            std::vector<std::string> names;
            GB_AttributeRef N_attrib, R_attrib;
            std::vector<GB_AttributeRef> attributes;
            std::vector<int> attribute_size;
            std::string path;
            float bbox[6];

            // draw with opengl
            float display_probability;
            int display_channel;
            float point_size;
            bool use_disk;

            // cull bounding box
            bool use_cull_bbox;
            UT_BoundingBox cull_bbox;
    };
}

#endif /* RMANPTCDETAIL_H_ */
