// scalerxbr.h
#ifndef H_TEX_SCALEXBR_H
#define H_TEX_SCALEXBR_H

typedef unsigned int uint32_t;

namespace xbrz
{
	// scaler configuration
	typedef struct ScalerCfg_s
	{
		double    luminanceWeight_;
		double    equalColorTolerance_;
		double    dominantDirectionThreshold;
		double    steepDirectionThreshold;
		double    newTestAttribute_;
		bool      noBlend;     // experimental
		bool      diffusion;   // experimental
		bool      crispBlend;  // experimental
	}ScalerCfg;
	extern ScalerCfg DefaultScalerCfg;

	// do xBR scale
	void scale(size_t factor, const uint32_t* src, uint32_t* trg, int srcWidth, int srcHeight, const ScalerCfg &cfg = ScalerCfg(), int yFirst = 0, int yLast = 2147483647);
}

#endif