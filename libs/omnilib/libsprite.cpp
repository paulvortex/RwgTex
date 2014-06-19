////////////////////////////////////////////////////////////////
//
// DpOmniTool / Sprite Stuff
// (c) Pavel [VorteX] Timofeyev
// See LICENSE text file for a license agreement
//
////////////////////////////////

#include "dpomnilib.h"
#include "dpomnilibinternal.h"

namespace omnilib
{

// fourCC codes
const char *FOURCC_QUAKESPRITE = "IDSP";

// sprite grow parms
#define SPRITE_GROW_FRAMES    64
#define SPRITE_GROW_PICS      64
#define SPRITE_GROW_COLORMAPS 64

/*
==========================================================================================

  COMMON INTERNAL FUNCTIONS

==========================================================================================
*/

// buffer reading routines
int LittleInt(unsigned char *b)
{
	return b[3]*16777216 + b[2]*65536 + b[1]*256 + b[0];
}

float LittleFloat(unsigned char *b)
{
	union {unsigned char b[4]; float f;} r;

	r.b[0] = b[0];
	r.b[1] = b[1];
	r.b[2] = b[2];
	r.b[3] = b[3];
	return r.f;
}

// image writing
void WriteTGA(char *filename, unsigned char *data, int width, int height, int bpp, unsigned char *colormap)
{
	unsigned char *buffer, *out, *in, *end;
	int y, tgacm, tgabpp, tgatype, tgacolorbits;
	FILE *f;

	// estimate
	if (bpp == 4)
	{
		tgacm = 0; // no colormap
		tgabpp = 4;
		tgatype = 2; // uncompressed
		tgacolorbits = 32;
	}
	else if (bpp == 3)
	{
		tgacm = 0; // no colormap
		tgabpp = 3;
		tgatype = 2; // uncompressed
		tgacolorbits = 24;
	}
	else if (bpp == 1 && colormap)
	{
		tgacm = 1; // colormapped
		tgabpp = 1;
		tgatype = 1; // uncompressed, colormapped
		tgacolorbits = 8;
	}
	else
		_omnilib_error("WriteTGA: bpp %i not supported", bpp);

	// write
	f = _omnilib_safeopen(filename, "wb");
	buffer = (unsigned char *)_omnilib_malloc(width*height*tgabpp + (tgacm ? 1024 : 0) + 18);
	memset(buffer, 0, 18);
	buffer[1] = tgacm;
	buffer[2] = tgatype;
	if (tgacm)
	{
		buffer[5] = (256 >> 0) & 0xFF;
		buffer[6] = (256 >> 8) & 0xFF;
		buffer[7] = 32;
	}
	buffer[12] = (width >> 0) & 0xFF;
	buffer[13] = (width >> 8) & 0xFF;
	buffer[14] = (height >> 0) & 0xFF;
	buffer[15] = (height >> 8) & 0xFF;
	buffer[16] = tgacolorbits;
	out = buffer + 18;
	if (bpp == 4)
	{
		// write BGRA
		for (y = height - 1;y >= 0;y--)
		{
			in = data + y * width * bpp;
			end = in + width * bpp;
			for (;in < end;in += bpp)
			{
				*out++ = in[2];
				*out++ = in[1];
				*out++ = in[0];
				*out++ = in[3];
			}
		}
	}
	else if (bpp == 3)
	{
		// write BGR
		for (y = height - 1;y >= 0;y--)
		{
			in = data + y * width * bpp;
			end = in + width * bpp;
			for (;in < end;in += bpp)
			{
				*out++ = in[2];
				*out++ = in[1];
				*out++ = in[0];
			}
		}
	}
	else
	{
		// write colormap
		in = colormap;
		end = colormap + 1024;
		while(in < end)
		{
			out[0] = in[2];
			out[1] = in[1];
			out[2] = in[0];
			out[3] = in[3];
			out += 4;
			in += 4;
		}
		// write indexes
		for (y = height - 1;y >= 0;y--)
		{
			memcpy(out, data + y * width, width);
			out += width;
		}
	}
	fwrite(buffer, width*height*tgabpp + (tgacm ? 1024 : 0) + 18, 1, f);
	_omnilib_free(buffer);
	fclose(f);
}

void CalcMaxWidthHeight(MetaSprite_t *sprite, long *outmaxwidth, long *outmaxheight, float *outboundradius)
{
	MetaSpriteFrame_t *frame;
	int i, maxwidth, maxheight;

	// calc max width/height, write header
	maxwidth = maxheight = 0;
	for (i = 0; i < sprite->numFrames; i++)
	{
		frame = sprite->frames[i];
		maxwidth = max(maxwidth, frame->width);
		maxheight = max(maxheight, frame->height);
	}

	// store out
	*outmaxwidth = (long)maxwidth;
	*outmaxheight = (long)maxheight;
	*outboundradius = (float)sqrt((maxwidth*0.5)*(maxwidth*0.5)+(maxheight*0.5)*(maxheight*0.5));
}

/*
==========================================================================================

  META CONTAINER

==========================================================================================
*/

// olCreateSprite()
// allocates new sprite
MetaSprite_t *olCreateSprite()
{
	MetaSprite_t *sprite;

	// allocate new sprite
	sprite = (MetaSprite_t *)_omnilib_malloc(sizeof(MetaSprite_t));
	memset(sprite, 0, sizeof(MetaSprite_t));

	return sprite;
}

// olSpriteAddFrame()
// allocates new sprite frame and returns it
MetaSpriteFrame_t *olSpriteAddFrame(MetaSprite_t *sprite)
{
	MetaSpriteFrame_t *frame;

	// alloc frame
	frame = (MetaSpriteFrame_t *)_omnilib_malloc(sizeof(MetaSpriteFrame_t));
	memset(frame, 0, sizeof(MetaSpriteFrame_t));

	// grow frames
	if (sprite->numFrames >= sprite->maxFrames)
	{
		sprite->maxFrames += SPRITE_GROW_FRAMES;
		sprite->frames = (MetaSpriteFrame_t **)_omnilib_realloc(sprite->frames, sprite->maxFrames * sizeof(MetaSpriteFrame_t *));
	}

	// link and return
	frame->num = sprite->numFrames;
	sprite->frames[sprite->numFrames++] = frame;
	return frame;
}

// FreeSpriteFrame()
// free sprite frame data
void FreeSpriteFrame(MetaSpriteFrame_t *frame)
{
	_omnilib_free(frame);
}


// olSpriteAddPic()
// allocates new sprite picture and returns it
MetaSpritePic_t *olSpriteAddPic(MetaSprite_t *sprite)
{
	MetaSpritePic_t *pic;

	// alloc frame
	pic = (MetaSpritePic_t *)_omnilib_malloc(sizeof(MetaSpritePic_t));
	memset(pic, 0, sizeof(MetaSpritePic_t));

	// grow pics
	if (sprite->numPics >= sprite->maxPics)
	{
		sprite->maxPics += SPRITE_GROW_PICS;
		sprite->pics = (MetaSpritePic_t **)_omnilib_realloc(sprite->pics, sprite->maxPics * sizeof(MetaSpritePic_t *));
	}

	// link and return
	pic->num = sprite->numPics;
	sprite->pics[sprite->numPics++] = pic;
	return pic;
}

// FreeSpritePic()
// free sprite picture data
void FreeSpritePic(MetaSpritePic_t *pic)
{
	if (pic->pixels)
		_omnilib_free(pic->pixels);
	_omnilib_free(pic);
}

// olSpritePicResize()
// allocates pixels for sprite pic
void olSpritePicResize(MetaSpritePic_t *pic, int width, int height, int bpp)
{
	// already allocated?
	if (pic->width == width && pic->height == height && pic->bpp == bpp && pic->pixels)
		return;

	// free old data
	if (pic->pixels)
	{
		_omnilib_free(pic->pixels);
		pic->pixels = NULL;
	}

	// allocate new data
	pic->width = width;
	pic->height = height;
	pic->bpp = bpp;
	pic->pixels = (unsigned char *)_omnilib_malloc(width * height * bpp);
	memset(pic->pixels, 0, width * height * bpp);
}

// olSpriteAddColormap()
// allocates new colormap picture and returns it
MetaSpriteColormap_t *olSpriteAddColormap(MetaSprite_t *sprite)
{
	MetaSpriteColormap_t *cm;

	// alloc frame
	cm = (MetaSpriteColormap_t *)_omnilib_malloc(sizeof(MetaSpriteColormap_t));
	memset(cm, 0, sizeof(MetaSpriteColormap_t));

	// grow colormaps
	if (sprite->maxColormaps >= sprite->maxColormaps)
	{
		sprite->maxColormaps += SPRITE_GROW_COLORMAPS;
		sprite->colormaps = (MetaSpriteColormap_t **)_omnilib_realloc(sprite->colormaps, sprite->maxColormaps * sizeof(MetaSpriteColormap_t *));
	}

	// link and return
	cm->num = sprite->numColormaps;
	sprite->colormaps[sprite->numColormaps++] = cm;
	return cm;
}

// olFreeSprite()
// free sprite plus allocated stuff
void olFreeSprite(MetaSprite_t *sprite)
{
	int i;

	// free pics
	for (i = 0; i < sprite->numPics; i++)
		FreeSpritePic(sprite->pics[i]);
	// free frames
	for (i = 0; i < sprite->numFrames; i++)
		FreeSpriteFrame(sprite->frames[i]);
	// free sprite
	_omnilib_free(sprite->pics);
	_omnilib_free(sprite->frames);
	_omnilib_free(sprite);
}

/*
==========================================================================================

  LOAD SPRITE

==========================================================================================
*/

unsigned char quake_palette[1024] = { 0,0,0,255,15,15,15,255,31,31,31,255,47,47,47,255,63,63,63,255,75,75,75,255,91,91,91,255,107,107,107,255,123,123,123,255,139,139,139,255,155,155,155,255,171,171,171,255,187,187,187,255,203,203,203,255,219,219,219,255,235,235,235,255,15,11,7,255,23,15,11,255,31,23,11,255,39,27,15,255,47,35,19,255,55,43,23,255,63,47,23,255,75,55,27,255,83,59,27,255,91,67,31,255,99,75,31,255,107,83,31,255,115,87,31,255,123,95,35,255,131,103,35,255,143,111,35,255,11,11,15,255,19,19,27,255,27,27,39,255,39,39,51,255,47,47,63,255,55,55,75,255,63,63,87,255,71,71,103,255,79,79,115,255,91,91,127,255,99,99,139,255,107,107,151,255,115,115,163,255,123,123,175,255,131,131,187,255,139,139,203,255,0,0,0,255,7,7,0,255,11,11,0,255,19,19,0,255,27,27,0,255,35,35,0,255,43,43,7,255,47,47,7,255,55,55,7,255,63,63,7,255,71,71,7,255,75,75,11,255,83,83,11,255,91,91,11,255,99,99,11,255,107,107,15,255,7,0,0,255,15,0,0,255,23,0,0,255,31,0,0,255,39,0,0,255,47,0,0,255,55,0,0,255,63,0,0,255,71,0,0,255,79,0,0,255,87,0,0,255,95,0,0,255,103,0,0,255,111,0,0,255,119,0,0,255,127,0,0,255,19,19,0,255,27,27,0,255,35,35,0,255,47,43,0,255,55,47,0,255,67,55,0,255,75,59,7,255,87,67,7,255,95,71,7,255,107,75,11,255,119,83,15,255,131,87,19,255,139,91,19,255,151,95,27,255,163,99,31,255,175,103,35,255,35,19,7,255,47,23,11,255,59,31,15,255,75,35,19,255,87,43,23,255,99,47,31,255,115,55,35,255,127,59,43,255,143,67,51,255,159,79,51,255,175,99,47,255,191,119,47,255,207,143,43,255,223,171,39,255,239,203,31,255,255,243,27,255,11,7,0,255,27,19,0,255,43,35,15,255,55,43,19,255,71,51,27,255,83,55,35,255,99,63,43,255,111,71,51,255,127,83,63,255,139,95,71,255,155,107,83,255,167,123,95,255,183,135,107,255,195,147,123,255,211,163,139,255,227,179,151,255,171,139,163,255,159,127,151,255,147,115,135,255,139,103,123,255,127,91,111,255,119,83,99,255,107,75,87,255,95,63,75,255,87,55,67,255,75,47,55,255,67,39,47,255,55,31,35,255,43,23,27,255,35,19,19,255,23,11,11,255,15,7,7,255,187,115,159,255,175,107,143,255,163,95,131,255,151,87,119,255,139,79,107,255,127,75,95,255,115,67,83,255,107,59,75,255,95,51,63,255,83,43,55,255,71,35,43,255,59,31,35,255,47,23,27,255,35,19,19,255,23,11,11,255,15,7,7,255,219,195,187,255,203,179,167,255,191,163,155,255,175,151,139,255,163,135,123,255,151,123,111,255,135,111,95,255,123,99,83,255,107,87,71,255,95,75,59,255,83,63,51,255,67,51,39,255,55,43,31,255,39,31,23,255,27,19,15,255,15,11,7,255,111,131,123,255,103,123,111,255,95,115,103,255,87,107,95,255,79,99,87,255,71,91,79,255,63,83,71,255,55,75,63,255,47,67,55,255,43,59,47,255,35,51,39,255,31,43,31,255,23,35,23,255,15,27,19,255,11,19,11,255,7,11,7,255,255,243,27,255,239,223,23,255,219,203,19,255,203,183,15,255,187,167,15,255,171,151,11,255,155,131,7,255,139,115,7,255,123,99,7,255,107,83,0,255,91,71,0,255,75,55,0,255,59,43,0,255,43,31,0,255,27,15,0,255,11,7,0,255,0,0,255,255,11,11,239,255,19,19,223,255,27,27,207,255,35,35,191,255,43,43,175,255,47,47,159,255,47,47,143,255,47,47,127,255,47,47,111,255,47,47,95,255,43,43,79,255,35,35,63,255,27,27,47,255,19,19,31,255,11,11,15,255,43,0,0,255,59,0,0,255,75,7,0,255,95,7,0,255,111,15,0,255,127,23,7,255,147,31,7,255,163,39,11,255,183,51,15,255,195,75,27,255,207,99,43,255,219,127,59,255,227,151,79,255,231,171,95,255,239,191,119,255,247,211,139,255,167,123,59,255,183,155,55,255,199,195,55,255,231,227,87,255,127,191,255,255,171,231,255,255,215,255,255,255,103,0,0,255,139,0,0,255,179,0,0,255,215,0,0,255,255,0,0,255,255,243,147,255,255,247,199,255,255,255,255,255,159,91,83,0 };

// olLoadSprite()
// load up sprite from buffer
// todo: more checks for buffer overrun
MetaSprite_t *olLoadSprite(unsigned char *buffer, int bufsize)
{
	int i, group, npic, ncolormaps, ncm;
	MetaSprite_t *sprite;
	MetaSpriteFrame_t *frame;
	MetaSpriteColormap_t *cm;
	MetaSpritePic_t *pic;
	unsigned char *buf;

	#define expectbytes(s) if (bufsize < s) { strcpy(sprite->errormsg, "unexpected EOF when reading file"); return sprite; }

	// allocate sprite
	sprite = (MetaSprite_t *)_omnilib_malloc(sizeof(MetaSprite_t));
	memset(sprite, 0, sizeof(MetaSprite_t));

	// load header
	buf = buffer;
	if (!memcmp(buf, FOURCC_QUAKESPRITE, 4))
	{
		expectbytes(36)
		memcpy(sprite->fourCC, buf, 4);
		sprite->version = LittleInt(buf + 4);
		sprite->type = LittleInt(buf + 8);
		sprite->radius = LittleFloat(buf + 12);
		sprite->maxwidth = LittleInt(buf + 16);
		sprite->maxheight = LittleInt(buf + 20);
		sprite->nframes = LittleInt(buf + 24);
		sprite->beamlength = LittleFloat(buf + 28);
		sprite->synchtype = LittleInt(buf + 32);
		buf += 36;
		bufsize -= 36;
	}
	else
	{
		char fourccs[5];
		memcpy(fourccs, buf, 4);
		fourccs[4] = 0;
		sprintf(sprite->errormsg, "fourCC %s not supported", fourccs);
		return sprite;
	}

	// check sprite version
	if (sprite->version == SPR_QUAKE)
	{
		// allocate a quake colormap
		cm = olSpriteAddColormap(sprite);
		memcpy(cm->palette, quake_palette, 1024);

		// load frames/pictures
		for (i = 0; i < sprite->nframes; i++)
		{
			expectbytes(20)
			// read frame/framegroup
			group = LittleInt(buf);
			if (group)
			{
				if (group != 1)
				{
					sprintf(sprite->errormsg, "unexpected groupflag 0x%X on frame %i", group, i);
					return sprite;
				}
				strcpy(sprite->errormsg, "framegroups not implemented");
				return sprite;
			}
			// read frame
			frame = olSpriteAddFrame(sprite);
			frame->ofsx = LittleInt(buf + 4);
			frame->ofsy = LittleInt(buf + 8);
			frame->width = LittleInt(buf + 12);
			frame->height = LittleInt(buf + 16);
			buf += 20;
			bufsize -= 20;
			// read picture
			pic = olSpriteAddPic(sprite);
			olSpritePicResize(pic, frame->width, frame->height, 1);
			expectbytes(pic->width*pic->height*pic->bpp)
			memcpy(pic->pixels, buf, pic->width*pic->height*pic->bpp);
			pic->colormap = cm;
			frame->pic = pic;
			buf += pic->width*pic->height*pic->bpp;
			bufsize -= pic->width*pic->height*pic->bpp;
		}
	}
	else if (sprite->version == SPR_SPRITE32)
	{
		// load frames/pictures
		for (i = 0; i < sprite->nframes; i++)
		{
			expectbytes(20)
			// read frame/framegroup
			group = LittleInt(buf);
			if (group)
			{
				if (group != 1)
				{
					sprintf(sprite->errormsg, "unexpected groupflag 0x%X on frame %i", group, i);
					return sprite;
				}
				strcpy(sprite->errormsg, "framegroups not implemented");
				return sprite;
			}
			// read frame
			frame = olSpriteAddFrame(sprite);
			frame->ofsx = LittleInt(buf + 4);
			frame->ofsy = LittleInt(buf + 8);
			frame->width = LittleInt(buf + 12);
			frame->height = LittleInt(buf + 16);
			buf += 20;
			bufsize -= 20;
			// read picture
			pic = olSpriteAddPic(sprite);
			olSpritePicResize(pic, frame->width, frame->height, 4);
			expectbytes(pic->width*pic->height*pic->bpp)
			memcpy(pic->pixels, buf, pic->width*pic->height*pic->bpp);
			frame->pic = pic;
			buf += pic->width*pic->height*pic->bpp;
			bufsize -= pic->width*pic->height*pic->bpp;
		}
	}
	else if (sprite->version == SPR_PACKED || sprite->version == SPR_PACKED32)
	{
		// read scale
		expectbytes(8)
		sprite->scalex = LittleFloat(buf);
		sprite->scaley = LittleFloat(buf + 4);
		buf += 8;
		bufsize -= 8;
		// read colormaps
		expectbytes(4)
		ncolormaps = LittleInt(buf);
		buf += 4;
		bufsize -= 4;
		for (i = 0; i < ncolormaps; i++)
		{
			cm = olSpriteAddColormap(sprite);
			expectbytes(1024)
			memcpy(cm->palette, buf, 1024);
			buf += 1024;
			bufsize -= 1024;
		}
		// read pictures
		expectbytes(4)
		npic = LittleInt(buf);
		buf += 4;
		bufsize -= 4;
		for (i = 0; i < npic; i++)
		{
			pic = olSpriteAddPic(sprite);
			expectbytes(12)
			olSpritePicResize(pic, LittleInt(buf), LittleInt(buf + 4), (sprite->version == SPR_PACKED32) ? 4 : 1);
			ncm = LittleInt(buf + 8);
			if (ncm >= sprite->numColormaps)
			{
				sprintf(sprite->errormsg, "no such colormap %i on pic %i", ncm, i);
				return sprite;
			}
			if (ncm >= 0)
				pic->colormap = sprite->colormaps[ncm];
			buf += 12;
			bufsize -= 12;
			expectbytes(pic->width*pic->height*pic->bpp)
			memcpy(pic->pixels, buf, pic->width*pic->height*pic->bpp);
			buf += pic->width*pic->height*pic->bpp;
			bufsize -= pic->width*pic->height*pic->bpp;
		}
		// read frames/framegroups
		for (i = 0; i < sprite->nframes; i++)
		{
			expectbytes(32)
			// read frame
			group = LittleInt(buf);
			if (group)
			{
				if (group != 1)
				{
					sprintf(sprite->errormsg, "unexpected groupflag 0x%X on frame %i", group, i);
					return sprite;
				}
				strcpy(sprite->errormsg, "framegroups not implemented");
				return sprite;
			}
			else
			{
				frame = olSpriteAddFrame(sprite);
				frame->ofsx = LittleInt(buf + 4);
				frame->ofsy = LittleInt(buf + 8);
				frame->width = LittleInt(buf + 12);
				frame->height = LittleInt(buf + 16);
				npic = LittleInt(buf + 20);
				if (npic < 0 || npic >= sprite->numPics)
				{
					sprintf(sprite->errormsg, "no such picture %i on frame %i", npic, i);
					return sprite;
				}
				frame->picposx = LittleInt(buf + 24);
				frame->picposy = LittleInt(buf + 28);
				frame->pic = sprite->pics[npic];
				buf += 32;
				bufsize -= 32;
			}
		}
	}
	else
		sprintf(sprite->errormsg, "unsupported sprite version %i", sprite->version);
	
	// check if any frames/pictures was loaded
	if (sprite->numPics <= 0 || sprite->numFrames <= 0) 
	{
		sprintf(sprite->errormsg, "no frames/pics");
		return sprite;
	}

	// ok
	return sprite;

	#undef expectbytes
}

/*
==========================================================================================

  SAVE SPRITE

==========================================================================================
*/

// olSpriteSave()
// save sprite to buffer
int olSpriteSave(MetaSprite_t *sprite, unsigned char **bufferptr)
{
	MetaSpritePic_t *pic;
	MetaSpriteFrame_t *frame;
	_omnilib_membuf_t *buf;
	int i;

	// check consistency
	if (sprite->numFrames <= 0)
		_omnilib_error("olSpriteExport: no frames!");
	if (sprite->numPics <= 0)
		_omnilib_error("olSpriteExport: no pics!");

	// check sprite version
	if (sprite->version != SPR_QUAKE && 
		sprite->version != SPR_SPRITE32 && 
		sprite->version != SPR_PACKED && 
		sprite->version != SPR_PACKED32)
		_omnilib_error("olSpriteSave: unsupported sprite version %i", sprite->version);

	// calc max width/height
	CalcMaxWidthHeight(sprite, &sprite->maxwidth, &sprite->maxheight, &sprite->radius);

	// write
	buf = _omnilib_bcreate(1024);
	if (!memcmp(sprite->fourCC, FOURCC_QUAKESPRITE, 4))
	{
		_omnilib_bwrite(buf, sprite->fourCC, 4);
		_omnilib_bputlittleint(buf, sprite->version);
		_omnilib_bputlittleint(buf, sprite->type);
		_omnilib_bputlittlefloat(buf, sprite->radius);
		_omnilib_bputlittleint(buf, sprite->maxwidth);
		_omnilib_bputlittleint(buf, sprite->maxheight);
		_omnilib_bputlittleint(buf, sprite->numFrames);
		_omnilib_bputlittlefloat(buf, sprite->beamlength);
		_omnilib_bputlittleint(buf, sprite->synchtype);
	}
	else
	{
		char fourccs[5];
		memcpy(fourccs, sprite->fourCC, 4);
		fourccs[4] = 0;
		_omnilib_error("olSpriteSave: fourCC %s not supported", fourccs);
	}

	// write new packed sprite format
	if (sprite->version == SPR_PACKED || sprite->version == SPR_PACKED32)
	{
		// write sub-header
		_omnilib_bputlittlefloat(buf, sprite->scalex);
		_omnilib_bputlittlefloat(buf, sprite->scaley);
		// write colormaps
		_omnilib_bputlittleint(buf, sprite->numColormaps);
		for (i = 0; i < sprite->numColormaps; i++)
			_omnilib_bwrite(buf, sprite->colormaps[i]->palette, 1024);
		// write pics
		_omnilib_bputlittleint(buf, sprite->numPics);
		for (i = 0; i < sprite->numPics; i++)
		{
			pic = sprite->pics[i];
			_omnilib_bputlittleint(buf, pic->width);
			_omnilib_bputlittleint(buf, pic->height);
			_omnilib_bputlittleint(buf, (pic->colormap != NULL) ? pic->colormap->num : -1 );
			_omnilib_bwrite(buf, pic->pixels, pic->width * pic->height * pic->bpp);
		}
		// write frames
		for (i = 0; i < sprite->numFrames; i++)
		{
			frame = sprite->frames[i];
			_omnilib_bputlittleint(buf, 0);
			// todo: support framegroups
			_omnilib_bputlittleint(buf, frame->ofsx);
			_omnilib_bputlittleint(buf, frame->ofsy);
			_omnilib_bputlittleint(buf, frame->width);
			_omnilib_bputlittleint(buf, frame->height);
			_omnilib_bputlittleint(buf, frame->pic->num);
			_omnilib_bputlittleint(buf, frame->picposx);
			_omnilib_bputlittleint(buf, frame->picposy);
		}
	}
	else
	{
		// SPR_SPRITE32, SPR_QUAKE
		// write frames
		for (i = 0; i < sprite->numFrames; i++)
		{
			frame = sprite->frames[i];
			_omnilib_bputlittleint(buf, 0);
			// todo: support framegroups
			_omnilib_bputlittleint(buf, frame->ofsx);
			_omnilib_bputlittleint(buf, frame->ofsy);
			_omnilib_bputlittleint(buf, frame->width);
			_omnilib_bputlittleint(buf, frame->height);
			_omnilib_bwrite(buf, frame->pic->pixels, frame->width * frame->height * frame->pic->bpp);
		}
	}
	return _omnilib_brelease(buf, (void **)bufferptr);
}

/*
==========================================================================================

  EXPORT SPRITE

==========================================================================================
*/

// olSpriteExport()
// export sprite to infofile + TGA pictures
void olSpriteExport(MetaSprite_t *sprite, char *outdir, char *outfilename)
{
	char infofilename[MAX_FPATH], picfilename[MAX_FPATH];
	FILE *infofile;
	MetaSpriteFrame_t *frame;
	MetaSpritePic_t *pic;
	MetaSpriteColormap_t *cm;
	int i;

	// check consistency
	if (sprite->numFrames <= 0)
		_omnilib_error("olSpriteExport: no frames!");
	if (sprite->numPics <= 0)
		_omnilib_error("olSpriteExport: no pics!");

	// calc max width/height
	CalcMaxWidthHeight(sprite, &sprite->maxwidth, &sprite->maxheight, &sprite->radius);

	// begin infofile
	sprintf(infofilename, "%s/%s/info.txt", outdir, outfilename);
	infofile = _omnilib_safeopen(infofilename, "w");

	// write sprite info
	fprintf(infofile, "BEGINSPRITE\n");
	fprintf(infofile, "INFOVERSION 1\n");
	fprintf(infofile, "TYPE %i\n", sprite->type);
	fprintf(infofile, "VERSION %i\n", sprite->version);
	fprintf(infofile, "SYNCTYPE %i\n", sprite->synchtype);
	fprintf(infofile, "BEAMLENGTH %i\n", sprite->beamlength);
	if (sprite->version == SPR_PACKED32)
	{
		fprintf(infofile, "SCALEX %f\n", sprite->scalex);
		fprintf(infofile, "SCALEY %f\n", sprite->scaley);
	}

	// write colormaps
	for (i = 0; i < sprite->numColormaps; i++)
	{
		cm = sprite->colormaps[i];

		// write info
		sprintf(picfilename, "colormap%04i.tga", i);
		fprintf(infofile, "BEGINCOLORMAP %i\n", cm->num);
		fprintf(infofile, "  FILE %s\n", picfilename);

		// write tga
		sprintf(picfilename, "%s/%s/colormap%04i.tga", outdir, outfilename, i);
		WriteTGA(picfilename, cm->palette, 16, 16, 4, NULL);
	}

	// write pictures
	for (i = 0; i < sprite->numPics; i++)
	{
		pic = sprite->pics[i];

		// write info
		sprintf(picfilename, "pic%04i.tga", i);
		fprintf(infofile, "BEGINPIC %i\n", pic->num);
		fprintf(infofile, "  WIDTH %i\n", pic->width);
		fprintf(infofile, "  HEIGHT %i\n", pic->height);
		fprintf(infofile, "  BPP %i\n", pic->bpp);
		fprintf(infofile, "  FILE %s\n", picfilename);
		if (pic->colormap != NULL)
			fprintf(infofile, "  COLORMAP %s\n", pic->colormap->num);

		// write tga
		sprintf(picfilename, "%s/%s/pic%04i.tga", outdir, outfilename, i);
		WriteTGA(picfilename, pic->pixels, pic->width, pic->height, pic->bpp, (pic->colormap != NULL) ? pic->colormap->palette : NULL);
	}

	// write frames
	for (i = 0; i < sprite->numFrames; i++)
	{
		frame = sprite->frames[i];

		// write info
		fprintf(infofile, "BEGINFRAME %i\n", frame->num);
		fprintf(infofile, "  X %i\n", frame->ofsx);
		fprintf(infofile, "  Y %i\n", frame->ofsy);
		fprintf(infofile, "  WIDTH %i\n", frame->width);
		fprintf(infofile, "  HEIGHT %i\n", frame->height);
		if (frame->pic)
		{
			fprintf(infofile, "  PIC %i\n", frame->pic->num);
			if (sprite->version == SPR_PACKED32)
			{
				fprintf(infofile, "  PICPOSX %i\n", frame->picposx);
				fprintf(infofile, "  PICPOSY %i\n", frame->picposy);
			}
		}
	}

	// write info
	fprintf(infofile, "ENDSPRITE\n");
}

/*
==========================================================================================

  CONVERT SINGLE SPRITE TO PACKED SPRITE

==========================================================================================
*/

// NextPowerOfTwo
// get next power of two for a number
int NextPowerOfTwo(int n) 
{ 
    if ( n <= 1 ) return n;
    double d = n-1; 
    return 1 << ((((int*)&d)[1]>>20)-1022); 
}

// structs used for merging
typedef struct
{
	int   num;
	int   nummerged;
	int   width;
	int   height;
	int   bpp;
	MetaSpriteColormap_t *colormap;
	unsigned char *pixels;
	unsigned char *fill;
	int   maxwidth;
	int   maxheight;
} MergedPic_t;
typedef struct
{
	int  posx;
	int  posy;
	int  picnum;
} MergePicMap_t;

// FreeMergedTex()
// free stuff allocated on merged texture
void FreeMergedTex(MergedPic_t *merged)
{
	if (merged->pixels)
		_omnilib_free(merged->pixels);
	merged->pixels = NULL;
	if (merged->fill)
		_omnilib_free(merged->fill);
	merged->fill = NULL;
}

// InitMergedTex()
// initialize merged texture
void InitMergedTex(MergedPic_t *merged, int width, int height, int bpp, MetaSpriteColormap_t *colormap, int maxwidth, int maxheight, bool forcesquare)
{
	FreeMergedTex(merged);

	merged->nummerged = 0;
	merged->maxwidth = NextPowerOfTwo(max(width, maxwidth));
	merged->maxheight = NextPowerOfTwo(max(height, maxheight));
	merged->width = NextPowerOfTwo(width);
	merged->height = NextPowerOfTwo(height);
	if (forcesquare)
	{
		merged->maxwidth = merged->maxheight = max(merged->maxwidth, merged->maxheight);
		merged->width = merged->height = max(merged->width, merged->height);
	}
	merged->bpp = bpp; 
	merged->colormap = colormap;
	merged->pixels = (unsigned char *)_omnilib_malloc(merged->width * merged->height * merged->bpp);
	merged->fill = (unsigned char *)_omnilib_malloc(merged->width * merged->height);
}

// FlushMergedTex()
// zero out merged texture
void FlushMergedTex(MergedPic_t *merged)
{
	memset(merged->pixels, 0, merged->width * merged->height * merged->bpp);
	memset(merged->fill, 0, merged->width * merged->height);
}

// ResizeMergedTex()
// resize merged texture to a new size, keeping old data unchanged
bool ResizeMergedTex(MergedPic_t *merged, int newwidth, int newheight)
{
	unsigned char *newpixels, *newfill, *out, *in;
	int h;

	// reached max merged pic size
	if (newwidth > merged->maxwidth || newheight > merged->maxheight)
		return false;

	// allocate new data
	newpixels = (unsigned char *)_omnilib_malloc(newwidth * newheight * merged->bpp);
	memset(newpixels, 0, newwidth * newheight * merged->bpp);
	newfill = (unsigned char *)_omnilib_malloc(newwidth * newheight);
	memset(newfill, 0, newwidth * newheight);

	// copy data
	for (h = 0; h < merged->height; h++)
	{
		// copy pixels line
		in = merged->pixels + merged->width * h * merged->bpp;
		out = newpixels + newwidth * h * merged->bpp;
		memcpy(out, in, merged->width * merged->bpp);
		// copy fill line
		in = merged->fill + merged->width * h;
		out = newfill + newwidth * h;
		memcpy(out, in, merged->width);
	}

	// replace
	_omnilib_free(merged->pixels);
	_omnilib_free(merged->fill);
	merged->pixels = newpixels;
	merged->fill = newfill;
	merged->width = newwidth;
	merged->height = newheight;
	return true;
}

// StoreMergeTex()
// store merged texture as new pic of the sprite
void StoreMergeTex(MergedPic_t *merged, MetaSprite_t *sprite, bool debugfill, bool npot)
{
	MetaSpritePic_t *pic;
	unsigned char *rgba, *end, *fill;
	int x, y, left, top;

	// debug
	if (debugfill && merged->bpp == 4)
	{
		rgba = merged->pixels;
		fill = merged->fill;
		end = rgba + merged->width * merged->height * merged->bpp;
		while(rgba < end)
		{
			if (!*fill)
			{
				rgba[0] = 0;
				rgba[1] = 255;
				rgba[2] = 255;
				rgba[3] = 255;
			}
			fill += 1;
			rgba += 4;
		}
	}

	// store
	pic = olSpriteAddPic(sprite);
	pic->colormap = merged->colormap;
	if (pic->num != merged->num)
		_omnilib_error("StoreMergeTex: pic->num != merged->num!");
	if (npot)
	{
		// find out borders
		left = 1;
		top = 1;
		for(y = 0; y < merged->height; y++)
		{
			fill = merged->fill + y*merged->width;
			for (x = merged->width - 1; x > 0; x--)
			{	
				if (fill[x])
				{	
					left = max(left, x + 1);
					top = max(top, y + 1);
					break;
				}
			}
		}
		left = min(left + 1, merged->width - 1);
		top = min(top + 1, merged->height - 1);

		// only allow shrinking that saves at least 25% of height/width, in other case we just got too much exaggeration when converting to power-of-two-size
		// for example: a width of 511 pixels would kill the details
		if (((float)left / float(merged->width)) > 0.75f)
			left = merged->width;
		if (((float)top / float(merged->height)) > 0.75f)
			top = merged->height;
		
		// copy out only filled zone
		olSpritePicResize(pic, left, top, merged->bpp);
		rgba = merged->pixels;
		fill = pic->pixels;
		end = fill + pic->height*pic->width*pic->bpp;
		while(fill < end)
		{
			memcpy(fill, rgba, pic->width*pic->bpp);
			rgba += merged->width*merged->bpp;
			fill += pic->width*pic->bpp;
		}
	}
	else
	{
		// store as is
		olSpritePicResize(pic, merged->width, merged->height, merged->bpp);
		memcpy(pic->pixels, merged->pixels, pic->width*pic->height*pic->bpp);
	}
}

// FindPicPos()
// find out position of sprite on merged texture
bool FindPicPos(MergedPic_t *merged, MetaSpritePic_t *pic, int border, int *foundposx, int *foundposy, bool debugborders)
{
	int pwidth, pheight, x, y, h, w;
	unsigned char *out, *in, *end;
	bool failed;

	pwidth = pic->width + border * 2;
	pheight = pic->height + border * 2;
	for (y = 0; y < (merged->height - pheight); y++)
	{
		for (x = 0; x < (merged->width - pwidth); x++)
		{
			if (merged->fill[merged->width * y + x])
				continue;

			// trace if we can use this pos
			failed = false;
			for (h = 0; h < pheight && !failed; h++)
			{
				in = merged->fill + merged->width * (y + h) + x;
				for (w = 0; w < pwidth && !failed; w++)
					if (in[w])
						failed = true;
			}
			if (!failed)
			{
				// place the pic
				// up border
				for (h = 0; h < border; h++)
				{
					// set fill
					out = merged->fill + merged->width * (y + h) + x;
					memset(out, 1, pwidth);
				}
				if (debugborders && merged->bpp == 4)
				{
					for (h = 0; h < border; h++)
					{
						// debug fill
						out = merged->pixels + (merged->width * (y + h) + x) * merged->bpp ;
						end = out + pwidth * merged->bpp;
						while(out < end)
						{
							out[0] = 255;
							out[1] = 0;
							out[2] = 255;
							out[3] = 255;
							out += 4;
						}
					}
				}
				// pic
				for (h = 0; h < pic->height; h++)
				{
					out = merged->pixels + ( merged->width * (y + border + h) + x ) * merged->bpp;
					if (debugborders && merged->bpp == 4)
					{
						// debug fill
						for (w = 0; w < border; w++)
						{
							out[0] = 255;
							out[1] = 0;
							out[2] = 255;
							out[3] = 255;
							out += 4;
						}
					}
					else
						out += merged->bpp * border;
					// copy rgb data
					in = pic->pixels + pic->width * h * pic->bpp;
					memcpy(out, in, pic->width * pic->bpp);
					if (debugborders && merged->bpp == 4)
					{
						// debug fill
						out += pic->width * merged->bpp;
						for (w = 0; w < border; w++)
						{
							out[0] = 255;
							out[1] = 0;
							out[2] = 255;
							out[3] = 255;
							out += 4;
						}
					}
					// set fill
					out = merged->fill + merged->width * (y + border + h) + x;
					memset(out, 1, pwidth);
				}
				// bottom border
				for (h = 0; h < border; h++)
				{
					// set fill
					out = merged->fill + merged->width * (y + border + pic->height + h) + x;
					memset(out, 1, pwidth);
				}
				if (debugborders && merged->bpp == 4)
				{
					for (h = 0; h < border; h++)
					{
						// debug fill
						out = merged->pixels + (merged->width * (y + border + pic->height + h) + x) * merged->bpp;
						end = out + pwidth * merged->bpp;
						while(out < end)
						{
							out[0] = 255;
							out[1] = 0;
							out[2] = 255;
							out[3] = 255;
							out += 4;
						}
					}
				}
				*foundposx = x + border;
				*foundposy = y + border;
				return true;
			}
		}
	}

	// failed to find position
	return false;
}

// CompareSpritePic
// compare function for qsort
int CompareSpritePic( const void *a, const void *b )
{
	MetaSpritePic_t *pica, *picb;

	// get pics
	pica = *((MetaSpritePic_t **)a);
	picb = *((MetaSpritePic_t **)b);

	// compare size
	return (pica->height - picb->height);
}

// olSpriteConvertToPacked()
// convert sprite to paged frames sprite
MetaSprite_t *olSpriteConvertToPacked(MetaSprite_t *sprite, int border, int maxpicwidth, int maxpicheight, bool forcesquare, bool debugfill, bool debugborders, bool nosort, bool npot, SpritePackMode_t mode)
{
	MetaSpritePic_t **pics, *pic, *pic2;
	MetaSpriteFrame_t *frame, *frame2;
	MetaSpriteColormap_t *cm, *cm2;
	MergedPic_t merged = { 0, 0, 0, 0, 0 };
	MergePicMap_t *picmaps;
	MetaSprite_t *sprite2;
	int i, j, foundposx, foundposy;
	bool resized;

	// sanity checks
	if (sprite->version == SPR_PACKED || sprite->version == SPR_PACKED32)
		return sprite;
	if (sprite->version != SPR_QUAKE && 
		sprite->version != SPR_SPRITE32)
		_omnilib_error("olSpriteConvertToPacked: unsupported sprite version %i", sprite->version);

	// create new sprite
	sprite2 = olCreateSprite();
	sprite2->fourCC[0] = sprite->fourCC[0];
	sprite2->fourCC[1] = sprite->fourCC[1];
	sprite2->fourCC[2] = sprite->fourCC[2];
	sprite2->fourCC[3] = sprite->fourCC[3];
	sprite2->version = (sprite->version == SPR_QUAKE) ? SPR_PACKED : SPR_PACKED32;
	sprite2->type = sprite->type;
	sprite2->radius = sprite->radius;
	sprite2->maxwidth = sprite->maxwidth;
	sprite2->maxheight = sprite->maxheight;
	sprite2->nframes = sprite->nframes;
	sprite2->beamlength = sprite->beamlength;
	sprite2->synchtype = sprite->synchtype;

	// copy colormaps
	for (i = 0; i < sprite->numColormaps; i++)
	{
		cm = sprite->colormaps[i];
		cm2 = olSpriteAddColormap(sprite2);
		memcpy(cm2->palette, cm->palette, 1024);
	}

	// copy and sort pics
	pics = (MetaSpritePic_t **)_omnilib_malloc(sizeof(MetaSpritePic_t *)*sprite->numPics);
	memcpy(pics, sprite->pics, sizeof(MetaSpritePic_t *)*sprite->numPics);
	if (!nosort)
		qsort(pics, sprite->numPics, sizeof(MetaSpritePic_t *), CompareSpritePic);

	// merge texture for each colormap
	cm = sprite->numColormaps ? sprite->colormaps[0] : NULL;
	while(1)
	{
		// allocate merged pic, init merge map
		picmaps = (MergePicMap_t *)_omnilib_malloc(sizeof(MergePicMap_t) * sprite->numPics);
		for (i = 0; i < sprite->numPics; i++)
			picmaps[i].picnum = -1;
		InitMergedTex(&merged, pics[0]->width, pics[0]->height, pics[0]->bpp, cm, maxpicwidth, maxpicheight, forcesquare );
normal_try:
		FlushMergedTex(&merged);

		// perform merge tests
		for (i = 0; i < sprite->numPics; i++)
		{
			pic = pics[i];
			if (picmaps[pic->num].picnum >= 0 || pic->colormap != cm)
				continue;

			//printf("merge pic %i of %i (%ix%i)\n", i, sprite->numPics, pic->width, pic->height);
			// check if we can fit this pic on target texture
fast_try:
			if (FindPicPos(&merged, pic, border, &foundposx, &foundposy, debugborders))
			{
				//printf("found pos %i %i\n", foundposx, foundposy);
				picmaps[pic->num].posx = foundposx;
				picmaps[pic->num].posy = foundposy;
				picmaps[pic->num].picnum = merged.num;
				merged.nummerged++;
				continue;
			}

			// fast mode: before trying resize, try another pics to be stored
			if (mode == SPR_PACK_FAST)
			{
				for (j = 0; j < sprite->numPics; j++)
				{
					pic2 = pics[j];
					if (picmaps[pic2->num].picnum >= 0 || pic->colormap != cm || pic == pic2)
						continue;
					if (FindPicPos(&merged, pic2, border, &foundposx, &foundposy, debugborders))
					{
						//printf("found pos for another pic %i %i\n", foundposx, foundposy);
						picmaps[pic2->num].posx = foundposx;
						picmaps[pic2->num].posy = foundposy;
						picmaps[pic2->num].picnum = merged.num;
						merged.nummerged++;
					}
				}
			}

			// try resize
			// try 2x width, then 2x height
			if ( forcesquare )
			{
				//printf("resize merge texture -> %ix%i\n", merged.width * 2, merged.height * 2);
				resized = ResizeMergedTex(&merged, merged.width * 2, merged.height * 2);
			}
			else if ( merged.width <= merged.height )
			{
				//printf("resize merge texture -> %ix%i\n", merged.width * 2, merged.height );
				resized = ResizeMergedTex(&merged, merged.width * 2, merged.height);
			}
			else
			{
				//printf("resize merge texture -> %ix%i\n", merged.width, merged.height * 2);
				resized = ResizeMergedTex(&merged, merged.width, merged.height * 2);
			}
			if (resized)
			{
				if (mode == SPR_PACK_FAST)
					goto fast_try; // fast mode: try to add rest of pics
				// normal mode: try from the very beginning (better control over wasted space)
				for (j = 0; j < sprite->numPics; j++)
					if (picmaps[j].picnum == merged.num)
						picmaps[j].picnum = -1;
				goto normal_try;
			}

			// move data to merged pic
			//printf("store merge texture %i\n", merged.num);
			StoreMergeTex(&merged, sprite2, debugfill, npot);

			// new merged pic
			merged.num++;
			InitMergedTex(&merged,  pic->width, pic->height, pic->bpp, cm, maxpicwidth, maxpicheight, forcesquare );
			//printf("new merge texture %i\n", merged.num);
			goto fast_try;
		}

		// move rest of stuff
		if (merged.nummerged)
			StoreMergeTex(&merged, sprite2, debugfill, npot);
		FreeMergedTex(&merged);

		// next colormap
		if (cm == NULL)
			break;
		if ((cm->num + 1) >= sprite->numColormaps)
			break;
		cm = sprite->colormaps[cm->num + 1];
	}

	// now copy frames
	for (i = 0; i < sprite->numFrames; i++)
	{
		frame = sprite->frames[i];
		frame2 = olSpriteAddFrame(sprite2);

		// copy
		memcpy(frame2, frame, sizeof(MetaSpriteFrame_t));
		frame2->picposx = picmaps[frame->pic->num].posx;
		frame2->picposy = picmaps[frame->pic->num].posy;
		frame2->pic = sprite2->pics[picmaps[frame->pic->num].picnum];
	}
	_omnilib_free(picmaps);

	// return new sprite
	return sprite2;
}

/*
==========================================================================================

  CONVERT PACKED SPRITE TO SINGLE SPRITE

==========================================================================================
*/

// olSpriteConvertToSingle()
// convert sprite to single-frame format (one pic for each frame)
MetaSprite_t *olSpriteConvertToSingle(MetaSprite_t *sprite)
{
	MetaSprite_t *sprite2;
	MetaSpriteFrame_t *frame, *frame2;
	MetaSpritePic_t *pic, *pic2;
	MetaSpriteColormap_t *cm, *cm2;
	int i, y;

	// sanity checks
	if (sprite->version == SPR_QUAKE || sprite->version == SPR_SPRITE32)
		return sprite;
	if (sprite->version != SPR_PACKED && 
		sprite->version != SPR_PACKED32)
		_omnilib_error("olSpriteConvertToSingle: unsupported sprite version %i", sprite->version);

	// create new sprite
	sprite2 = olCreateSprite();
	sprite2->fourCC[0] = sprite->fourCC[0];
	sprite2->fourCC[1] = sprite->fourCC[1];
	sprite2->fourCC[2] = sprite->fourCC[2];
	sprite2->fourCC[3] = sprite->fourCC[3];
	sprite2->version = (sprite->version == SPR_PACKED32) ? SPR_SPRITE32 : SPR_QUAKE;
	sprite2->type = sprite->type;
	sprite2->radius = sprite->radius;
	sprite2->maxwidth = sprite->maxwidth;
	sprite2->maxheight = sprite->maxheight;
	sprite2->nframes = sprite->nframes;
	sprite2->beamlength = sprite->beamlength;
	sprite2->synchtype = sprite->synchtype;

	// copy colormaps
	for (i = 0; i < sprite->numColormaps; i++)
	{
		cm = sprite->colormaps[i];
		cm2 = olSpriteAddColormap(sprite2);
		memcpy(cm2->palette, cm->palette, 1024);
	}

	// copy frames creating a new pic for each
	for (i = 0; i < sprite->numFrames; i++)
	{
		frame = sprite->frames[i];
		pic = frame->pic;

		// create frame
		frame2 = olSpriteAddFrame(sprite2);
		memcpy(frame2, frame, sizeof(MetaSpriteFrame_t));
		frame2->picposx = 0;
		frame2->picposy = 0;
		frame2->pic = olSpriteAddPic(sprite2);
		pic2 = frame2->pic;
		
		// copy pic
		olSpritePicResize(pic2, frame2->width, frame2->height, pic->bpp);
		pic2->colormap = pic->colormap;
		for (y = 0; y < pic2->height; y++)
			memcpy(pic2->pixels + y*pic2->width*pic2->bpp, pic->pixels + (frame->picposy + y)*pic->width*pic->bpp + frame->picposx*pic->bpp, frame2->width*pic2->bpp);
	}

	// return new sprite
	return sprite2;
}

/*
==========================================================================================

  OPTIMIZE SPRITE

==========================================================================================
*/

// olSpritePicFloodAlpha
// see olSpriteFloodAlpha
void olSpritePicFloodAlpha(MetaSpritePic_t *pic, int passes)
{
	byte *sampledPixels, *sampledPixelsMap, *sampledMap, *pixel;
	int x, y, pass, swidth, sheight;
	float sample[3], samples;
	size_t size;

	// todo: support BPP4
	// check sanity
	if (pic->bpp != 4)
		_omnilib_error("olSpritePicFloodAlpha: bpp %i not supported\n", pic->bpp);
	if (pic->pixels == NULL)
		_omnilib_error("olSpritePicFloodAlpha: pixels not allocated\n");

	// allocate sampled map
	size = pic->width * pic->height * pic->bpp;
	swidth = pic->width - 1;
	sheight = pic->height - 1;
	sampledPixels = (byte *)_omnilib_malloc(size);
	sampledPixelsMap = (byte *)_omnilib_malloc(pic->width * pic->height);
	sampledMap = (byte *)_omnilib_malloc(pic->width * pic->height);

	// fill sampled map
	for( y = 0; y < pic->height; y++ )
		for( x = 0; x < pic->width; x++ )
			sampledMap[y * pic->width + x] = pic->pixels[(y * pic->width + x) * pic->bpp + 3];

	// multiple pases
	for ( pass = 0; pass < passes; pass++ )
	{
		memcpy( sampledPixels, pic->pixels, size);
		memcpy( sampledPixelsMap, sampledMap, pic->width * pic->height);

		// walk all pixels
		#define addSample(ofsx,ofsy,scale) { if (sampledPixelsMap[(y ofsy) * pic->width + x ofsx] != 0) { pixel = pic->pixels + ((y ofsy) * pic->width + x ofsx) * pic->bpp; sample[0] += (float)pixel[0] * scale; sample[1] += (float)pixel[1] * scale; sample[2] += (float)pixel[2] * scale; samples += scale; } }
		for( y = 0; y < pic->height; y++ )
		{
			for( x = 0; x < pic->width; x++ )
			{
				// only process unsampled pixels
				if (sampledPixelsMap[y * pic->width + x] != 0)
					continue;

				// gather samples from nearest pixels
				samples = 0;
				sample[0] = 0.0f;
				sample[1] = 0.0f;
				sample[2] = 0.0f;

				// sample top
				if ( y > 0 )
				{
					// left top
					if ( x > 0 )
						addSample(-1, -1, 0.7f)
					// center top
					addSample(+0, -1, 1.0f)
					// right top
					if ( x < swidth )
						addSample(+1, -1, 0.7f)
				}
				
				// sample left
				if ( x > 0 )
					addSample(-1, +0, 1.0f)
				// sample right
				if ( x < swidth )
					addSample(+1, +0, 1.0f)
				
				// sample bottom
				if ( y < sheight )
				{
					// left top
					if ( x > 0 )
						addSample(-1, +1, 0.7f)
					// center top
					addSample(+0, +1, 1.0f)
					// right top
					if ( x < swidth )
						addSample(+1, +1, 0.7f)
				}

				// subsample
				if ( !samples )
					continue;
				pixel = sampledPixels + (y * pic->width + x) * pic->bpp;
				if ( samples != 1 )
				{
					sample[0] /= samples;
					sample[1] /= samples;
					sample[2] /= samples;
				}
				pixel[0] = min( 255, (byte)floor(sample[0] + 0.5f) );
				pixel[1] = min( 255, (byte)floor(sample[1] + 0.5f) );
				pixel[2] = min( 255, (byte)floor(sample[2] + 0.5f) );
				sampledMap[y * pic->width + x] = 255;
			}
		}
		#undef addSample

		// store sampled
		memcpy( pic->pixels, sampledPixels, size );
	}

	// clean up
	_omnilib_free( sampledPixels );
	_omnilib_free( sampledPixelsMap );
	_omnilib_free( sampledMap );
}

// olSpriteFloodAlpha
// flood null-alpha pixels with nearest opaque pixels RGB values
// this fixes black seams on edges (caused by filtering) and also makes sprites
// more friendly to block compression techniques
void olSpriteFloodAlpha(MetaSprite_t *sprite, int passes)
{
	int i;

	for (i = 0; i < sprite->numPics; i++)
		olSpritePicFloodAlpha(sprite->pics[i], passes);
}

#if 0
// olSpriteOptimize()
// optimizes the sprite frames by cropping them and auto-filling alpha to make sprite to be more suitable for block-based compression
MetaSprite_t *olSpriteOptimize(MetaSprite_t *sprite)
{
	int x, y, xs, left0, top0, left, top;

	// get right borders
	top0 = -1;
	left0 = merged->width;
	left = 1;
	top = 1;
	for(y = 0; y < merged->height; y++)
	{
		fill = merged->fill + y*merged->width;
		for (x = merged->width - 1; x >= 0; x--)
		{	
			if (fill[x])
			{	
				left = max(left, x + 1);
				top = max(top, y + 1);
				break;
			}
		}
		if (x >= 0) // line is not empty
		{
			// get horizontal crop start
			for (xs = 0; xs <= x; xs++)
			{
				if (fill[xs])
				{
					left0 = min(left0, xs - 1);
					break;
				}
			}
			// initialize vertical crop start on first non-empty line
			if (top0 < 0)
				top0 = y - 1;
		}
	}
	left = min(left + 1, merged->width - 1);
	top = min(top + 1, merged->height - 1);
	if (top0 < 0)
		top0 = 0;

	// copy out only filled zone
	olSpritePicResize(pic, left, top, merged->bpp);
	rgba = merged->pixels;
	fill = pic->pixels;
	end = fill + pic->height*pic->width*pic->bpp;
	while(fill < end)
	{
		memcpy(fill, rgba, pic->width*pic->bpp);
		rgba += merged->width*merged->bpp;
		fill += pic->width*pic->bpp;
	}
}
#endif

}