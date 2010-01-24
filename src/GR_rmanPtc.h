#ifndef GR_RMANPTC_H_
#define GR_RMANPTC_H_

#include <GR/GR_Detail.h>
#include <GR/GR_RenderHook.h>
#include <GR/GR_DisplayOption.h>

namespace rmanPtcSop
{
    class GR_rmanPtc : public GR_RenderHook
    {
        public:
    GR_rmanPtc() :
        mPtcId(0)
            {
            }

            virtual ~GR_rmanPtc()
            {
            }

/*
            virtual int getWireMask( GU_Detail *gdp,
                    const GR_DisplayOption *opt ) const;

            virtual int getShadedMask( GU_Detail *gdp,
                    const GR_DisplayOption *opt ) const;
*/
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

            virtual const char *getName() const
            {
                return "GR_rmanPtc";
            }

    private:
            GLuint mPtcId;
    };
}

#endif /* GR_RMANPTC_H_ */
