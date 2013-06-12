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

#include <libgimp/gimp.h>

static inline float saturate(float a)
{
   if(a < 0) a = 0;
   if(a > 1) a = 1;
   return(a);
}

void decode_ycocg_image(gint32 drawableID)
{
   GimpDrawable *drawable;
   GimpPixelRgn srgn, drgn;
   unsigned char *src, *dst;
   int x, y, w, h;
   
   const float offset = 0.5f * 256.0f / 255.0f;
   float Y, Co, Cg, R, G, B;
   
   drawable = gimp_drawable_get(drawableID);
   
   w = drawable->width;
   h = drawable->height;
   
   src = g_malloc(w * 4);
   dst = g_malloc(w * 4);
   
   gimp_pixel_rgn_init(&srgn, drawable, 0, 0, w, h, 0, 0);
   gimp_pixel_rgn_init(&drgn, drawable, 0, 0, w, h, 1, 1);
   
   gimp_progress_init("Decoding YCoCg pixels...");
   
   for(y = 0; y < h; ++y)
   {
      gimp_pixel_rgn_get_row(&srgn, src, 0, y, w);
      
      for(x = 0; x < w; ++x)
      {
         Y  = (float)src[4 * x + 3] / 255.0f;
         Co = (float)src[4 * x + 0] / 255.0f;
         Cg = (float)src[4 * x + 1] / 255.0f;
         
         /* convert YCoCg to RGB */
         Co -= offset;
         Cg -= offset;
         
         R = saturate(Y + Co - Cg);
         G = saturate(Y + Cg);
         B = saturate(Y - Co - Cg);
         
         /* copy new alpha from blue */
         dst[4 * x + 3] = src[4 * x + 2];
         
         dst[4 * x + 0] = (unsigned char)(R * 255.0f);
         dst[4 * x + 1] = (unsigned char)(G * 255.0f);
         dst[4 * x + 2] = (unsigned char)(B * 255.0f);
      }
      
      gimp_pixel_rgn_set_row(&drgn, dst, 0, y, w);

      if((y & 31) == 0)
         gimp_progress_update((gdouble)y / (gdouble)h);
   }
   
   gimp_progress_update(1.0);

   gimp_drawable_flush(drawable);
   gimp_drawable_merge_shadow(drawable->drawable_id, 1);
   gimp_drawable_update(drawable->drawable_id, 0, 0, w, h);
   gimp_drawable_detach(drawable);
   
   g_free(src);
   g_free(dst);
}

void decode_ycocg_scaled_image(gint32 drawableID)
{
   GimpDrawable *drawable;
   GimpPixelRgn srgn, drgn;
   unsigned char *src, *dst;
   int x, y, w, h;

   const float offset = 0.5f * 256.0f / 255.0f;
   float Y, Co, Cg, R, G, B, s;
   
   drawable = gimp_drawable_get(drawableID);
   
   w = drawable->width;
   h = drawable->height;

   src = g_malloc(w * 4);
   dst = g_malloc(w * 4);
   
   gimp_pixel_rgn_init(&srgn, drawable, 0, 0, w, h, 0, 0);
   gimp_pixel_rgn_init(&drgn, drawable, 0, 0, w, h, 1, 1);
   
   gimp_progress_init("Decoding YCoCg (scaled) pixels...");
   
   for(y = 0; y < h; ++y)
   {
      gimp_pixel_rgn_get_row(&srgn, src, 0, y, w);
      
      for(x = 0; x < w; ++x)
      {
         Y  = (float)src[4 * x + 3] / 255.0f;
         Co = (float)src[4 * x + 0] / 255.0f;
         Cg = (float)src[4 * x + 1] / 255.0f;
         s  = (float)src[4 * x + 2] / 255.0f;
         
         /* convert YCoCg to RGB */
         s = 1.0f / ((255.0f / 8.0f) * s + 1.0f);
         
         Co = (Co - offset) * s;
         Cg = (Cg - offset) * s;
         
         R = saturate(Y + Co - Cg);
         G = saturate(Y + Cg);
         B = saturate(Y - Co - Cg);

         dst[4 * x + 0] = (unsigned char)(R * 255.0f);
         dst[4 * x + 1] = (unsigned char)(G * 255.0f);
         dst[4 * x + 2] = (unsigned char)(B * 255.0f);

         /* set alpha to 1 */
         dst[4 * x + 3] = 255;
      }
      
      gimp_pixel_rgn_set_row(&drgn, dst, 0, y, w);
      
      if((y & 31) == 0)
         gimp_progress_update((gdouble)y / (gdouble)h);
   }
   
   gimp_progress_update(1.0);

   gimp_drawable_flush(drawable);
   gimp_drawable_merge_shadow(drawable->drawable_id, 1);
   gimp_drawable_update(drawable->drawable_id, 0, 0, w, h);
   gimp_drawable_detach(drawable);
   
   g_free(src);
   g_free(dst);
}

void decode_alpha_exp_image(gint32 drawableID)
{
   GimpDrawable *drawable;
   GimpPixelRgn srgn, drgn;
   unsigned char *src, *dst;
   int x, y, w, h;
   float R, G, B, A;
   
   drawable = gimp_drawable_get(drawableID);
   
   w = drawable->width;
   h = drawable->height;
   
   src = g_malloc(w * 4);
   dst = g_malloc(w * 4);
   
   gimp_pixel_rgn_init(&srgn, drawable, 0, 0, w, h, 0, 0);
   gimp_pixel_rgn_init(&drgn, drawable, 0, 0, w, h, 1, 1);
   
   gimp_progress_init("Decoding Alpha-exponent pixels...");
   
   for(y = 0; y < h; ++y)
   {
      gimp_pixel_rgn_get_row(&srgn, src, 0, y, w);
      
      for(x = 0; x < w; ++x)
      {
         R = src[4 * x + 0];
         G = src[4 * x + 1];
         B = src[4 * x + 2];
         A = (float)src[4 * x + 3] / 255.0f;
         
         R *= A;
         G *= A;
         B *= A;

         R = MIN(R, 255);
         G = MIN(G, 255);
         B = MIN(B, 255);
         
         dst[4 * x + 0] = (unsigned char)R;
         dst[4 * x + 1] = (unsigned char)G;
         dst[4 * x + 2] = (unsigned char)B;

         /* set alpha to 1 */
         dst[4 * x + 3] = 255;
      }
      
      gimp_pixel_rgn_set_row(&drgn, dst, 0, y, w);
      
      if((y & 31) == 0)
         gimp_progress_update((gdouble)y / (gdouble)h);
   }
   
   gimp_progress_update(1.0);
   
   gimp_drawable_flush(drawable);
   gimp_drawable_merge_shadow(drawable->drawable_id, 1);
   gimp_drawable_update(drawable->drawable_id, 0, 0, w, h);
   gimp_drawable_detach(drawable);
   
   g_free(src);
   g_free(dst);
}
