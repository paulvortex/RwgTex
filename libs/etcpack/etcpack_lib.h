// etcpack_lib.h
#ifndef H_TEX_ETCPACK_LIB_H
#define H_TEX_ETCPACK_LIB_H

#define ETCPACK_VERSION_S "2.72"

// data types
typedef unsigned char uint8;

// EtcPack library transformation
#define main main_etcpack

// etcpack.cxx: fix conflict with ATI_Compressonator
#define computeAverageColor2x4noQuantFloat etcpack_computeAverageColor2x4noQuantFloat
#define computeAverageColor4x2noQuantFloat etcpack_computeAverageColor4x2noQuantFloat
#define compressBlockWithTable2x4 etcpack_compressBlockWithTable2x4
#define compressBlockWithTable2x4percep etcpack_compressBlockWithTable2x4percep
#define compressBlockWithTable4x2 etcpack_compressBlockWithTable4x2
#define compressBlockWithTable4x2percep etcpack_compressBlockWithTable4x2percep
#define tryalltables_3bittable2x4 etcpack_tryalltables_3bittable2x4
#define tryalltables_3bittable2x4percep etcpack_tryalltables_3bittable2x4percep
#define tryalltables_3bittable4x2 etcpack_tryalltables_3bittable4x2
#define tryalltables_3bittable4x2percep etcpack_tryalltables_3bittable4x2percep
#define quantize444ColorCombined etcpack_quantize444ColorCombined
#define quantize555ColorCombined etcpack_quantize555ColorCombined
#define quantize444ColorCombinedPerceptual etcpack_quantize444ColorCombinedPerceptual
#define quantize555ColorCombinedPerceptual etcpack_quantize555ColorCombinedPerceptual
#define compressBlockDiffFlipCombined etcpack_compressBlockDiffFlipCombined
#define compressBlockDiffFlipCombinedPerceptual etcpack_compressBlockDiffFlipCombinedPerceptual
#define calcBlockErrorRGB etcpack_calcBlockErrorRGB
#define calcBlockPerceptualErrorRGB etcpack_calcBlockPerceptualErrorRGB
#define compressBlockDiffFlipFastPerceptual etcpack_compressBlockDiffFlipFastPerceptual
#define write_big_endian_2byte_word etcpack_write_big_endian_2byte_word
#define write_big_endian_4byte_word etcpack_write_big_endian_4byte_word
#define readSrcFile etcpack_readSrcFile
#define readArguments etcpack_readArguments
#define calculateError59Tperceptual1000 etcpack_calculateError59Tperceptual1000
#define decompressColor etcpack_decompressColor
#define calculatePaintColors58H etcpack_calculatePaintColors58H
#define formatSigned etcpack_formatSigned
#define etcalculatePaintColors59T etcpack_etcalculatePaintColors59T
#define etetcpack_decompressColor etcpack_etetcpack_decompressColor
#define etetcpack_calculatePaintColors58H etcpack_etetcpack_calculatePaintColors58H
#define etdecompressBlockTHUMB58H etcpack_etdecompressBlockTHUMB58H
#define etdecompressBlockTHUMB59T etcpack_etdecompressBlockTHUMB59T
#define etdecompressBlockPlanar57 etcpack_etdecompressBlockPlanar57
#define etdecompressBlockTHUMB58HAlpha etcpack_etdecompressBlockTHUMB58HAlpha
#define etdecompressBlockTHUMB59TAlpha etcpack_etdecompressBlockTHUMB59TAlpha
#define etdecompressBlockDifferentialWithAlpha etcpack_etdecompressBlockDifferentialWithAlpha
#define get16bits11bits etcpack_get16bits11bits
#define etsetupAlphaTable etcpack_etsetupAlphaTable
#define getbit etcpack_getbit
#define alphaTable etcpack_alphaTable
#define clamp etcpack_clamp
#define etdecompressBlockAlpha16bit etcpack_etdecompressBlockAlpha16bit
#define etdecompressBlockETC21BitAlpha etcpack_etdecompressBlockETC21BitAlpha
#define etdecompressBlockETC2 etcpack_etdecompressBlockETC2
#define etdecompressBlockAlpha etcpack_etdecompressBlockAlpha

// etcdec.cxx: fix conflict with ATI_Compressonator
#define decompressBlockDiffFlip etcpack_decompressBlockDiffFlip
#define read_big_endian_2byte_word etcpack_read_big_endian_2byte_word
#define read_big_endian_4byte_word etcpack_read_big_endian_4byte_word

// common
bool readCompressParams(void);

// ETC1 compression 
double compressBlockDiffFlipFast(uint8 *img, uint8 *imgdec,int width,int height,int startx,int starty, unsigned int &compressed1, unsigned int &compressed2);
void   compressBlockDiffFlipFastPerceptual(uint8 *img, uint8 *imgdec,int width,int height,int startx,int starty, unsigned int &compressed1, unsigned int &compressed2);
void   compressBlockETC1Exhaustive(uint8 *img, uint8 *imgdec,int width,int height,int startx,int starty, unsigned int &compressed1, unsigned int &compressed2);
void   compressBlockETC1ExhaustivePerceptual(uint8 *img, uint8 *imgdec,int width,int height,int startx,int starty, unsigned int &compressed1, unsigned int &compressed2);
void   compressBlockAlphaSlow(uint8* data, int ix, int iy, int width, int height, uint8* returnData);
void   compressBlockAlphaFast(uint8 * data, int ix, int iy, int width, int height, uint8* returnData);
void   decompressBlockDiffFlipC(unsigned int block_part1, unsigned int block_part2, uint8 *img, int width, int height, int startx, int starty, int channels);

// ETC2 compression
void   compressBlockETC2Fast(uint8 *img, uint8* alphaimg, uint8 *imgdec,int width,int height,int startx,int starty, unsigned int &compressed1, unsigned int &compressed2);
void   compressBlockETC2Exhaustive(uint8 *img, uint8 *imgdec,int width,int height,int startx,int starty, unsigned int &compressed1, unsigned int &compressed2);
void   compressBlockETC2FastPerceptual(uint8 *img, uint8 *imgdec,int width,int height,int startx,int starty, unsigned int &compressed1, unsigned int &compressed2);
void   compressBlockETC2ExhaustivePerceptual(uint8 *img, uint8 *imgdec,int width,int height,int startx,int starty, unsigned int &compressed1, unsigned int &compressed2);
void   decompressBlockETC2c(unsigned int block_part1, unsigned int block_part2, uint8 *img, int width, int height, int startx, int starty, int channels);


#endif