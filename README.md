RwgTex is commandline texture conversion tool used to generate compressed textures package for games.

Features
------

- batch-converts textures (the target is to generate whole game textures
  package with single call)
- automatically rescales images to nearest Power-Of-Two
- detect special file types (normalmaps, heightmaps) and use best looking
  compression and compressor for them
- for textures with alpha channels, alpha is scanned to check if it can be
  represented with one bit (DXT1) or requires compression with gradiant
  alpha (DXT5)
- multiple threads to compress
- supported input files: TGA, JPG, PNG
- supported specific texture containers: SPR32, Quake 1 .BSP
- textures read/export for .ZIP archives
- wide range of options to get it run with particlular engine/mod
- supported compression formats: DXT1-5
- supported DXT swizzled formats: Doom 3 RXGB, YCoCg Unscaled, YCoCg Scaled
- support uncompressed BGRA DDS
- saves a cache of files crc32 to check if they were modified (speeds up
  compression when run next time by only compressing files that was changed)

 
Usage
------

1) rwgdds.exe <input_dir> <output_dir>
   Scan <input_dir> for tga, jpg, png, spr32 files and convert them to DDS to
   output dir. Also it will generate a filescrc.txt file holding crc32 sums for
   source files used to make DDS. So next time you will run RwgDDS with this
   output folder it will only convert files that was changed.

2) rwgdds.exe <path>
   If <path> is file, it will convert it and place in same folder. 
   If it's a directory, it will convert all files in it and place
   in dds/ subdirectory.

3) Drag&drop files or directories to .exe (it acually launches method 1 or 2)


General options
------

Options to process files are stored in .ini file. For each particular engine
opt files should be tweaked to match (basedir, what normalmaps name format
is used, other tricks etc.), there can be several .ini files for different
platforms.


Commandline parms
------

-dds       : enforces DDS file creation and texture compression
-w         : wait for key press once finished
-f         : suppress any prints
-mem       : show memory usage stats at end
-threads X : manually sets the number of threads
-cd X      : change to this dir once started
-nomip     : do not generate any mipmaps
-opt X     : load custom option file
-ati       : use ATI compressor instead of mixed mode
-nv        : use Nvidia DXTlib instead of mixed mode
-gimp      : use GIMP DDS plugin compressor
-nocache   : dont read/write filescrc.txt
-npot      : allow non-power-of-two texture dimensions
-nm        : forces texture to be processed as normalmap
-dxt1      : forces DXT1 compression
-dxt2      : forces DXT2 compression
-dxt3      : forces DXT3 compression
-dxt4      : forces DXT4 compression
-dxt5      : forces DXT5 compression
-rxgb      : forces RXGB compression
-ycg1      : forces YCoCg Unscaled compression
-ycg2      : forces YCoCg Scaled compression
-bgra      : forces BGRA DDS file creation
-ap X      : sets archive internal path for ZIP file creation
-zipmem X  : create ZIP file is memory (X is number of megabytes),
             makes compression of many files faster
-2x        : Scale texture by 2x before compression
-scaler x  : Sets scaler for 2x scaling, possible scalers: nearest, bilinear,
             bicubic, bspline, catmullrom, lanczos, scale2x (default), super2x
			 (scale 4x with backscale to 2x using lanczos filter).
-nosign    : disable DDS magic sign
-gimpsign  : enables "GIMP-DDS" magic sign which makes generated swizzled
             DDS to be readable in GIMP

Trick: commandline parms could be included to exe name, this way they become defaults.
Example: rwgdds-ati-w.exe


Compression modes
------
1. "ATI" - AMD's The Compressonator tool.
   Supported compressions: DXT1-5, Doom 3 RXGB
2. "NVidia" - NVidia DXTlib compressor.
   Supported compressions: DXT1-5, Doom 3 RXGB
3. "Hybrid" - mixed compressor which uses ATI mode for color textures (as The
   Compressonator generally gives better perceptural quality, which is good for
   color textures. NVidia DXTlib is used for normammaps and heightmaps
   (as it gives better PSNR).
   Supported compressions: DXT1-5, Doom 3 RXGB
4. "Gimp DDS" - GIMP DDS plugin.
   Supported compressions: DXT YCoCg Unscaled, DXT YCoCg Scaled, DXT1-5
 
Trick: .ini file have parms to set compression mode on a per-file basis using name masks.


Known issues
------
- Archives larger than 2GB are not supported
- When using archeves, file cache are not supported
- DXT2/DXT4 cannot be represented by KTX format without additional key-pairs (they are used to be 'swizzled' format)

--------------------------------------------------------------------------------
 Version History + Changelog
--------------------------------------------------------------------------------

1.4
- Added support for ETC compression using etcpack.
-etcpack 
-etc1
-etc2
-etc2b
-etc2s
-etc2sb
-etc2r
-etc2rg

opt:
force_etc1
force_etc2
force_etc2b
force_etc2s
force_etc2sb
force_etc2r
force_etc2rg
force_etcpack

1.3
------
- Added support for YCoCg Scaled and Unscaled compression (DXT5 with RGB
  converted to YCoCg colorspace, color component stored in alpha, optional color
  scaling (normalizing image to 0-255 space). New general options: force_ycg1,
  force_ycg2 (file masks). New commandline parm: -ycg1, -ycg2.
- New DXT compressor from GIMP DDS Plugin, can compress DXT1, DXT3, DXT5 and
  YCoCg/YCoCg Scaled.
- Added DDS magic signs support. A sign is two DWORDS forming 8-char comment
  string to be included in DDS headers. New general options: sign, signword.
  New keys: -nosign (disables sign, generate pure DDS), -gimpsign
  (makes GIMP-DDS sign which makes generated YCG1/YCG2 DDS to be
  readable in GIMP)
- Added 4x scaling. This is done by double applying 2x scale filter.
  Each pass can have its' own scale filter (by default second scaler =
  first scaler). This feature adds new commandline parms: -4x and -scaler
  and also new exe-name switches: -4x -2bilinear, -2bicubic, -2bspline,
  -2catmullrom, -2lanczos, -2scale2x, -2super2x. Not that first scaler
  always set the second scaler. So second scaler keys must be after first
  one (-2x-nearest-2bilinear is correct). New option groups: scaler2 and
  scale_4x. For 4x scale, the power-of-two rescale is applied on second pass.

  
1.2
------ 
- Added support for Doom 3 RXGB compression (DXT5 with Red channel being stored
  in Alpha). New general options: normalmapcompression (default, rxgb),
  force_rxgb (file masks). New commandline parm: -rxgb.
- Added channel weights for normalmap compression with ATI Compress.
- Fixed color swapping bugs
- "-scaler" commandline parm implemented
- "-nm" commandline parm implemented - forces texture to be compressed as normalmap


1.1
------
- Can generate ZIP files in memory (speeds up compression with thousands of
  input files, threading likes it too)
- Fixed bug with JPG files conversion


1.0
------
Initial release



