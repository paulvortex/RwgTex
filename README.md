RwgTex is commandline texture conversion tool used to generate compressed textures package for games.

Features
------

- batch-converts textures (the target is to generate whole game textures
  package with single call)
- automatically rescales images to nearest Power-Of-Two
- detect special file types (normalmaps, heightmaps) and use best looking
  compression and compressor for them
- for textures with alpha channels, alpha is scanned to check if it can be
  represented with one bit (DXT1) or requires compression with gradient
  alpha (DXT5)
- multiple threads to compress
- supported input files: TGA, JPG, PNG
- supported specific texture containers: SPR32, Quake 1 .BSP
- textures read/export for .ZIP archives
- wide range of options to get it run with particlular engine/mod
- supported compression formats: DXT1-5, ETC1, ETC2, PVRTC
- supported DXT swizzled formats: Doom 3 RXGB, YCoCg, YCoCg Scaled, YCoCg Gamma 2.0, YCoCg Scaled Gamma 2.0
- support uncompressed BGRA DDS
- saves a cache of files crc32 to check if they were modified (speeds up
  compression when run next time by only compressing files that was changed)

 
Usage
------

1) rwgtex.exe <input_dir> <output_dir>
   Scan <input_dir> for tga, jpg, png, spr32 files and convert them to DDS to
   output dir. Also it will generate a filescrc.txt file holding crc32 sums for
   source files used to make DDS. So next time you will run RwgTex with this
   output folder it will only convert files that was changed.

2) rwgtex.exe <path>
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
-ktx       : enforces KTX (Chronos Texture) file creation and texture compression
-w         : wait for key press once finished
-nw        : don't wait for key press
-f         : suppress any prints
-mem       : show memory usage stats at end
-threads X : manually sets the number of threads
-cd X      : change to this dir once started
-nomip     : do not generate any mip-maps
-opt X     : load custom option file
-ati       : use ATI compressor
-nv        : use Nvidia DXTlib (deprecated to Nvidia Texture Tools) compressor
-nvtt      : use Nvidia Texture Tools compressor
-gimp      : use GIMP DDS plugin compressor
-etcpack   : use EtcPack compressor
-pvrtex    : use PowerVR's PvrTex compressor
-etc2comp  : use Google's Etc2Comp compressor
-npot      : allow non-power-of-two texture dimensions
-nm        : forces texture to be processed as normalmap
-dxt1      : forces DXT1 compression
-dxt2      : forces DXT2 compression
-dxt3      : forces DXT3 compression
-dxt4      : forces DXT4 compression
-dxt5      : forces DXT5 compression
-rxgb      : forces RXGB compression
-ycg1      : forces YCoCg compression
-ycg2      : forces YCoCg Scaled compression
-ycg3      : forces YCoCg Gamma 2.0 compression
-ycg4      : forces YCoCg Scaled Gamma 2.0 compression
-etc1      : forces ETC1 compression
-etc2      : forces ETC2 compression
-etc2rgb   : forces ETC2RGB compression
-etc2rgba  : forces ETC2RGBA compression
-etc2rgba1 : forces ETC2RGBA1 compression
-pvr2      : forces PVRTC 2bpp compression
-pvr4      : forces PVRTC 4bpp compressi

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
Example: rwgtex-ati-w.exe

Compression formats
------
DXT1   : DirectX Block Texture Compression (was S3TC)
         4 bits per pixel, RGB
DXT1A  : 4 bits per pixel, RGBA with punch-through alpha
DXT2   : 8 bits per pixel, RGBA, sharp alpha, color premultiplied
DXT3   : 8 bits per pixel, RGBA, sharp alpha
DXT4   : 8 bits per pixel, RGBA, gradient alpha, color premultiplied
DXT5   : 8 bits per pixel, RGBA, gradient alpha      
YCG1   : Swizzled DXT5 compression that uses YCoCg Colorspace
         better color quality than DXT5 but much worse alpha
YCG2   : Swizzled DXT5 compression that uses YCoCg Colorspace with Scaling
         better color quality than YCG1, no alpha support
YCG3   : YCG1 using Gamma 2.0 RGB colorspace
         better precision for dark colors
YCG4   : YCG2 using Gamma 2.0 RGB colorspace
         better precision for dark colors        
RXGB   : Swizzled DXT5 compression used by Doom 3
         green channel moved to alpha, used for normalmaps
ETC1   : Ericsson Texture Compression, a part of OpenGL ES specification
         4 bits per pixel, RGB
ETC2   : ETC2 compression, a part of OpenGL ES/OpenGL 4.3 specification
         4 bits per pixel, RGB
ETC2A  : 8 bits per pixel, RGBA
ETC2A1 : 4 bits per pixel, RGBA with punch-through alpha
EAC1   : 4 bits per pixel, alpha-only
EAC2   : 8 bits per pixel, two channels (RG)
PVR2   : PowerVR Texture compression, used in Apple devices
       : 2 bits per pixel, RGB
PVR2A  : 2 bits per pixel, RGBA
PVR4   : 4 bits per pixel, RGB
PVR2A  : 4 bits per pixel, RGBA

Compression modes
------
0. Default compressor. Use best speed/quality compressor for the current codec.
   For the DXT it's ATI Compressonator for color, NVidia Texture Tools for normalmaps
   For ETC1/ETC1 it's Google's Etc2Comp
   For PVRTC it is PowerVR TexTool
1. "ATI" - AMD's The Compressonator tool.
   Supported compressions: DXT1-5, RXGB, YCG1-4
2. "NV" - NVidia DXTlib compressor.
   Supported compressions: DXT1-5, RXGB, YCG1-4
   Now deprecated to NVidia Texture Tools (all options is keeped for compatibility reasons).
3. "NVTT" - NVidia Texture Tools.
   Supported compressions: DXT1-5, RXGB, YCG1-4
4. "CrnLib" - Crunch Library.
   Supported compressions: DXT1-5, RXGB, YCG1-4
5. "PvrTex" - PowerVR's PvrTex tool.
   Supported compressions: PVR2, PVR2A, PVR4, PVR4A, DXT1-5, RXGB, YCG1-4, ETC1
6. "RgEtc1" - Fast, high quality ETC1 compression tool
   Supported compressions: ETC1
7. "EtcPack" - Official ETC/ETC2 compression tool
   Supported compressions: ETC1, ETC2, ETC2A, ETC2A1, EAC1, EAC2
8. "Etc2Comp" - Google's ETC/ETC2 compression tool
   Supported compressions: ETC1, ETC2, ETC2A, ETC2A1, EAC1, EAC2
9. "Gimp DDS" - GIMP DDS plugin
   Supported compressions: DXT1-5, YCG1-4
10. "BRGA" - simple tool that write 32-bit BGRA files
   Supported compressions: none

Trick: .ini file have parms to set compression mode on a per-file basis using name masks.

Known issues
------
- Archives larger than 2GB are not supported
- When using archives, file cache are not supported
- DXT2/DXT4 cannot be represented by KTX format without additional key-pairs (they are used to be 'swizzled' format)

--------------------------------------------------------------------------------
 Version History + Changelog
--------------------------------------------------------------------------------

1.7
------
- Project moved to MSVS 2015
- NvDxtlib support removed (conflicts with MSVC 2015 runtimes)
- New tool: NVidia Texture Tools (NvTT), improved version of NvDxtLib
- NvTT now wraps NvDXTlib options and code (for compatibility reasons)
- New superb ETC2 compressor: Google Etc2Comp! Good quality ETC2 with reasonable speed!
- Bump PvrTexTool: version is now 4.13.0 (fixes some bugs and improves PVRTC compression)
- Fixed PVRTC 2bpp compression crash
- Win32 build: now supplied with 2 new dll's: nvtt.dll, PVRTexLib.dll

1.6
------
- Internal bugfixes release

1.5
------
- Internal bugfixes release

1.4
------
- Added support for ETC compression using etcpack.
- New compressor: ETCPack
- New keys: -etcpack, -etc1, -etc2, -etc2a, -etc2a1, -eac1, -eac2
- New options: force_etc1, force_etc2, force_etc2am force_etc2a1, force_eac1, force_eac2

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



