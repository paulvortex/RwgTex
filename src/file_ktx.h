// file_ktx.h
#ifndef H_FILE_KTX_H
#define H_FILE_KTX_H

extern TexContainer CONTAINER_KTX;

bool   KTX_Scan(byte *data);
void   KTX_PrintHeader(byte *data);
byte  *KTX_CreateHeader(LoadedImage *image, TexFormat *format, size_t *outsize);
size_t KTX_WriteMipHeader(byte *stream, size_t width, size_t height, size_t pixeldatasize);
bool   KTX_Read(TexDecodeTask *task);

// KTX file strucrure
const char KTX_IDENTIFIER[12] = { '«', 'K', 'T', 'X', ' ', '1', '1', '»', '\r', '\n', '\x1A', '\n' };
struct KTX_HEADER
{
	byte identifier[12];
	uint endianness;
	uint glType;
	uint glTypeSize;
	uint glFormat;
	uint glInternalFormat;
	uint glBaseInternalFormat;
	uint pixelWidth;
	uint pixelHeight;
	uint pixelDepth;
	uint numberOfArrayElements;
	uint numberOfFaces;
	uint numberOfMipmapLevels;
	uint bytesOfKeyValueData;
};

#endif
