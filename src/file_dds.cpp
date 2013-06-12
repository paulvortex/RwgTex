////////////////////////////////////////////////////////////////
//
// RwgTex / DDS file format
// (c) Pavel [VorteX] Timofeyev
// See LICENSE text file for a license agreement
//
////////////////////////////////

#define F_FILE_DDS_C
#include "ddraw.h"
#include "d3d9types.h"
#include "main.h"
#include "tex.h"

TexContainer CONTAINER_DDS =
{
	"DDS", "DirectDraw Surface (.DDS)", "dds", 4,
	&DDS_Scan,
	DDS_HEADER_SIZE, 0, 0,
	&DDS_PrintHeader,
	&DDS_CreateHeader,
	&DDS_WriteMipHeader,
	&DDS_Read,
};

bool DDS_Scan(byte *data)
{
	if (memcmp(data, &DDS_HEADER, 4))
		return false;
	return true;
}

void DDS_PrintHeader(byte *data)
{
	DDSD2 *dds;

	dds = (DDSD2 *)(data + 4);
	Print("DDS headers:\n");
	Print("  PixelFormat:\n");
	Print("    flags = %i\n", dds->ddpfPixelFormat.dwFlags);
	Print("    fourCC = %c%c%c%c\n", dds->ddpfPixelFormat.dwFourCC & 0xFF, (dds->ddpfPixelFormat.dwFourCC >> 8) & 0xFF, (dds->ddpfPixelFormat.dwFourCC >> 16) & 0xFF, (dds->ddpfPixelFormat.dwFourCC >> 24) & 0xFF);
	Print("    RGBBitCount = %i\n", dds->ddpfPixelFormat.dwRGBBitCount);
	Print("    RBitMask = 0x%08X\n", dds->ddpfPixelFormat.dwRBitMask);
	Print("    BBitMask = 0x%08X\n", dds->ddpfPixelFormat.dwBBitMask);
	Print("    AlphaBitMask = 0x%08X\n", dds->ddpfPixelFormat.dwRGBAlphaBitMask);
	Print("  EmptyFaceColor = %c%c%c%c\n", dds->dwEmptyFaceColor & 0xFF, (dds->dwEmptyFaceColor >> 8) & 0xFF, (dds->dwEmptyFaceColor >> 16) & 0xFF, (dds->dwEmptyFaceColor >> 24) & 0xFF);
	Print("    GBitMask = 0x%08X\n", dds->ddpfPixelFormat.dwGBitMask);
	Print("  Comment = %c%c%c%c%c%c%c%c\n", dds->dwAlphaBitDepth & 0xFF, (dds->dwAlphaBitDepth >> 8) & 0xFF, (dds->dwAlphaBitDepth >> 16) & 0xFF, (dds->dwAlphaBitDepth >> 24) & 0xFF, dds->dwReserved & 0xFF, (dds->dwReserved >> 8) & 0xFF, (dds->dwReserved >> 16) & 0xFF, (dds->dwReserved >> 24) & 0xFF);
}

byte *DDS_CreateHeader(LoadedImage *image, TexFormat *format, size_t *outsize)
{
	byte *head;
	DDSD2 *dds;
	
	head = (byte *)mem_alloc(4 + sizeof(DDSD2));
	memcpy(head, &DDS_HEADER, sizeof(DWORD));
	dds = (DDSD2 *)(head + 4);

	// write header
	memset(dds, 0, sizeof(DDSD2));
	dds->dwSize = sizeof(DDSD2);
	dds->dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT | DDSD_MIPMAPCOUNT;
	dds->dwWidth = image->width;
	dds->dwHeight = image->height;
	dds->dwMipMapCount = 1;
	for (MipMap *mipmap = image->mipMaps; mipmap; mipmap = mipmap->nextmip) dds->dwMipMapCount++;
	dds->ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
	dds->ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_COMPLEX | DDSCAPS_MIPMAP;
	if (format->fourCC == FOURCC('B','G','R','A'))
	{
		dds->lPitch = dds->dwWidth * 4;
		dds->ddpfPixelFormat.dwRBitMask = 0x00ff0000;
		dds->ddpfPixelFormat.dwGBitMask = 0x0000ff00;
		dds->ddpfPixelFormat.dwBBitMask = 0x000000ff;
		dds->ddpfPixelFormat.dwRGBBitCount = 32;
        dds->ddpfPixelFormat.dwFlags = DDPF_ALPHAPIXELS | DDPF_RGB;
		dds->ddpfPixelFormat.dwRGBAlphaBitMask = 0xff000000;
	}
	else if (format->fourCC == FOURCC('Y','C','G','1') || format->fourCC == FOURCC('Y','C','G','2'))
	{
		dds->ddpfPixelFormat.dwFourCC = FOURCC('D','X','T','5');
		dds->ddpfPixelFormat.dwRBitMask = 0x00ff0000;
		dds->ddpfPixelFormat.dwGBitMask = 0x0000ff00;
		dds->ddpfPixelFormat.dwBBitMask = 0x000000ff;
		dds->ddpfPixelFormat.dwFlags = DDPF_FOURCC;
		dds->dwEmptyFaceColor = format->fourCC;
	}
	else
	{
		dds->ddpfPixelFormat.dwFourCC = format->fourCC;
		dds->ddpfPixelFormat.dwRBitMask = 0x00ff0000;
		dds->ddpfPixelFormat.dwGBitMask = 0x0000ff00;
		dds->ddpfPixelFormat.dwBBitMask = 0x000000ff;
		dds->ddpfPixelFormat.dwFlags = DDPF_FOURCC;
	}

	// set DDS magic works (GIMP uses it as special info)
	if (tex_useSign)
	{
		dds->dwAlphaBitDepth = tex_signWord1;
		dds->dwReserved = tex_signWord2;
	}

	// fill alphapixels information, ensure that our texture have actual alpha channel
	// also texture may truncate it's alpha by forcing DXT1 compression no it
	if (image->hasAlpha && (format->features & FF_ALPHA))
		dds->ddpfPixelFormat.dwFlags |= DDPF_ALPHAPIXELS;

	// fill alphapremult
	if (format->colorSwizzle == &Swizzle_Premult)
		dds->ddpfPixelFormat.dwFlags |= DDPF_ALPHAPREMULT;
	
	*outsize = 4 + sizeof(DDSD2);
	return head;
}

size_t DDS_WriteMipHeader(byte *stream, size_t width, size_t height, size_t pixeldatasize)
{
	return 0;
}

bool DDS_Read(TexDecodeTask *task)
{
	DDSD2 *dds;

	// validate header
	if (task->datasize < DDS_HEADER_SIZE)
	{
		sprintf(task->errorMessage, "failed to read DDS header");
		return false;
	}
	dds = (DDSD2 *)(task->data + 4);
	if (!(dds->dwFlags & DDSD_WIDTH)) { sprintf(task->errorMessage, "DDSD_WIDTH not specified"); return false; }
	if (!(dds->dwFlags & DDSD_HEIGHT)) { sprintf(task->errorMessage, "DDSD_HEIGHT not specified"); return false; }

	// detect file type
	if (!task->codec && (dds->ddpfPixelFormat.dwFlags & DDPF_RGB) && (dds->ddpfPixelFormat.dwRGBBitCount == 32))
	{
		task->codec = &CODEC_BGRA;
		task->format = &F_BGRA;
	}
	if (!task->codec && (dds->ddpfPixelFormat.dwFlags & DDPF_FOURCC))
	{
		if (dds->dwEmptyFaceColor)
			findFormatByFourCCAndAlpha(dds->dwEmptyFaceColor, (dds->ddpfPixelFormat.dwFlags & DDPF_ALPHAPIXELS) ? true : false, &task->codec, &task->format);
		if (!task->codec)
			findFormatByFourCCAndAlpha(dds->ddpfPixelFormat.dwFourCC, (dds->ddpfPixelFormat.dwFlags & DDPF_ALPHAPIXELS) ? true : false, &task->codec, &task->format);
	}
	if (!task->codec)
	{
		sprintf(task->errorMessage, "failed to find decoder");
		return false;
	}

	// read comment
	task->comment = (char *)mem_alloc(9);
	sprintf(task->comment, "%c%c%c%c%c%c%c%c", dds->dwAlphaBitDepth & 0xFF, (dds->dwAlphaBitDepth >> 8) & 0xFF, (dds->dwAlphaBitDepth >> 16) & 0xFF, (dds->dwAlphaBitDepth >> 24) & 0xFF, dds->dwReserved & 0xFF, (dds->dwReserved >> 8) & 0xFF, (dds->dwReserved >> 16) & 0xFF, (dds->dwReserved >> 24) & 0xFF);
	task->comment[9] = 0;

	// get image dimensions
	task->hasAlpha = ((dds->ddpfPixelFormat.dwFlags & DDPF_ALPHAPIXELS) && (task->format->features & FF_ALPHA)) ? true : false;
	task->numMipmaps = (dds->dwFlags & DDSD_MIPMAPCOUNT) ? (dds->dwMipMapCount - 1) : 0;
	task->width = dds->dwWidth;
	task->height = dds->dwHeight;
	task->pixeldata = task->data + DDS_HEADER_SIZE;
	task->pixeldatasize = task->datasize - DDS_HEADER_SIZE;
	if (dds->ddpfPixelFormat.dwRBitMask == 0x00ff0000 && dds->ddpfPixelFormat.dwBBitMask == 0x000000ff)
		task->colorSwap = true;
	return true;
}