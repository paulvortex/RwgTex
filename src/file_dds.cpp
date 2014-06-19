////////////////////////////////////////////////////////////////
//
// RwgTex / DDS file format
// (c) Pavel [VorteX] Timofeyev
// See LICENSE text file for a license agreement
//
////////////////////////////////

#define F_FILE_DDS_C

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
	DDSHeader_t *dds;

	dds = (DDSHeader_t *)(data + 4);
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
	Print("  Comment = %c%c%c%c%c%c%c%c\n", dds->cComment[0], dds->cComment[1], dds->cComment[2], dds->cComment[3], dds->cComment[4], dds->cComment[5], dds->cComment[6], dds->cComment[7]);
}

byte *DDS_CreateHeader(LoadedImage *image, TexFormat *format, size_t *outsize)
{
	byte *head;
	DDSHeader_t *dds;

	head = (byte *)mem_alloc(4 + sizeof(DDSHeader_t));
	memcpy(head, &DDS_HEADER, sizeof(DWORD));
	dds = (DDSHeader_t *)(head + 4);

	// write header
	memset(dds, 0, sizeof(DDSHeader_t));
	dds->dwSize = sizeof(DDSHeader_t);
	dds->dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
	dds->dwWidth = image->width;
	dds->dwHeight = image->height;
	dds->dwMipMapCount = 0;
	for (ImageMap *map = image->maps; map; map = map->next)
		dds->dwMipMapCount++;
	dds->ddpfPixelFormat.dwSize = DDS_SIZE_PIXELFORMAT;
	dds->ddsCaps.dwCaps = DDSCAPS_TEXTURE;
	if (dds->dwMipMapCount > 1)
	{
		dds->ddsCaps.dwCaps = dds->ddsCaps.dwCaps | DDSCAPS_COMPLEX | DDSCAPS_MIPMAP;
		dds->dwFlags = dds->dwFlags | DDSD_MIPMAPCOUNT;
	}
	if (format->fourCC == FOURCC('B','G','R','A'))
	{
		dds->lPitch = dds->dwWidth * 4;
		dds->ddpfPixelFormat.dwRBitMask = 0x00ff0000;
		dds->ddpfPixelFormat.dwGBitMask = 0x0000ff00;
		dds->ddpfPixelFormat.dwBBitMask = 0x000000ff;
		dds->ddpfPixelFormat.dwRGBBitCount = 32;
        dds->ddpfPixelFormat.dwFlags = DDPF_ALPHAPIXELS | DDPF_RGB | DDPF_FOURCC;
		dds->ddpfPixelFormat.dwRGBAlphaBitMask = 0xff000000;
		dds->ddpfPixelFormat.dwFourCC = format->fourCC;
	}
	else if (format->block->fourCC != format->fourCC)
	{
		dds->ddpfPixelFormat.dwFourCC = format->block->fourCC;
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

	// NVidia TextureTools flags (useful!)
	if (image->maps->sRGB)
		dds->ddpfPixelFormat.dwFlags = dds->ddpfPixelFormat.dwFlags | DDPF_SRGB;
	if (image->datatype == IMAGE_NORMALMAP)
		dds->ddpfPixelFormat.dwFlags = dds->ddpfPixelFormat.dwFlags | DDPF_NORMALMAP;
	
	// set DDS comment (GIMP uses it as special info)
	if (tex_useSign)
		memcpy(dds->cComment, tex_sign, 8);

	// fill average color information
	if (image->hasAverageColor)
	{
		dds->ddckAvgColor.cFlag = 0x41; 
		dds->ddckAvgColor.ucRGB[0] = image->averagecolor[0];
		dds->ddckAvgColor.ucRGB[1] = image->averagecolor[1];
		dds->ddckAvgColor.ucRGB[2] = image->averagecolor[2];
	}

	// fill alphapixels information, ensure that our texture have actual alpha channel
	// also texture may truncate it's alpha by forcing DXT1 compression no it
	if (image->hasAlpha && (format->features & FF_ALPHA))
		dds->ddpfPixelFormat.dwFlags |= DDPF_ALPHAPIXELS;

	// fill alphapremult
	if (format->colorSwizzle == &Swizzle_Premult)
		dds->ddpfPixelFormat.dwFlags |= DDPF_ALPHAPREMULT;
	
	*outsize = 4 + sizeof(DDSHeader_t);
	return head;
}

size_t DDS_WriteMipHeader(byte *stream, size_t width, size_t height, size_t pixeldatasize)
{
	return 0;
}

bool DDS_Read(TexDecodeTask *task)
{
	DDSHeader_t *dds;

	// validate header
	if (task->datasize < DDS_HEADER_SIZE)
	{
		sprintf(task->errorMessage, "failed to read DDS header");
		return false;
	}
	dds = (DDSHeader_t *)(task->data + 4);
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
	memcpy(task->comment, dds->cComment, 8);
	task->comment[9] = 0;

	// get average color information
	if (dds->ddckAvgColor.cFlag == 0x41)
	{
		task->ImageParms.hasAverageColor = true;
		task->ImageParms.averagecolor[0] = dds->ddckAvgColor.ucRGB[0];
		task->ImageParms.averagecolor[1] = dds->ddckAvgColor.ucRGB[1];
		task->ImageParms.averagecolor[2] = dds->ddckAvgColor.ucRGB[2];
	}

	// get image dimensions
	task->ImageParms.hasAlpha = ((dds->ddpfPixelFormat.dwFlags & DDPF_ALPHAPIXELS) && (task->format->features & FF_ALPHA)) ? true : false;
	task->ImageParms.colorSwap = (dds->ddpfPixelFormat.dwRBitMask == 0x00ff0000 && dds->ddpfPixelFormat.dwBBitMask == 0x000000ff) ? true : false;
	task->ImageParms.isNormalmap = (dds->ddpfPixelFormat.dwFlags & DDPF_NORMALMAP) ? true : false;
	task->ImageParms.sRGB = (dds->ddpfPixelFormat.dwFlags & DDPF_SRGB) ? true : false;
	task->numMipmaps = (dds->dwFlags & DDSD_MIPMAPCOUNT) ? (dds->dwMipMapCount - 1) : 0;
	task->width = dds->dwWidth;
	task->height = dds->dwHeight;
	task->pixeldata = task->data + DDS_HEADER_SIZE;
	task->pixeldatasize = task->datasize - DDS_HEADER_SIZE;
	return true;
}