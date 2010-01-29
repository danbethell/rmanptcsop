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

#ifndef GR_RMANPTC_H_
#define GR_RMANPTC_H_

#include <GR/GR_Detail.h>
#include <GR/GR_RenderHook.h>
#include <GR/GR_DisplayOption.h>

namespace rmanPtcSop
{
    /*! \class GR_rmanPtc
     * \brief Provides a GL render preview for our rmanPtcDetail.
     *
     * This class implements the GR_RenderHook interface and allows us to
     * render using GL a preview of our RenderMan point cloud.
     */
    class GR_rmanPtc : public GR_RenderHook
    {
        public:
            GR_rmanPtc() {;}
            virtual ~GR_rmanPtc() {;}

            virtual void renderWire( GU_Detail *gdp,
                    RE_Render &ren,
                    const GR_AttribOffset &ptinfo,
                    const GR_DisplayOption *dopt,
                    float lod,
                    const GU_PrimGroupClosure *hidden_geometry );

            virtual void renderShaded( GU_Detail *gdp,
                    RE_Render &ren,
                    const GR_AttribOffset &ptinfo,
                    const GR_DisplayOption *dopt,
                    float lod,
                    const GU_PrimGroupClosure *hidden_geometry );

            virtual const char *getName() const { return "GR_rmanPtc"; }
    };
}

#endif /* GR_RMANPTC_H_ */
