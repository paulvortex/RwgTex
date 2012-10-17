// dds.h
#pragma once

int DDS_Main(int argc, char **argv);
void DDS_PrintModules(void);

// supporter compressors
typedef enum
{
	COMPRESSOR_AUTOSELECT,
	COMPRESSOR_INTERNAL,
	COMPRESSOR_NVIDIA,
	COMPRESSOR_NVIDIA_TT,
	COMPRESSOR_ATI,
}
TOOL;

// supported formats
#define FORMAT_DXT1 MAKEFOURCC('D','X','T','1')
#define FORMAT_DXT2 MAKEFOURCC('D','X','T','2')
#define FORMAT_DXT3 MAKEFOURCC('D','X','T','3')
#define FORMAT_DXT4 MAKEFOURCC('D','X','T','4')
#define FORMAT_DXT5 MAKEFOURCC('D','X','T','5')
#define FORMAT_BGRA MAKEFOURCC('B','G','R','A')
#define FORMAT_RXGB MAKEFOURCC('R','X','G','B')