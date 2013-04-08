// dds.h
#pragma once

int DDS_Main(int argc, char **argv);
void DDS_PrintModules(void);

// compression formats
#define FORMAT_DXT1 MAKEFOURCC('D','X','T','1')
#define FORMAT_DXT2 MAKEFOURCC('D','X','T','2')
#define FORMAT_DXT3 MAKEFOURCC('D','X','T','3')
#define FORMAT_DXT4 MAKEFOURCC('D','X','T','4')
#define FORMAT_DXT5 MAKEFOURCC('D','X','T','5')
#define FORMAT_BGRA MAKEFOURCC('B','G','R','A')
#define FORMAT_RXGB MAKEFOURCC('R','X','G','B')
#define FORMAT_YCG1 MAKEFOURCC('Y','C','G','1')
#define FORMAT_YCG2 MAKEFOURCC('Y','C','G','2')

char *getCompressionFormatString(DWORD formatCC);
double getCompressionRatio(DWORD formatCC);

// compressors
#include "dds_nvdxtlib.h"
#include "dds_nvtt.h"
#include "dds_aticompress.h"
#include "dds_bgra.h"
#include "dds_gimp.h"

typedef enum
{
	COMPRESSOR_AUTOSELECT,
	COMPRESSOR_NVIDIA,
	COMPRESSOR_NVIDIA_TT,
	COMPRESSOR_ATI,
	COMPRESSOR_BGRA,
	COMPRESSOR_GIMPDDS,
	NUM_COMPRESSORS
}
TOOL;