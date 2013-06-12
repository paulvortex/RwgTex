/*
	DDS GIMP plugin

	Copyright (C) 2004 Shawn Kirst <skirst@gmail.com>,
   with parts (C) 2003 Arne Reuter <homepage@arnereuter.de> where specified.

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public
	License as published by the Free Software Foundation; either
	version 2 of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301 USA.
*/

#include <string.h>
#include <math.h>

#include <gtk/gtk.h>

#include "dds.h"
#include "mipmap.h"
#include "imath.h"

typedef void (*mipmapfunc_t)(unsigned char *, int, int, unsigned char *, int, int, int, int, float);
typedef void (*volmipmapfunc_t)(unsigned char *, int, int, int, unsigned char *, int, int, int, int, int, float);

int get_num_mipmaps(int width, int height)
{
   int w = width << 1;
   int h = height << 1;
   int n = 0;

   while(w != 1 || h != 1)
   {
      if(w > 1) w >>= 1;
      if(h > 1) h >>= 1;
      ++n;
   }

   return(n);
}

unsigned int get_mipmapped_size(int width, int height, int bpp,
                                int level, int num, int format)
{
   int w, h, n = 0;
   unsigned int size = 0;

   w = width >> level;
   h = height >> level;
   w = MAX(1, w);
   h = MAX(1, h);
   w <<= 1;
   h <<= 1;

   while(n < num && (w != 1 || h != 1))
   {
      if(w > 1) w >>= 1;
      if(h > 1) h >>= 1;
      if(format == DDS_COMPRESS_NONE)
         size += (w * h);
      else
         size += ((w + 3) >> 2) * ((h + 3) >> 2);
      ++n;
   }

   if(format == DDS_COMPRESS_NONE)
      size *= bpp;
   else
   {
      if(format == DDS_COMPRESS_BC1 || format == DDS_COMPRESS_BC4)
         size *= 8;
      else
         size *= 16;
   }

   return(size);
}

unsigned int get_volume_mipmapped_size(int width, int height,
                                       int depth, int bpp, int level,
                                       int num, int format)
{
   int w, h, d, n = 0;
   unsigned int size = 0;

   w = width >> level;
   h = height >> level;
   d = depth >> level;
   w = MAX(1, w);
   h = MAX(1, h);
   d = MAX(1, d);
   w <<= 1;
   h <<= 1;
   d <<= 1;

   while(n < num && (w != 1 || h != 1))
   {
      if(w > 1) w >>= 1;
      if(h > 1) h >>= 1;
      if(d > 1) d >>= 1;
      if(format == DDS_COMPRESS_NONE)
         size += (w * h * d);
      else
         size += (((w + 3) >> 2) * ((h + 3) >> 2) * d);
      ++n;
   }

   if(format == DDS_COMPRESS_NONE)
      size *= bpp;
   else
   {
      if(format == DDS_COMPRESS_BC1 || format == DDS_COMPRESS_BC4)
         size *= 8;
      else
         size *= 16;
   }

   return(size);
}

int get_next_mipmap_dimensions(int *next_w, int *next_h,
                               int  curr_w, int  curr_h)
{
   if(curr_w == 1 || curr_h == 1)
      return(0);

   if(next_w) *next_w = curr_w >> 1;
   if(next_h) *next_h = curr_h >> 1;

   return(1);
}

static void scale_image_nearest(unsigned char *dst, int dw, int dh,
                                unsigned char *src, int sw, int sh,
                                int bpp, int gc, float gamma)
{
   int n, x, y;
   int ix, iy;
   int srowbytes = sw * bpp;
   int drowbytes = dw * bpp;

   for(y = 0; y < dh; ++y)
   {
      iy = (y * sh + sh / 2) / dh;
      for(x = 0; x < dw; ++x)
      {
         ix = (x * sw + sw / 2) / dw;
         for(n = 0; n < bpp; ++n)
         {
            dst[y * drowbytes + (x * bpp) + n] =
               src[iy * srowbytes + (ix * bpp) + n];
         }
      }
   }
}

static inline int gamma_correct(int v, float gamma)
{
   v = (int)(powf((float)v / 255.0f, gamma) * 255);
   if(v > 255) v = 255;
   return(v);
}

static void scale_image_box(unsigned char *dst, int dw, int dh,
                            unsigned char *src, int sw, int sh,
                            int bpp, int gc, float gamma)
{
   int x, y, n, ix, iy, v;
   int dstride = dw * bpp;
   unsigned char *s;
   float invgamma;

   if(gc)
   {
      invgamma = 1.0f / gamma;
      for(y = 0; y < dh; ++y)
      {
         iy = ((y * sh + sh / 2) / dh) - 1;
         if(iy < 0) iy = 0;

         for(x = 0; x < dw; ++x)
         {
            ix = ((x * sw + sw / 2) / dw) - 1;
            if(ix < 0) ix = 0;

            s = src + (iy * sw + ix) * bpp;

            for(n = 0; n < bpp; ++n)
            {
               v = (gamma_correct(s[0], gamma) +
                    gamma_correct(s[bpp], gamma) +
                    gamma_correct(s[sw * bpp], gamma) +
                    gamma_correct(s[(sw + 1) * bpp], gamma)) >> 2;
               dst[(y * dstride) + (x * bpp) + n] = gamma_correct(v, invgamma);
               ++s;
            }
         }
      }
   }
   else
   {
      for(y = 0; y < dh; ++y)
      {
         iy = ((y * sh + sh / 2) / dh) - 1;
         if(iy < 0) iy = 0;

         for(x = 0; x < dw; ++x)
         {
            ix = ((x * sw + sw / 2) / dw) - 1;
            if(ix < 0) ix = 0;

            s = src + (iy * sw + ix) * bpp;

            for(n = 0; n < bpp; ++n)
            {
               v = (s[0] + s[bpp] + s[sw * bpp] + s[(sw + 1) * bpp]) >> 2;
               dst[(y * dstride) + (x * bpp) + n] = v;
               ++s;
            }
         }
      }
   }
}

static void scale_image_bilinear(unsigned char *dst, int dw, int dh,
                                 unsigned char *src, int sw, int sh,
                                 int bpp, int gc, float gamma)
{
   int x, y, n, ix, iy, wx, wy, v, v0, v1;
   int dstride = dw * bpp;
   unsigned char *s;
   float invgamma;

   if(gc)
   {
      invgamma = 1.0 / gamma;

      for(y = 0; y < dh; ++y)
      {
         if(dh > 1)
         {
            iy = (((sh - 1) * y) << 8) / (dh - 1);
            if(y == dh - 1) --iy;
            wy = iy & 0xff;
            iy >>= 8;
         }
         else
            iy = wy = 0;

         for(x = 0; x < dw; ++x)
         {
            if(dw > 1)
            {
               ix = (((sw - 1) * x) << 8) / (dw - 1);
               if(x == dw - 1) --ix;
               wx = ix & 0xff;
               ix >>= 8;
            }
            else
               ix = wx = 0;

            s = src + (iy * sw + ix) * bpp;

            for(n = 0; n < bpp; ++n)
            {
               v0 = blerp(gamma_correct(s[0], gamma),
                          gamma_correct(s[bpp], gamma), wx);
               v1 = blerp(gamma_correct(s[sw * bpp], gamma),
                          gamma_correct(s[(sw + 1) * bpp], gamma), wx);
               v = blerp(v0, v1, wy);
               dst[(y * dstride) + (x * bpp) + n] = gamma_correct(v, invgamma);
               ++s;
            }
         }
      }
   }
   else
   {
      for(y = 0; y < dh; ++y)
      {
         if(dh > 1)
         {
            iy = (((sh - 1) * y) << 8) / (dh - 1);
            if(y == dh - 1) --iy;
            wy = iy & 0xff;
            iy >>= 8;
         }
         else
            iy = wy = 0;

         for(x = 0; x < dw; ++x)
         {
            if(dw > 1)
            {
               ix = (((sw - 1) * x) << 8) / (dw - 1);
               if(x == dw - 1) --ix;
               wx = ix & 0xff;
               ix >>= 8;
            }
            else
               ix = wx = 0;

            s = src + (iy * sw + ix) * bpp;

            for(n = 0; n < bpp; ++n)
            {
               v0 = blerp(s[0], s[bpp], wx);
               v1 = blerp(s[sw * bpp], s[(sw + 1) * bpp], wx);
               v = blerp(v0, v1, wy);
               if(v < 0) v = 0;
               if(v > 255) v = 255;
               dst[(y * dstride) + (x * bpp) + n] = v;
               ++s;
            }
         }
      }
   }
}

static void scale_image_bicubic(unsigned char *dst, int dw, int dh,
                                unsigned char *src, int sw, int sh,
                                int bpp, int gc, float gamma)
{
   int x, y, n, ix, iy, wx, wy, v;
   int a, b, c, d;
   int dstride = dw * bpp;
   unsigned char *s;
   float invgamma;

   if(gc)
   {
      invgamma = 1.0 / gamma;

      for(y = 0; y < dh; ++y)
      {
         if(dh > 1)
         {
            iy = (((sh - 1) * y) << 7) / (dh - 1);
            if(y == dh - 1) --iy;
            wy = iy & 0x7f;
            iy >>= 7;
         }
         else
            iy = wy = 0;

         for(x = 0; x < dw; ++x)
         {
            if(dw > 1)
            {
               ix = (((sw - 1) * x) << 7) / (dw - 1);
               if(x == dw - 1) --ix;
               wx = ix & 0x7f;
               ix >>= 7;
            }
            else
               ix = wx = 0;

            s = src + ((iy - 1) * sw + (ix - 1)) * bpp;

            for(n = 0; n < bpp; ++n)
            {
               b = icerp(gamma_correct(s[(sw + 0) * bpp], gamma),
                         gamma_correct(s[(sw + 1) * bpp], gamma),
                         gamma_correct(s[(sw + 2) * bpp], gamma),
                         gamma_correct(s[(sw + 3) * bpp], gamma), wx);
               if(iy > 0)
               {
                  a = icerp(gamma_correct(s[      0], gamma),
                            gamma_correct(s[    bpp], gamma),
                            gamma_correct(s[2 * bpp], gamma),
                            gamma_correct(s[3 * bpp], gamma), wx);
               }
               else
                  a = b;

               c = icerp(gamma_correct(s[(2 * sw + 0) * bpp], gamma),
                         gamma_correct(s[(2 * sw + 1) * bpp], gamma),
                         gamma_correct(s[(2 * sw + 2) * bpp], gamma),
                         gamma_correct(s[(2 * sw + 3) * bpp], gamma), wx);
               if(iy < dh - 1)
               {
                  d = icerp(gamma_correct(s[(3 * sw + 0) * bpp], gamma),
                            gamma_correct(s[(3 * sw + 1) * bpp], gamma),
                            gamma_correct(s[(3 * sw + 2) * bpp], gamma),
                            gamma_correct(s[(3 * sw + 3) * bpp], gamma), wx);
               }
               else
                  d = c;

               v = icerp(a, b, c, d, wy);
               if(v < 0) v = 0;
               if(v > 255) v = 255;
               dst[(y * dstride) + (x * bpp) + n] = gamma_correct(v, invgamma);
               ++s;
            }
         }
      }
   }
   else
   {
      for(y = 0; y < dh; ++y)
      {
         if(dh > 1)
         {
            iy = (((sh - 1) * y) << 7) / (dh - 1);
            if(y == dh - 1) --iy;
            wy = iy & 0x7f;
            iy >>= 7;
         }
         else
            iy = wy = 0;

         for(x = 0; x < dw; ++x)
         {
            if(dw > 1)
            {
               ix = (((sw - 1) * x) << 7) / (dw - 1);
               if(x == dw - 1) --ix;
               wx = ix & 0x7f;
               ix >>= 7;
            }
            else
               ix = wx = 0;

            s = src + ((iy - 1) * sw + (ix - 1)) * bpp;

            for(n = 0; n < bpp; ++n)
            {
               b = icerp(s[(sw + 0) * bpp],
                         s[(sw + 1) * bpp],
                         s[(sw + 2) * bpp],
                         s[(sw + 3) * bpp], wx);
               if(iy > 0)
               {
                  a = icerp(s[      0],
                            s[    bpp],
                            s[2 * bpp],
                            s[3 * bpp], wx);
               }
               else
                  a = b;

               c = icerp(s[(2 * sw + 0) * bpp],
                         s[(2 * sw + 1) * bpp],
                         s[(2 * sw + 2) * bpp],
                         s[(2 * sw + 3) * bpp], wx);
               if(iy < dh - 1)
               {
                  d = icerp(s[(3 * sw + 0) * bpp],
                            s[(3 * sw + 1) * bpp],
                            s[(3 * sw + 2) * bpp],
                            s[(3 * sw + 3) * bpp], wx);
               }
               else
                  d = c;

               v = icerp(a, b, c, d, wy);
               if(v < 0) v = 0;
               if(v > 255) v = 255;
               dst[(y * dstride) + (x * bpp) + n] = v;
               ++s;
            }
         }
      }
   }
}

static const float FILTER_RADIUS = 3.0f;

static float lanczos(float r, float x)
{
   float t;
   if(x == 0.0f) return(1.0f);
   if(x <= -r || x >= r) return(0.0f);
   t = x * M_PI;
   return(r * sinf(t) * sinf(t / r) / (t * t));
}

static void scale_image_lanczos(unsigned char *dst, int dw, int dh,
                                unsigned char *src, int sw, int sh,
                                int bpp, int gc, float gamma)
{
   const float blur = 1.0f;
   const float xfactor = (float)dw / (float)sw;
   const float yfactor = (float)dh / (float)sh;

   int x, y, start, stop, nmax, n, i;
   int sstride = sw * bpp;
   float center, contrib, density, s, r;

   unsigned char *d, *row, *col;

   float xscale = MIN(xfactor, 1.0f) / blur;
   float yscale = MIN(yfactor, 1.0f) / blur;
   float xsupport = FILTER_RADIUS / xscale;
   float ysupport = FILTER_RADIUS / yscale;

   float invgamma;

   if(xsupport <= 0.5f)
   {
      xsupport = 0.5f + 1e-12f;
      xscale = 1.0f;
   }
   if(ysupport <= 0.5f)
   {
      ysupport = 0.5f + 1e-12f;
      yscale = 1.0f;
   }

   /* resample in Y direction first to temporary buffer */
   unsigned char *tmp;

   tmp = g_malloc(sw * dh * bpp);
   d = tmp;

   if(gc)
   {
      invgamma = 1.0 / gamma;

      for(y = 0; y < dh; ++y)
      {
         for(x = 0; x < sw; ++x)
         {
            col = src + (x * bpp);

            center = ((float)y + 0.5f) / yfactor;
            start = (int)MAX(center - ysupport + 0.5f, 0);
            stop = (int)MIN(center + ysupport + 0.5f, sh);
            nmax = stop - start;
            s = (float)start - center + 0.5f;

            for(i = 0; i < bpp; ++i)
            {
               density = 0.0f;
               r = 0.0f;

               for(n = 0; n < nmax; ++n)
               {
                  contrib = lanczos(FILTER_RADIUS, (s + n) * yscale);
                  density += contrib;
                  r += (float)gamma_correct(col[((start + n) * sstride) + i], gamma) * contrib;
               }

               if(density != 0.0f && density != 1.0f)
                  r /= density;

               if(r < 0) r = 0;
               if(r > 255) r = 255;

               *d++ = (unsigned char)gamma_correct(r, invgamma);
            }
         }
      }

      /* resample temp buffer in X direction */

      d = dst;

      for(y = 0; y < dh; ++y)
      {
         row = tmp + (y * sstride);

         for(x = 0; x < dw; ++x)
         {
            center = ((float)x + 0.5f) / xfactor;
            start = (int)MAX(center - xsupport + 0.5f, 0);
            stop = (int)MIN(center + xsupport + 0.5f, sw);
            nmax = stop - start;
            s = (float)start - center + 0.5f;

            for(i = 0; i < bpp; ++i)
            {
               density = 0.0f;
               r = 0.0f;

               for(n = 0; n < nmax; ++n)
               {
                  contrib = lanczos(FILTER_RADIUS, (s + n) * xscale);
                  density += contrib;
                  r += (float)gamma_correct(row[((start + n) * bpp) + i], gamma) * contrib;
               }

               if(density != 0.0f && density != 1.0f)
                  r /= density;

               if(r < 0) r = 0;
               if(r > 255) r = 255;

               *d++ = (unsigned char)gamma_correct(r, invgamma);
            }
         }
      }
   }
   else
   {
      for(y = 0; y < dh; ++y)
      {
         for(x = 0; x < sw; ++x)
         {
            col = src + (x * bpp);

            center = ((float)y + 0.5f) / yfactor;
            start = (int)MAX(center - ysupport + 0.5f, 0);
            stop = (int)MIN(center + ysupport + 0.5f, sh);
            nmax = stop - start;
            s = (float)start - center + 0.5f;

            for(i = 0; i < bpp; ++i)
            {
               density = 0.0f;
               r = 0.0f;

               for(n = 0; n < nmax; ++n)
               {
                  contrib = lanczos(FILTER_RADIUS, (s + n) * yscale);
                  density += contrib;
                  r += (float)col[((start + n) * sstride) + i] * contrib;
               }

               if(density != 0.0f && density != 1.0f)
                  r /= density;

               if(r < 0) r = 0;
               if(r > 255) r = 255;

               *d++ = (unsigned char)r;
            }
         }
      }

      /* resample temp buffer in X direction */

      d = dst;

      for(y = 0; y < dh; ++y)
      {
         row = tmp + (y * sstride);

         for(x = 0; x < dw; ++x)
         {
            center = ((float)x + 0.5f) / xfactor;
            start = (int)MAX(center - xsupport + 0.5f, 0);
            stop = (int)MIN(center + xsupport + 0.5f, sw);
            nmax = stop - start;
            s = (float)start - center + 0.5f;

            for(i = 0; i < bpp; ++i)
            {
               density = 0.0f;
               r = 0.0f;

               for(n = 0; n < nmax; ++n)
               {
                  contrib = lanczos(FILTER_RADIUS, (s + n) * xscale);
                  density += contrib;
                  r += (float)row[((start + n) * bpp) + i] * contrib;
               }

               if(density != 0.0f && density != 1.0f)
                  r /= density;

               if(r < 0) r = 0;
               if(r > 255) r = 255;

               *d++ = (unsigned char)r;
            }
         }
      }
   }

   g_free(tmp);
}

int generate_mipmaps(unsigned char *dst, unsigned char *src,
                     unsigned int width, unsigned int height, int bpp,
                     int indexed, int mipmaps, int filter,
                     int gc, float gamma)
{
   int i;
   unsigned int sw, sh, dw, dh;
   unsigned char *s, *d;
   mipmapfunc_t mipmap_func = NULL;

   if(indexed)
      mipmap_func = scale_image_nearest;
   else
   {
      switch(filter)
      {
         case DDS_MIPMAP_FILTER_NEAREST:  mipmap_func = scale_image_nearest;  break;
         case DDS_MIPMAP_FILTER_BILINEAR: mipmap_func = scale_image_bilinear; break;
         case DDS_MIPMAP_FILTER_BICUBIC:  mipmap_func = scale_image_bicubic;  break;
         case DDS_MIPMAP_FILTER_LANCZOS:  mipmap_func = scale_image_lanczos;  break;
         case DDS_MIPMAP_FILTER_BOX:
         default:                         mipmap_func = scale_image_box;      break;
      }
   }

   memcpy(dst, src, width * height * bpp);

   s = dst;
   d = dst + (width * height * bpp);

   sw = width;
   sh = height;

   for(i = 1; i < mipmaps; ++i)
   {
      dw = MAX(1, sw >> 1);
      dh = MAX(1, sh >> 1);

      mipmap_func(d, dw, dh, s, sw, sh, bpp, gc, gamma);

      s = d;
      sw = dw;
      sh = dh;
      d += (dw * dh * bpp);
   }

   return(1);
}

static void scale_volume_image_nearest(unsigned char *dst, int dw, int dh, int dd,
                                       unsigned char *src, int sw, int sh, int sd,
                                       int bpp, int gc, float gamma)
{
   int n, x, y, z;
   int ix, iy, iz;

   for(z = 0; z < dd; ++z)
   {
      iz = (z * sd + sd / 2) / dd;
      for(y = 0; y < dh; ++y)
      {
         iy = (y * sh + sh / 2) / dh;
         for(x = 0; x < dw; ++x)
         {
            ix = (x * sw + sw / 2) / dw;
            for(n = 0; n < bpp; ++n)
            {
               dst[(z * (dw * dh)) + (y * dw) + (x * bpp) + n] =
                  src[(iz * (sw * sh)) + (iy * sw) + (ix * bpp) + n];
            }
         }
      }
   }
}

static void scale_volume_image_box(unsigned char *dst, int dw, int dh, int dd,
                                   unsigned char *src, int sw, int sh, int sd,
                                   int bpp, int gc, float gamma)
{
   int n, x, y, z, v;
   int ix, iy, iz;
   unsigned char *s1, *s2, *d = dst;
   float invgamma;

   if(gc)
   {
      invgamma = 1.0 / gamma;

      for(z = 0; z < dd; ++z)
      {
         iz = ((z * sd + sd / 2) / dd) - 1;
         if(iz < 0) iz = 0;

         for(y = 0; y < dh; ++y)
         {
            iy = ((y * sh + sh / 2) / dh) - 1;
            if(iy < 0) iy = 0;

            for(x = 0; x < dw; ++x)
            {
               ix = ((x * sw + sw / 2) / dw) - 1;
               if(ix < 0) ix = 0;

               s1 = src + ((iz * (sw * sh)) + (iy * sw) + ix) * bpp;
               if(iz < dd - 1)
                  s2 = src + (((iz + 1) * (sw * sh)) + (iy * sw) + ix) * bpp;
               else
                  s2 = src;

               for(n = 0; n < bpp; ++n)
               {
                  v = (gamma_correct(s1[0], gamma) +
                       gamma_correct(s2[0], gamma) +
                       gamma_correct(s1[bpp], gamma) +
                       gamma_correct(s2[bpp], gamma) +
                       gamma_correct(s1[sw * bpp], gamma) +
                       gamma_correct(s2[sw * bpp], gamma) +
                       gamma_correct(s1[(sw + 1) * bpp], gamma) +
                       gamma_correct(s2[(sw + 1) * bpp], gamma)) >> 3;
                  *d++ = gamma_correct(v, invgamma);
                  ++s1;
                  ++s2;
               }
            }
         }
      }
   }
   else
   {
      for(z = 0; z < dd; ++z)
      {
         iz = ((z * sd + sd / 2) / dd) - 1;
         if(iz < 0) iz = 0;

         for(y = 0; y < dh; ++y)
         {
            iy = ((y * sh + sh / 2) / dh) - 1;
            if(iy < 0) iy = 0;

            for(x = 0; x < dw; ++x)
            {
               ix = ((x * sw + sw / 2) / dw) - 1;
               if(ix < 0) ix = 0;

               s1 = src + ((iz * (sw * sh)) + (iy * sw) + ix) * bpp;
               if(iz < dd - 1)
                  s2 = src + (((iz + 1) * (sw * sh)) + (iy * sw) + ix) * bpp;
               else
                  s2 = src;

               for(n = 0; n < bpp; ++n)
               {
                  v =
                     ((s1[0] + s1[bpp] + s1[sw * bpp] + s1[(sw + 1) * bpp]) +
                     (s2[0] + s2[bpp] + s2[sw * bpp] + s2[(sw + 1) * bpp])) >> 3;
                  *d++ = v;
                  ++s1;
                  ++s2;
               }
            }
         }
      }
   }
}

static void scale_volume_image_bilinear(unsigned char *dst, int dw, int dh, int dd,
                                        unsigned char *src, int sw, int sh, int sd,
                                        int bpp, int gc, float gamma)
{
   int x, y, z, n, ix, iy, iz, wx, wy, wz, v, v0, v1, r0, r1;
   unsigned char *s1, *s2, *d = dst;
   float invgamma;

   if(gc)
   {
      invgamma = 1.0 / gamma;

      for(z = 0; z < dd; ++z)
      {
         if(dd > 1)
         {
            iz = (((sd - 1) * z) << 8) / (dd - 1);
            if(z == dd - 1) --iz;
            wz = iz & 0xff;
            iz >>= 8;
         }
         else
            iz = wz = 0;

         for(y = 0; y < dh; ++y)
         {
            if(dh > 1)
            {
               iy = (((sh - 1) * y) << 8) / (dh - 1);
               if(y == dh - 1) --iy;
               wy = iy & 0xff;
               iy >>= 8;
            }
            else
               iy = wy = 0;

            for(x = 0; x < dw; ++x)
            {
               if(dw > 1)
               {
                  ix = (((sw - 1) * x) << 8) / (dw - 1);
                  if(x == dw - 1) --ix;
                  wx = ix & 0xff;
                  ix >>= 8;
               }
               else
                  ix = wx = 0;

               s1 = src + ((iz * (sw * sh)) + (iy * sw) + ix) * bpp;
               s2 = src + (((iz + 1) * (sw * sh)) + (iy * sw) + ix) * bpp;

               for(n = 0; n < bpp; ++n)
               {
                  r0 = blerp(gamma_correct(s1[0], gamma),
                             gamma_correct(s1[bpp], gamma), wx);
                  r1 = blerp(gamma_correct(s1[sw * bpp], gamma),
                             gamma_correct(s1[(sw + 1) * bpp], gamma), wx);
                  v0 = blerp(r0, r1, wy);

                  r0 = blerp(gamma_correct(s2[0], gamma),
                             gamma_correct(s2[bpp], gamma), wx);
                  r1 = blerp(gamma_correct(s2[sw * bpp], gamma),
                             gamma_correct(s2[(sw + 1) * bpp], gamma), wx);
                  v1 = blerp(r0, r1, wy);

                  v = blerp(v0, v1, wz);
                  if(v < 0) v = 0;
                  if(v > 255) v = 255;
                  *d++ = gamma_correct(v, invgamma);
                  ++s1;
                  ++s2;
               }
            }
         }
      }
   }
   else
   {
      for(z = 0; z < dd; ++z)
      {
         if(dd > 1)
         {
            iz = (((sd - 1) * z) << 8) / (dd - 1);
            if(z == dd - 1) --iz;
            wz = iz & 0xff;
            iz >>= 8;
         }
         else
            iz = wz = 0;

         for(y = 0; y < dh; ++y)
         {
            if(dh > 1)
            {
               iy = (((sh - 1) * y) << 8) / (dh - 1);
               if(y == dh - 1) --iy;
               wy = iy & 0xff;
               iy >>= 8;
            }
            else
               iy = wy = 0;

            for(x = 0; x < dw; ++x)
            {
               if(dw > 1)
               {
                  ix = (((sw - 1) * x) << 8) / (dw - 1);
                  if(x == dw - 1) --ix;
                  wx = ix & 0xff;
                  ix >>= 8;
               }
               else
                  ix = wx = 0;

               s1 = src + ((iz * (sw * sh)) + (iy * sw) + ix) * bpp;
               s2 = src + (((iz + 1) * (sw * sh)) + (iy * sw) + ix) * bpp;

               for(n = 0; n < bpp; ++n)
               {
                  r0 = blerp(s1[0], s1[bpp], wx);
                  r1 = blerp(s1[sw * bpp], s1[(sw + 1) * bpp], wx);
                  v0 = blerp(r0, r1, wy);

                  r0 = blerp(s2[0], s2[bpp], wx);
                  r1 = blerp(s2[sw * bpp], s2[(sw + 1) * bpp], wx);
                  v1 = blerp(r0, r1, wy);

                  v = blerp(v0, v1, wz);
                  if(v < 0) v = 0;
                  if(v > 255) v = 255;
                  *d++ = v;
                  ++s1;
                  ++s2;
               }
            }
         }
      }
   }
}

static void scale_volume_image_cubic(unsigned char *dst, int dw, int dh, int dd,
                                     unsigned char *src, int sw, int sh, int sd,
                                     int bpp, int gc, float gamma)
{
   int n, x, y, z;
   int ix, iy, iz;
   int wx, wy, wz;
   int a, b, c, d;
   int val, v0, v1, v2, v3;
   int dstride = dw * bpp;
   int sslice = sw * sh * bpp;
   int dslice = dw * dh * bpp;
   unsigned char *s0, *s1, *s2, *s3;
   float invgamma;

   if(gc)
   {
      invgamma = 1.0 / gamma;

      for(z = 0; z < dd; ++z)
      {
         if(dd > 1)
         {
            iz = (((sd - 1) * z) << 7) / (dd - 1);
            if(z == dd - 1) --iz;
            wz = iz & 0x7f;
            iz >>= 7;
         }
         else
            iz = wz = 0;

         for(y = 0; y < dh; ++y)
         {
            if(dh > 1)
            {
               iy = (((sh - 1) * y) << 7) / (dh - 1);
               if(y == dh - 1) --iy;
               wy = iy & 0x7f;
               iy >>= 7;
            }
            else
               iy = wy = 0;

            for(x = 0; x < dw; ++x)
            {
               if(dw > 1)
               {
                  ix = (((sw - 1) * x) << 7) / (dw - 1);
                  if(x == dw - 1) --ix;
                  wx = ix & 0x7f;
                  ix >>= 7;
               }
               else
                  ix = wx = 0;

               s0 = src + (((iz - 1) * (sw * sh)) + ((iy - 1) * sw) + (ix - 1)) * bpp;
               s1 = s0 + sslice;
               s2 = s1 + sslice;
               s3 = s2 + sslice;

               for(n = 0; n < bpp; ++n)
               {
                  b = icerp(gamma_correct(s1[(sw + 0) * bpp], gamma),
                            gamma_correct(s1[(sw + 1) * bpp], gamma),
                            gamma_correct(s1[(sw + 2) * bpp], gamma),
                            gamma_correct(s1[(sw + 3) * bpp], gamma), wx);
                  if(iy > 0)
                  {
                     a = icerp(gamma_correct(s1[      0], gamma),
                               gamma_correct(s1[    bpp], gamma),
                               gamma_correct(s1[2 * bpp], gamma),
                               gamma_correct(s1[3 * bpp], gamma), wx);
                  }
                  else
                     a = b;

                  c = icerp(gamma_correct(s1[(2 * sw + 0) * bpp], gamma),
                            gamma_correct(s1[(2 * sw + 1) * bpp], gamma),
                            gamma_correct(s1[(2 * sw + 2) * bpp], gamma),
                            gamma_correct(s1[(2 * sw + 3) * bpp], gamma), wx);
                  if(iy < dh - 1)
                  {
                     d = icerp(gamma_correct(s1[(3 * sw + 0) * bpp], gamma),
                               gamma_correct(s1[(3 * sw + 1) * bpp], gamma),
                               gamma_correct(s1[(3 * sw + 2) * bpp], gamma),
                               gamma_correct(s1[(3 * sw + 3) * bpp], gamma), wx);
                  }
                  else
                     d = c;

                  v1 = icerp(a, b, c, d, wy);

                  if(iz > 0)
                  {
                     b = icerp(gamma_correct(s0[(sw + 0) * bpp], gamma),
                               gamma_correct(s0[(sw + 1) * bpp], gamma),
                               gamma_correct(s0[(sw + 2) * bpp], gamma),
                               gamma_correct(s0[(sw + 3) * bpp], gamma), wx);
                     if(iy > 0)
                     {
                        a = icerp(gamma_correct(s0[      0], gamma),
                                  gamma_correct(s0[    bpp], gamma),
                                  gamma_correct(s0[2 * bpp], gamma),
                                  gamma_correct(s0[3 * bpp], gamma), wx);
                     }
                     else
                        a = b;

                     c = icerp(gamma_correct(s0[(2 * sw + 0) * bpp], gamma),
                               gamma_correct(s0[(2 * sw + 1) * bpp], gamma),
                               gamma_correct(s0[(2 * sw + 2) * bpp], gamma),
                               gamma_correct(s0[(2 * sw + 3) * bpp], gamma), wx);
                     if(iy < dh - 1)
                     {
                        d = icerp(gamma_correct(s0[(3 * sw + 0) * bpp], gamma),
                                  gamma_correct(s0[(3 * sw + 1) * bpp], gamma),
                                  gamma_correct(s0[(3 * sw + 2) * bpp], gamma),
                                  gamma_correct(s0[(3 * sw + 3) * bpp], gamma), wx);
                     }
                     else
                        d = c;

                     v0 = icerp(a, b, c, d, wy);
                  }
                  else
                     v0 = v1;

                  b = icerp(gamma_correct(s2[(sw + 0) * bpp], gamma),
                            gamma_correct(s2[(sw + 1) * bpp], gamma),
                            gamma_correct(s2[(sw + 2) * bpp], gamma),
                            gamma_correct(s2[(sw + 3) * bpp], gamma), wx);
                  if(iy > 0)
                  {
                     a = icerp(gamma_correct(s2[      0], gamma),
                               gamma_correct(s2[    bpp], gamma),
                               gamma_correct(s2[2 * bpp], gamma),
                               gamma_correct(s2[3 * bpp], gamma), wx);
                  }
                  else
                     a = b;

                  c = icerp(gamma_correct(s2[(2 * sw + 0) * bpp], gamma),
                            gamma_correct(s2[(2 * sw + 1) * bpp], gamma),
                            gamma_correct(s2[(2 * sw + 2) * bpp], gamma),
                            gamma_correct(s2[(2 * sw + 3) * bpp], gamma), wx);
                  if(iy < dh - 1)
                  {
                     d = icerp(gamma_correct(s2[(3 * sw + 0) * bpp], gamma),
                               gamma_correct(s2[(3 * sw + 1) * bpp], gamma),
                               gamma_correct(s2[(3 * sw + 2) * bpp], gamma),
                               gamma_correct(s2[(3 * sw + 3) * bpp], gamma), wx);
                  }
                  else
                     d = c;

                  v2 = icerp(a, b, c, d, wy);

                  if(iz < dd - 1)
                  {
                     b = icerp(gamma_correct(s3[(sw + 0) * bpp], gamma),
                               gamma_correct(s3[(sw + 1) * bpp], gamma),
                               gamma_correct(s3[(sw + 2) * bpp], gamma),
                               gamma_correct(s3[(sw + 3) * bpp], gamma), wx);
                     if(iy > 0)
                     {
                        a = icerp(gamma_correct(s3[      0], gamma),
                                  gamma_correct(s3[    bpp], gamma),
                                  gamma_correct(s3[2 * bpp], gamma),
                                  gamma_correct(s3[3 * bpp], gamma), wx);
                     }
                     else
                        a = b;

                     c = icerp(gamma_correct(s3[(2 * sw + 0) * bpp], gamma),
                               gamma_correct(s3[(2 * sw + 1) * bpp], gamma),
                               gamma_correct(s3[(2 * sw + 2) * bpp], gamma),
                               gamma_correct(s3[(2 * sw + 3) * bpp], gamma), wx);
                     if(iy < dh - 1)
                     {
                        d = icerp(gamma_correct(s3[(3 * sw + 0) * bpp], gamma),
                                  gamma_correct(s3[(3 * sw + 1) * bpp], gamma),
                                  gamma_correct(s3[(3 * sw + 2) * bpp], gamma),
                                  gamma_correct(s3[(3 * sw + 3) * bpp], gamma), wx);
                     }
                     else
                        d = c;

                     v3 = icerp(a, b, c, d, wy);
                  }
                  else
                     v3 = v2;

                  val = icerp(v0, v1, v2, v3, wz);

                  if(val <   0) val = 0;
                  if(val > 255) val = 255;

                  dst[(z * dslice) + (y * dstride) + (x * bpp) + n] =
                     gamma_correct(val, invgamma);

                  ++s0;
                  ++s1;
                  ++s2;
                  ++s3;
               }
            }
         }
      }
   }
   else
   {
      for(z = 0; z < dd; ++z)
      {
         if(dd > 1)
         {
            iz = (((sd - 1) * z) << 7) / (dd - 1);
            if(z == dd - 1) --iz;
            wz = iz & 0x7f;
            iz >>= 7;
         }
         else
            iz = wz = 0;

         for(y = 0; y < dh; ++y)
         {
            if(dh > 1)
            {
               iy = (((sh - 1) * y) << 7) / (dh - 1);
               if(y == dh - 1) --iy;
               wy = iy & 0x7f;
               iy >>= 7;
            }
            else
               iy = wy = 0;

            for(x = 0; x < dw; ++x)
            {
               if(dw > 1)
               {
                  ix = (((sw - 1) * x) << 7) / (dw - 1);
                  if(x == dw - 1) --ix;
                  wx = ix & 0x7f;
                  ix >>= 7;
               }
               else
                  ix = wx = 0;

               s0 = src + (((iz - 1) * (sw * sh)) + ((iy - 1) * sw) + (ix - 1)) * bpp;
               s1 = s0 + sslice;
               s2 = s1 + sslice;
               s3 = s2 + sslice;

               for(n = 0; n < bpp; ++n)
               {
                  b = icerp(s1[(sw + 0) * bpp],
                           s1[(sw + 1) * bpp],
                           s1[(sw + 2) * bpp],
                           s1[(sw + 3) * bpp], wx);
                  if(iy > 0)
                  {
                     a = icerp(s1[      0],
                              s1[    bpp],
                              s1[2 * bpp],
                              s1[3 * bpp], wx);
                  }
                  else
                     a = b;

                  c = icerp(s1[(2 * sw + 0) * bpp],
                           s1[(2 * sw + 1) * bpp],
                           s1[(2 * sw + 2) * bpp],
                           s1[(2 * sw + 3) * bpp], wx);
                  if(iy < dh - 1)
                  {
                     d = icerp(s1[(3 * sw + 0) * bpp],
                              s1[(3 * sw + 1) * bpp],
                              s1[(3 * sw + 2) * bpp],
                              s1[(3 * sw + 3) * bpp], wx);
                  }
                  else
                     d = c;

                  v1 = icerp(a, b, c, d, wy);

                  if(iz > 0)
                  {
                     b = icerp(s0[(sw + 0) * bpp],
                              s0[(sw + 1) * bpp],
                              s0[(sw + 2) * bpp],
                              s0[(sw + 3) * bpp], wx);
                     if(iy > 0)
                     {
                        a = icerp(s0[      0],
                                 s0[    bpp],
                                 s0[2 * bpp],
                                 s0[3 * bpp], wx);
                     }
                     else
                        a = b;

                     c = icerp(s0[(2 * sw + 0) * bpp],
                              s0[(2 * sw + 1) * bpp],
                              s0[(2 * sw + 2) * bpp],
                              s0[(2 * sw + 3) * bpp], wx);
                     if(iy < dh - 1)
                     {
                        d = icerp(s0[(3 * sw + 0) * bpp],
                                 s0[(3 * sw + 1) * bpp],
                                 s0[(3 * sw + 2) * bpp],
                                 s0[(3 * sw + 3) * bpp], wx);
                     }
                     else
                        d = c;

                     v0 = icerp(a, b, c, d, wy);
                  }
                  else
                     v0 = v1;

                  b = icerp(s2[(sw + 0) * bpp],
                           s2[(sw + 1) * bpp],
                           s2[(sw + 2) * bpp],
                           s2[(sw + 3) * bpp], wx);
                  if(iy > 0)
                  {
                     a = icerp(s2[      0],
                              s2[    bpp],
                              s2[2 * bpp],
                              s2[3 * bpp], wx);
                  }
                  else
                     a = b;

                  c = icerp(s2[(2 * sw + 0) * bpp],
                           s2[(2 * sw + 1) * bpp],
                           s2[(2 * sw + 2) * bpp],
                           s2[(2 * sw + 3) * bpp], wx);
                  if(iy < dh - 1)
                  {
                     d = icerp(s2[(3 * sw + 0) * bpp],
                              s2[(3 * sw + 1) * bpp],
                              s2[(3 * sw + 2) * bpp],
                              s2[(3 * sw + 3) * bpp], wx);
                  }
                  else
                     d = c;

                  v2 = icerp(a, b, c, d, wy);

                  if(iz < dd - 1)
                  {
                     b = icerp(s3[(sw + 0) * bpp],
                              s3[(sw + 1) * bpp],
                              s3[(sw + 2) * bpp],
                              s3[(sw + 3) * bpp], wx);
                     if(iy > 0)
                     {
                        a = icerp(s3[      0],
                                 s3[    bpp],
                                 s3[2 * bpp],
                                 s3[3 * bpp], wx);
                     }
                     else
                        a = b;

                     c = icerp(s3[(2 * sw + 0) * bpp],
                              s3[(2 * sw + 1) * bpp],
                              s3[(2 * sw + 2) * bpp],
                              s3[(2 * sw + 3) * bpp], wx);
                     if(iy < dh - 1)
                     {
                        d = icerp(s3[(3 * sw + 0) * bpp],
                                 s3[(3 * sw + 1) * bpp],
                                 s3[(3 * sw + 2) * bpp],
                                 s3[(3 * sw + 3) * bpp], wx);
                     }
                     else
                        d = c;

                     v3 = icerp(a, b, c, d, wy);
                  }
                  else
                     v3 = v2;

                  val = icerp(v0, v1, v2, v3, wz);

                  if(val <   0) val = 0;
                  if(val > 255) val = 255;

                  dst[(z * dslice) + (y * dstride) + (x * bpp) + n] = val;

                  ++s0;
                  ++s1;
                  ++s2;
                  ++s3;
               }
            }
         }
      }
   }
}

int generate_volume_mipmaps(unsigned char *dst, unsigned char *src,
                            unsigned int width, unsigned int height,
                            unsigned int depth, int bpp, int indexed,
                            int mipmaps, int filter, int gc, float gamma)
{
   int i;
   unsigned int sw, sh, sd;
   unsigned int dw, dh, dd;
   unsigned char *s, *d;
   volmipmapfunc_t mipmap_func = NULL;

   if(indexed)
      mipmap_func = scale_volume_image_nearest;
   else
   {
      switch(filter)
      {
         case DDS_MIPMAP_FILTER_NEAREST:  mipmap_func = scale_volume_image_nearest;  break;
         case DDS_MIPMAP_FILTER_BILINEAR: mipmap_func = scale_volume_image_bilinear; break;
         case DDS_MIPMAP_FILTER_BICUBIC:  mipmap_func = scale_volume_image_cubic;    break;
         case DDS_MIPMAP_FILTER_BOX:
         default:
                                   mipmap_func = scale_volume_image_box;      break;
      }
   }

   memcpy(dst, src, width * height * depth * bpp);

   s = dst;
   d = dst + (width * height * depth * bpp);

   sw = width;
   sh = height;
   sd = depth;

   for(i = 1; i < mipmaps; ++i)
   {
      dw = MAX(1, sw >> 1);
      dh = MAX(1, sh >> 1);
      dd = MAX(1, sd >> 1);

      mipmap_func(d, dw, dh, dd, s, sw, sh, sd, bpp, gc, gamma);

      s = d;
      sw = dw;
      sh = dh;
      sd = dd;
      d += (dw * dh * dd * bpp);
   }

   return(1);
}
