////////////////////////////////////////////////////////////////
//
// RWGTEX - BGRA DDS compressor support
//
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//
// See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
////////////////////////////////

#include "main.h"
#include "dds.h"

/*
==========================================================================================

  DDS compression - BGRA compressor

==========================================================================================
*/

bool BGRA(byte *stream, FS_File *file, LoadedImage *image, DWORD formatCC)
{
	// BGRA compressor
	if (formatCC == FORMAT_BGRA)
	{
		byte *in = Image_GetData(image);
		byte *end = in + image->width * image->height * image->bpp;
		byte *out = stream;

		if (image->hasAlpha)
		{
			while(in < end)
			{
				out[0] = in[0];
				out[1] = in[1];
				out[2] = in[2];
				out[3] = in[3];
				out += 4;		
				in  += 4;
			}
		}
		else
		{
			while(in < end)
			{
				out[0] = in[0];
				out[1] = in[1];
				out[2] = in[2];
				out[3] = 255;
				out += 4;
				in  += 3;
			}
		}
		return true;
	}

	Warning("BGRA : %s%s.dds - unsupported compression %s", file->path.c_str(), file->name.c_str(), getCompressionFormatString(formatCC));
	return false;
}