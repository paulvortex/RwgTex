// pvrtextool_lib.h
#ifndef H_PVRTEXTOOL_LIB_H
#define H_PVRTEXTOOL_LIB_H

#include "inc/PVRTTexture.h"
#include "inc/PVRTexture.h"
#include "inc/PVRTextureDefines.h"
#include "inc/PVRTextureUtilities.h"
#include "inc/PVRTextureVersion.h"
#include "inc/PVRTDecompress.h"

namespace pvr
{
	int PVRTDecompressPVRTC(const void* pCompressedData,
		int Do2bitMode,
		int XDim,
		int YDim,
		unsigned char* pResultImage);
}

#endif