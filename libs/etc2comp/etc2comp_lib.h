// etccomp_lib.h
#ifndef H_TEX_ETCCOMP_LIB_H
#define H_TEX_ETCCOMP_LIB_H

#define ETC2COMP_GITVERSION "'aecc84f' on 22 Sep 2016"

#include "Etc/EtcConfig.h"
#include "Etc/Etc.h"
#include "Etc/EtcImage.h"
#include "EtcCodec/EtcErrorMetric.h"

#ifdef F_TOOL_ETC2COMP_C
namespace Etc
{ 
	char *ImageStatusName(Image::EncodingStatus status)
	{
		if (status == Image::EncodingStatus::SUCCESS) return "SUCCESS";
		if (status == Image::EncodingStatus::WARNING_THRESHOLD) return "WARNING_THRESHOLD";
		if (status == Image::EncodingStatus::WARNING_EFFORT_OUT_OF_RANGE) return "WARNING_EFFORT_OUT_OF_RANGE";
		if (status == Image::EncodingStatus::WARNING_JOBS_OUT_OF_RANGE) return "WARNING_JOBS_OUT_OF_RANGE";
		if (status == Image::EncodingStatus::WARNING_SOME_NON_OPAQUE_PIXELS) return "WARNING_SOME_NON_OPAQUE_PIXELS";
		if (status == Image::EncodingStatus::WARNING_ALL_OPAQUE_PIXELS) return "WARNING_ALL_OPAQUE_PIXELS";
		if (status == Image::EncodingStatus::WARNING_ALL_TRANSPARENT_PIXELS) return "WARNING_ALL_TRANSPARENT_PIXELS";
		if (status == Image::EncodingStatus::WARNING_SOME_TRANSLUCENT_PIXELS) return "WARNING_SOME_TRANSLUCENT_PIXELS";
		if (status == Image::EncodingStatus::WARNING_SOME_RGBA_NOT_0_TO_1) return "WARNING_SOME_RGBA_NOT_0_TO_1";
		if (status == Image::EncodingStatus::WARNING_SOME_BLUE_VALUES_ARE_NOT_ZERO) return "WARNING_SOME_BLUE_VALUES_ARE_NOT_ZERO";
		if (status == Image::EncodingStatus::WARNING_SOME_GREEN_VALUES_ARE_NOT_ZERO) return "WARNING_SOME_GREEN_VALUES_ARE_NOT_ZERO";
		if (status == Image::EncodingStatus::ERROR_THRESHOLD) return "ERROR_THRESHOLD";
		if (status == Image::EncodingStatus::ERROR_UNKNOWN_FORMAT) return "ERROR_UNKNOWN_FORMAT";
		if (status == Image::EncodingStatus::ERROR_UNKNOWN_ERROR_METRIC) return "ERROR_UNKNOWN_ERROR_METRIC";
		if (status == Image::EncodingStatus::ERROR_ZERO_WIDTH_OR_HEIGHT) return "SUCCERROR_ZERO_WIDTH_OR_HEIGHTESS";
		return "UNKNOWNSTATUS";
	}
}
#endif
#endif