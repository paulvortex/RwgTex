// file_dds.h
#ifndef H_FILE_DDS_H
#define H_FILE_DDS_H

extern TexContainer CONTAINER_DDS;

bool   DDS_Scan(byte *data);
void   DDS_PrintHeader(byte *data);
byte  *DDS_CreateHeader(LoadedImage *image, TexFormat *format, size_t *outsize);
size_t DDS_WriteMipHeader(byte *stream, size_t width, size_t height, size_t pixeldatasize);
bool   DDS_Read(TexDecodeTask *task);

// DDS flags (dwFlags)
#define DDSD_CAPS         0x1 // ddsCaps parameter used
#define DDSD_HEIGHT       0x2 // dwHeight parameter used
#define DDSD_WIDTH        0x4 // dwWidth parameter used
#define DDSD_PITCH        0x8 // lPitch parameter used
#define DDSD_PIXELFORMAT  0x1000 // ddpfPixelFormat parameter used
#define DDSD_MIPMAPCOUNT  0x20000 // dwMipMapCount parameter used
#define DDSD_LINEARSIZE   0x80000 // pitch is provided for a compressed texture
#define DDSD_DEPTH        0x800000 // depth texture

// DDS caps (dwCaps)
#define DDSCAPS_COMPLEX   0x8
#define DDSCAPS_TEXTURE   0x1000
#define DDSCAPS_MIPMAP    0x400000

// DDS caps2 (dwCaps2)
#define DDSCAPS2_CUBEMAP            0x200 // texture is cubemap
#define DDSCAPS2_CUBEMAP_POSITIVEX  0x400 // stored flag
#define DDSCAPS2_CUBEMAP_NEGATIVEX	0x800 // stored flag
#define DDSCAPS2_CUBEMAP_POSITIVEY	0x1000 // stored flag
#define DDSCAPS2_CUBEMAP_NEGATIVEY	0x2000 // stored flag
#define DDSCAPS2_CUBEMAP_POSITIVEZ	0x4000 // stored flag
#define DDSCAPS2_CUBEMAP_NEGATIVEZ	0x8000 // stored flag
#define DDSCAPS2_CUBEMAP_ALL_FACES  0xFC00U // all faces
#define DDSCAPS2_VOLUME	            0x200000 // volume texture

// DDS pixelformat flags
#define DDPF_ALPHAPIXELS            0x1 // has alpha channel
#define DDPF_RGB                    0x4 // has RGB channels
#define DDPF_FOURCC                 0x40 // fourCC field is valid
#define DDPF_ALPHAPREMULT           0x8000 // RGB premultiplied by the alpha
#define DDPF_SRGB                   0x40000000 // NVTT: sRGB colorspace
#define DDPF_NORMALMAP              0x80000000 // NVTT: normalmap colorspace

// DDSURFACEDESC2
typedef struct DDSHeader_s
{
	// byte 0-3
    DWORD dwSize;                            // size of the DDSURFACEDESC structure
	// byte 4-15
    DWORD dwFlags;                           // determines what fields are valid
    DWORD dwHeight;                          // height of surface to be created
    DWORD dwWidth;                           // width of input surface
	// byte 16-19
    union
    {
        LONG lPitch;                         // distance to start of next line (return value only)
        DWORD dwLinearSize;                  // Formless late-allocated optimized surface size
    };
	// byte 20-23
    union
    {
        DWORD dwBackBufferCount;             // number of back buffers requested
        DWORD dwDepth;                       // the depth if this is a volume texture 
    };
	// byte 24-27
    union
    {
        DWORD dwMipMapCount;                 // number of mip-map levels requested
        DWORD dwRefreshRate;                 // refresh rate (used when display mode is described)
        DWORD dwSrcVBHandle;                 // The source used in VB::Optimize
    };
	// byte 28-35
	union
    {
		struct
		{
			DWORD dwAlphaBitDepth;            // depth of alpha buffer requested
			DWORD dwReserved;                 // reserved
		} ddckAlphaDepth;
		// VorteX: store a small 8-char comment inside DDS file, GIMP DDS Plugin uses it as special info field
		char cComment[8];
	};
	// byte 36-39
	union
    {
		LPVOID lpSurface;                    // pointer to the associated surface memory
		// VorteX: this field is used to store average color of image if first byte equals to 'A'
		struct
		{
			char cFlag;                      // must be  0x41
			unsigned char ucRGB[3];          // RGB
		} ddckAvgColor;
	};
	// byte 40-47
    union
    {
		struct 
		{
			DWORD dwColorSpaceLowValue;
			DWORD dwColorSpaceHighValue;
		} ddckCKDestOverlay;
		// VorteX: this field is used to store swizzled format fourCC code
        DWORD dwEmptyFaceColor;              // Physical color for empty cubemap faces
	};
	// byte 48-71
	struct
	{
		DWORD dwColorSpaceLowValue;
		DWORD dwColorSpaceHighValue;
	} ddckCKDestBlt;
	struct
	{
		
		DWORD dwColorSpaceLowValue;
		DWORD dwColorSpaceHighValue;
	} ddckCKSrcOverlay;
	struct
	{
		
		DWORD dwColorSpaceLowValue;
		DWORD dwColorSpaceHighValue;
	} ddckCKSrcBlt;
	// byte 72-107
    union
    {
		// DDSPixelFormat
		#define DDS_SIZE_PIXELFORMAT 32
		struct
		{
			DWORD dwSize;                      // size of structure
			DWORD dwFlags;                     // pixel format flags
			DWORD dwFourCC;                    // (FOURCC code)
			union
			{
				DWORD dwRGBBitCount;           // how many bits per pixel
				DWORD dwYUVBitCount;           // how many bits per pixel
				DWORD dwZBufferBitDepth;       // how many total bits/pixel in z buffer (including any stencil bits)
				DWORD dwAlphaBitDepth;         // how many bits for alpha channels
				DWORD dwLuminanceBitCount;     // how many bits per pixel
				DWORD dwBumpBitCount;          // how many bits per "buxel", total
				DWORD dwPrivateFormatBitCount; // Bits per pixel of private driver formats. Only valid in texture format list and if DDPF_D3DFORMAT is set
			};
			union
			{
				DWORD dwRBitMask;              // mask for red bit
				DWORD dwYBitMask;              // mask for Y bits
				DWORD dwStencilBitDepth;       // how many stencil bits (note: dwZBufferBitDepth-dwStencilBitDepth is total Z-only bits)
				DWORD dwLuminanceBitMask;      // mask for luminance bits
				DWORD dwBumpDuBitMask;         // mask for bump map U delta bits
				DWORD dwOperations;            // DDPF_D3DFORMAT Operations
			};
			union
			{
				DWORD dwGBitMask;              // mask for green bits
				DWORD dwUBitMask;              // mask for U bits
				DWORD dwZBitMask;              // mask for Z bits
				DWORD dwBumpDvBitMask;         // mask for bump map V delta bits
				struct
				{
					WORD  wFlipMSTypes;        // Multisample methods supported via flip for this D3DFORMAT
					WORD  wBltMSTypes;         // Multisample methods supported via blt for this D3DFORMAT
				} MultiSampleCaps;
			};
			union
			{
				DWORD dwBBitMask;              // mask for blue bits
				DWORD dwVBitMask;              // mask for V bits
				DWORD dwStencilBitMask;        // mask for stencil bits
				DWORD dwBumpLuminanceBitMask;  // mask for luminance in bump map
			};
			union
			{
				DWORD dwRGBAlphaBitMask;       // mask for alpha channel
				DWORD dwYUVAlphaBitMask;       // mask for alpha channel
				DWORD dwLuminanceAlphaBitMask; // mask for alpha channel
				DWORD dwRGBZBitMask;           // mask for Z channel
				DWORD dwYUVZBitMask;           // mask for Z channel
			};
		} ddpfPixelFormat;
        DWORD dwFVF;                           // vertex format description of vertex buffers
    };
	// byte 108-123
	struct
	{
		DWORD dwCaps;                          // capabilities of surface wanted
		DWORD dwCaps2;
		DWORD dwCaps3;
		union
		{
			DWORD dwCaps4;
			DWORD dwVolumeDepth;
		};
	} ddsCaps;
	// byte 124-127
    DWORD dwTextureStage;                      // stage in multitexture cascade
	// 128 bytes total
} DDSHeader_t;

// DDS file structure
static const DWORD DDS_HEADER = FOURCC('D', 'D', 'S', ' ');
#ifdef F_FILE_DDS_C
uint DDS_HEADER_SIZE = sizeof(DDSHeader_t) + sizeof(DDS_HEADER);
#else
extern uint DDS_HEADER_SIZE;
#endif

#endif
