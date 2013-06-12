// tex_glformats.h
#ifndef H_TEX_GLFORMATS_H
#define H_TEX_GLFORMATS_H

/* glType */
#define GL_BYTE           0x1400
#define GL_UNSIGNED_BYTE  0x1401
#define GL_SHORT          0x1402
#define GL_UNSIGNED_SHORT 0x1403
#define GL_INT            0x1404
#define GL_UNSIGNED_INT   0x1405
#define GL_FLOAT          0x1406
#define GL_DOUBLE         0x140A

/* glFormat, glBaseInternalFormat */
#define GL_RED            0x1903
#define GL_GREEN          0x1904
#define GL_BLUE           0x1905
#define GL_ALPHA          0x1906
#define GL_RGB            0x1907
#define GL_RGBA           0x1908
#define GL_BGR            0x80E0 // GL_EXT_bgr
#define GL_BGRA           0x80E1 // GL_EXT_bgra
#define GL_RG             0x8227 // GL_ARB_texture_rg

/* glInternalFormat */
#define GL_COMPRESSED_RGB_S3TC_DXT1_EXT                  0x83F0 // DXT - GL_EXT_texture_compression_s3tc
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT                 0x83F1 // DXT - GL_EXT_texture_compression_s3tc
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT                 0x83F2 // DXT - GL_EXT_texture_compression_s3tc
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT                 0x83F3 // DXT - GL_EXT_texture_compression_s3tc
#define GL_COMPRESSED_ETC1_RGB8_OES                      0x8D64 // ETC1 - GL_OES_compressed_ETC1_RGB8_texture
#define GL_COMPRESSED_R11_EAC                            0x9270 // ETC2
#define GL_COMPRESSED_SIGNED_R11_EAC                     0x9271 // ETC2
#define GL_COMPRESSED_RG11_EAC                           0x9272 // ETC2
#define GL_COMPRESSED_SIGNED_RG11_EAC                    0x9273 // ETC2
#define GL_COMPRESSED_RGB8_ETC2                          0x9274 // ETC2
#define GL_COMPRESSED_SRGB8_ETC2                         0x9275 // ETC2
#define GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2      0x9276 // ETC2
#define GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2     0x9277 // ETC2
#define GL_COMPRESSED_RGBA8_ETC2_EAC                     0x9278 // ETC2
#define GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC              0x9279 // ETC2
#define COMPRESSED_RGB_PVRTC_4BPPV1_IMG                  0x8C00 // PVRTC - IMG_texture_compression_pvrtc
#define COMPRESSED_RGB_PVRTC_2BPPV1_IMG                  0x8C01 // PVRTC - IMG_texture_compression_pvrtc
#define COMPRESSED_RGBA_PVRTC_4BPPV1_IMG                 0x8C02 // PVRTC - IMG_texture_compression_pvrtc
#define COMPRESSED_RGBA_PVRTC_2BPPV1_IMG                 0x8C03 // PVRTC - IMG_texture_compression_pvrtc

// DXT1: { GL_COMPRESSED_RGB_S3TC_DXT1_EXT, GL_RGB }
// DXT1A: { GL_NONE, GL_NONE, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, GL_RGBA }


#endif