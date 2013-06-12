// file_dds.h
#ifndef H_FILE_DDS_H
#define H_FILE_DDS_H

extern TexContainer CONTAINER_DDS;

bool   DDS_Scan(byte *data);
void   DDS_PrintHeader(byte *data);
byte  *DDS_CreateHeader(LoadedImage *image, TexFormat *format, size_t *outsize);
size_t DDS_WriteMipHeader(byte *stream, size_t width, size_t height, size_t pixeldatasize);
bool   DDS_Read(TexDecodeTask *task);

// DDS file structure
#define DDSD2 DDSURFACEDESC2
static const DWORD DDS_HEADER = FOURCC('D', 'D', 'S', ' ');
#ifdef F_FILE_DDS_C
uint DDS_HEADER_SIZE = sizeof(DDSD2) + sizeof(DDS_HEADER);
#else
extern uint DDS_HEADER_SIZE;
#endif

#endif
