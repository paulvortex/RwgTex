// dpomnilib.h
#ifndef F_DPOMNILIB_H
#define F_DPOMNILIB_H

namespace omnilib
{

// internal functions
void OmnilibSetMemFunc(void *(*allocfunc)(size_t), void *(*reallocfunc)(void *,size_t), void (*freefunc)(void *));
void OmnilibSetMessageFunc(void (*messagefunc)(int, char *), void (*errorfunc)(char *));

// sprite formats
typedef enum
{
	SPR_QUAKE = 1,      // Quake 8-bit paletted sprite
	SPR_HL = 2,         // Half-life sprite
	SPR_SPRITE32 = 32,  // Darkplaces RGBA sprite
	SPR_PACKED = 128,   // Blood Omnicide 8-bit paged sprite format
	SPR_PACKED32 = 160  // Blood Omnicide 32-bit paged sprite format
} QuakeSpriteVersion_t;

// sprite types
typedef enum
{
	SPR_VP_PARALLEL_UPRIGHT = 0, // flames and such, vertical beam sprite, faces view plane
	SPR_VP_FACING_UPRIGHT = 1, // flames and such, vertical beam sprite, faces viewer's origin (not the view plane)
	SPR_VP_PARALLEL = 2, // normal sprite, faces view plane
	SPR_ORIENTED = 3, // bullet marks on walls, ignores viewer entirely
	SPR_OVERHEAD = 7, // VP_PARALLEL with couple of hacks
} QuakeSpriteType_t;

// frames and framegroup
typedef enum
{
	SPR_FRAME_SINGLE = 0,
	SPR_FRAME_GROUP = 1
} QuakeSpriteFrameType_t;

// sprite colormap
typedef struct
{
	int num;
	unsigned char palette[1024];
} MetaSpriteColormap_t;

// sprite picture
typedef struct
{
	int num;
	long width;
	long height;
	int bpp;
	MetaSpriteColormap_t *colormap;
	unsigned char *pixels;
} MetaSpritePic_t;

// sprite frame
typedef struct
{
	int num;
	int ofsx;
	int ofsy;
	int width;
	int height;
	MetaSpritePic_t *pic;
	int picposx;
	int picposy;
} MetaSpriteFrame_t;

// sprite header
typedef struct
{
	// quake sprite header
	char fourCC[4];      // fourCC code
	long version;        // one of QuakeSpriteVersion_t
	long type;           // one of QuakeSpriteType_t
	float radius;        // Bounding Radius
	long maxwidth;       // Width of the largest frame
	long maxheight;      // Height of the largest frame
	long nframes;        // Number of frames
	float beamlength;    // pushs the sprite away, strange legacy from DOOM?
	long synchtype;      // 0 = synchron, 1 = random
	float scalex;        // sprite horizontal scale (SPR_PACKED/SPR_PACKED32)
	float scaley;        // sprite vertical scale (SPR_PACKED/SPR_PACKED32)

	// meta info
	char errormsg[1024]; // if set, loading failed

	// colormaps
	long numColormaps;
	long maxColormaps;
	MetaSpriteColormap_t **colormaps;

	// pictures/textures
	long numPics;
	long maxPics;
	MetaSpritePic_t **pics;

	// frames
	long numFrames;
	long maxFrames;
	MetaSpriteFrame_t **frames;
} MetaSprite_t;

// packed modes
typedef enum
{
	SPR_PACK_FAST,
	SPR_PACK_NORMAL,
	NUM_SPR_PACK_MODES
} SpritePackMode_t;

// sprite functions
MetaSprite_t         *olCreateSprite();
MetaSprite_t         *olLoadSprite(unsigned char *buf, int bufsize);
void                  olFreeSprite(MetaSprite_t *sprite);
MetaSpriteFrame_t    *olSpriteAddFrame(MetaSprite_t *sprite);
MetaSpritePic_t      *olSpriteAddPic(MetaSprite_t *sprite);
MetaSpriteColormap_t *olSpriteAddColormap(MetaSprite_t *sprite);
void                  olSpriteExport(MetaSprite_t *sprite, char *outdir, char *outfilename);
MetaSprite_t         *olSpriteConvertToPacked(MetaSprite_t *sprite, int border, int maxpicwidth, int maxpicheight, bool forcesquare, bool debugfill, bool debugborders, bool nosort, bool npot, SpritePackMode_t mode);
MetaSprite_t         *olSpriteConvertToSingle(MetaSprite_t *sprite);
int                   olSpriteSave(MetaSprite_t *sprite, unsigned char **bufferptr);
void                  olSpritePicResize(MetaSpritePic_t *pic, int width, int height, int bpp);
void                  olSpritePicFloodAlpha(MetaSpritePic_t *pic, int passes);
void                  olSpriteFloodAlpha(MetaSprite_t *sprite, int passes);

}

#endif
