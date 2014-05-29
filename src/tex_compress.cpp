////////////////////////////////////////////////////////////////
//
// RwgTex / texture compression engine
// (c) Pavel [VorteX] Timofeyev
// See LICENSE text file for a license agreement
//
////////////////////////////////

#include "main.h"
#include "freeimage.h"

/*
==========================================================================================

  Compress

==========================================================================================
*/

void Compress(TexEncodeTask *task)
{
	// force tool
	if (task->codec->forceTool)
		task->tool = task->codec->forceTool;
	else
	{
		for (vector<TexTool*>::iterator i = task->codec->tools.begin(); i < task->codec->tools.end(); i++)
		{
			if (FS_FileMatchList(task->file, task->image, (*i)->forceFileList))
			{
				task->tool = *i;
				break;
			}
		}
	}

	// force format
	if (task->codec->forceFormat)
		task->format = task->codec->forceFormat;
	else
	{
		for (vector<TexFormat*>::iterator i = task->codec->formats.begin(); i < task->codec->formats.end(); i++)
		{
			if (FS_FileMatchList(task->file, task->image, (*i)->forceFileList))
			{
				task->format = *i;
				break;
			}
		}
	}

	// run codec
	if (task->codec->fEncode)
		task->codec->fEncode(task);
	else
		Error("%s: no support for encoding", task->codec->name);
	if (!task->format)
		Error("%s: uninitialized texture format for image '%s'", task->codec->name, task->file->fullpath.c_str());
	if (!task->tool)
		Error("%s: uninitialized texture tool for image '%s", task->codec->name, task->file->fullpath.c_str());


	// postprocess
	bool srgb = (tex_allowSRGB && (task->format->features & FF_SRGB)) && (task->image->sRGB  || tex_forceSRGB || FS_FileMatchList(task->file, task->image, tex_sRGBcolorspace));
	if (tex_mode == TEXMODE_DROP_FILE)
		Print("Compressing %s as %s:%s (%s/%s)\n", task->file->fullpath.c_str(), task->tool->name, task->format->name, task->format->block->name, OptionEnumName(tex_profile, tex_profiles));
	else
		Verbose("Compressing %s as %s:%s (%s/%s)\n", task->file->fullpath.c_str(), task->tool->name, task->format->name, task->format->block->name, OptionEnumName(tex_profile, tex_profiles));
	Image_ConvertSRGB(task->image, srgb);
	Image_SwapColors(task->image, (task->tool->inputflags & (TEXINPUT_BGR|TEXINPUT_BGRA)) ? true : false);
	Image_CalcAverageColor(task->image);
	if (!(task->tool->inputflags & (TEXINPUT_BGR|TEXINPUT_RGB)))
		Image_ConvertBPP(task->image, 4);
	Image_Swizzle(task->image, task->format->colorSwizzle, false);
	if (FS_FileMatchList(task->file, task->image, tex_scale4xFiles) || tex_forceScale4x)
	{
		Image_Scale2x(task->image, tex_firstScaler, true);
		Image_Scale2x(task->image, tex_secondScaler, (tex_allowNPOT && !(task->format->features & FF_POT)) ? false : true);
	}
	else if (FS_FileMatchList(task->file, task->image, tex_scale2xFiles) || tex_forceScale2x)
		Image_Scale2x(task->image, tex_firstScaler, (tex_allowNPOT && !(task->format->features & FF_POT)) ? false : true);
	if (!tex_allowNPOT)
		Image_MakeDimensions(task->image, true, (task->format->features & FF_SQUARE) ? true : false);
	if (task->format->features & FF_BINARYALPHA)
		Image_MakeAlphaBinary(task->image, tex_binaryAlphaCenter);
	Image_ConvertSRGB(task->image, srgb);
	Image_GenerateMaps(task->image, false, (!tex_noMipmaps && !FS_FileMatchList(task->file, task->image, tex_noMipFiles) && !(task->format->features & FF_NOMIP)) ? true : false, (task->format->features & FF_BINARYALPHA) ? true : false);

	
	// allocate memory for destination file
	size_t headersize;
	byte  *header = task->container->fCreateHeader(task->image, task->format, &headersize);
	task->streamLen = headersize + compressedTextureSize(task->image, task->format, task->container, true, true);
	byte *stream = (byte *)mem_alloc(task->streamLen);
	memcpy(stream, header, headersize);
	task->stream = stream + headersize;
	mem_free(header);

	// compress
	task->tool->fCompress(task);
	task->stream = stream;
}

void TexCompress_WorkerThread(ThreadData *thread)
{
	LoadedImage *image, *frame;
	TexCompressData *SharedData;
	TexWriteData *WriteData;
	TexEncodeTask task = { 0 };
	TexCodec *codec;
	char *ext;
	int work;

	SharedData = (TexCompressData *)thread->data;

	image = Image_Create();
	while(1)
	{
		work = GetWorkForThread(thread);
		if (work == -1)
			break; 

		memset(&task, 0, sizeof(task));
		task.file = &textures[work];
		task.container = tex_container;
		task.image = image;
		if (!task.container)
			Error("TexCompress_WorkerThread: no container specified\n");
		
		// cycle all active codecs
		for (codec = tex_active_codecs; codec; codec = codec->nextActive)
		{
			// load image
			if (!image->bitmap)
				Image_Load(task.file, image);
			if (!image->bitmap)
				continue;

			// check if codec accepts task
			// discarded files get fallback codec
			task.codec = codec;
			if (task.codec->disabled)
				continue;
			if (!task.codec->fAccept(&task) || FS_FileMatchList(task.file, task.image, task.codec->discardList))
			{
				task.codec = task.codec->fallback;
				if (!task.codec)
					continue;
				if (task.codec->disabled)
					continue;
			}

			// global stats
			if (task.codec == tex_active_codecs)
				SharedData->size_original_files += image->width*image->height*image->bpp / 1048576.0f;

			// detect special texture types
			image->datatype = IMAGE_COLOR;
			if (FS_FileMatchList(task.file, image, tex_normalMapFiles) || tex_forceBestPSNR)
				image->datatype = IMAGE_NORMALMAP;
			else if (FS_FileMatchList(task.file, image, tex_grayScaleFiles))
				image->datatype = IMAGE_GRAYSCALE;

			// postprocess and export all frames for all encoders
			size_t numexported = 0;
			int framenum = 0;
			for (frame = image; frame != NULL; frame = frame->next, framenum++)
			{
				//Print("Processing %s frame %i %ix%i %i bpp for codec %s\n", task.file->name.c_str(), framenum, frame->width, frame->height, frame->bpp, codec->name);
				// input stats
				task.codec->stat_inputDiskMB += (frame->width*frame->height*frame->bpp) / 1048576.0f;
				if (tex_noMipmaps || FS_FileMatchList(task.file, frame, tex_noMipFiles))
				{
					task.codec->stat_inputRamMB += (frame->width*frame->height*frame->bpp) / 1048576.0f;
					task.codec->stat_inputPOTRamMB += (NextPowerOfTwo(frame->width)*NextPowerOfTwo(frame->height)*frame->bpp)/1048576.0f;
				}
				else
				{
					int w = frame->width;
					int h = frame->height;
					while (w > 1 && h > 1) { task.codec->stat_inputRamMB += (w*h*frame->bpp) / 1048576.0f; w /= 2; h /= 2; }
					w = NextPowerOfTwo(frame->width);
					h = NextPowerOfTwo(frame->height);
					while (w > 1 && h > 1) { task.codec->stat_inputPOTRamMB += (w*h*frame->bpp) / 1048576.0f; w /= 2; h /= 2; }
				}

				// compress
				task.image = frame;
				task.stream = NULL;
				task.streamLen = 0;
				task.tool = NULL;
				task.format = NULL;
				Compress(&task);

				// output stats
				task.codec->stat_outputDiskMB += (float)task.streamLen/1048576.0f;
				task.codec->stat_outputRamMB += (float)(task.streamLen - task.container->headerSize)/1048576.0f;
				task.codec->stat_numTextures++;
				task.codec->stat_numImages++;
				for (ImageMap *map = frame->maps; map; map = map->next)
					codec->stat_numImages++;

				// save for saving thread
				WriteData = (TexWriteData *)mem_alloc(sizeof(TexWriteData));
				memset(WriteData, 0, sizeof(TexWriteData));
				ext = task.container->extensionName;
				sprintf(WriteData->outfile, "%s%s%s%s%s", tex_generateArchive ? "" : tex_destPath, 
					                                    tex_destPathUseCodecDir ? codec->destDir : "", 
														tex_addPath.c_str(),
														task.file->path.c_str(), 
														frame->useTexname ? frame->texname : task.file->name.c_str());
				if (tex_useSuffix & TEXSUFF_FORMAT)
				{
					strcat(WriteData->outfile, task.format->suffix);
					if (task.image->sRGB)
						strcat(WriteData->outfile, "_sRGB");
				}
				if (tex_useSuffix & TEXSUFF_TOOL)
					strcat(WriteData->outfile, task.tool->suffix);
				if (tex_useSuffix & TEXSUFF_PROFILE)
				{
					strcat(WriteData->outfile, "-");
					strcat(WriteData->outfile, OptionEnumName(tex_profile, tex_profiles));
				}
				if (tex_testCompresion)
				{
					ext = "tga";
					byte *oldstream = task.stream;
					task.stream = TexDecompress(WriteData->outfile, &task, &task.streamLen);
					mem_free(oldstream);
				}
				strcat(WriteData->outfile, ".");
				strcat(WriteData->outfile, ext);
				WriteData->data = task.stream;
				WriteData->datasize = task.streamLen;
				WriteData->next = SharedData->writeData;
				SharedData->writeData = WriteData;

				// output stats
				numexported++;
			}
			task.image = image;

			// delete image if it was changed or if it is not needed anymore
			if (Image_Changed(image) || !codec->nextActive)
				Image_Unload(image);

			// global stats
			if (codec == tex_active_codecs)
			{
				SharedData->num_original_files++;
				SharedData->num_exported_files += numexported;
			}
		}
	}
	Image_Delete(image);
}

void TexCompress_MainThread(ThreadData *thread)
{
	HZIP outzip = NULL;
	TexCompressData *SharedData;
	TexWriteData *WriteData, *PrevData;
	void *zipdata;
	unsigned long zipdatalen;

	SharedData = (TexCompressData *)thread->data;

	// check if dest path is an archive
	if (!FS_FileMatchList(tex_destPath, tex_archiveFiles))
	{
		tex_generateArchive = false;
		Print("Generating to \"%s\"\n", tex_destPath);
		AddSlash(tex_destPath);
	}
	else
	{
		tex_generateArchive = true;
		if (tex_zipInMemory <= 0)
			outzip = CreateZip(tex_destPath, "");
		else
		{
			SharedData->zip_maxlen = 1048576 * tex_zipInMemory;
			SharedData->zip_data = mem_alloc(SharedData->zip_maxlen);
			outzip = CreateZip(SharedData->zip_data, SharedData->zip_maxlen, "");

		}
		if (!outzip)
		{
			thread->pool->stop = true;
			Print("Failed to create output archive file %s\n", tex_destPath);
			return;
		}
		Print("Generating to \"%s\" (ZIP archive, compression %i)\n", tex_destPath, tex_zipCompression);
		tex_destPathUseCodecDir = true;
		if (tex_zipInMemory > 0)
			Print("Keeping ZIP in memory (max size %i MBytes)\n", tex_zipInMemory);
	}
	if (tex_addPath.c_str()[0])
		Print("Additional path \"%s\"\n", tex_addPath.c_str());
	thread->pool->started = true;

	// write files
	while(1)
	{
		// print pacifier
		if (!noprint)
			Pacifier(" file %i of %i", SharedData->num_original_files, thread->pool->work_num);
		else
		{
			int p = (int)(((float)(SharedData->num_original_files) / (float)thread->pool->work_num)*100);
			PercentPacifier("%i", p);
		}

		// check if files to write, thats because they havent been added it, or we are finished
		if (!SharedData->writeData)
		{
			if (thread->pool->finished == true)
				break;
			// todo: do some help?
			Sleep(1);
		}
		else
		{
			// get a last file from chain
			PrevData = NULL;
			for (WriteData = SharedData->writeData; WriteData->next; WriteData = WriteData->next)
				PrevData = WriteData;
			if (!PrevData)
			{
				WriteData = SharedData->writeData;
				SharedData->writeData = NULL;
			}
			else
			{
				WriteData = PrevData->next;
				PrevData->next = NULL;
			}

			// write
			if (outzip)
			{
				SharedData->zip_len = ZipGetMemoryWritten(outzip);
				if (tex_zipInMemory)
					if ((SharedData->zip_len + WriteData->datasize) >= SharedData->zip_maxlen)
						Error("TexCompress: Out of free ZIP memory (reached %i MB), consider increasing -zipmem!\n", (float)SharedData->zip_maxlen / 1048576.0f);
				ZRESULT zr = ZipAdd(outzip, WriteData->outfile, WriteData->data, WriteData->datasize, tex_zipCompression);
				if (zr != ZR_OK)
				{
					if (zr == ZR_MEMSIZE)
						Error("TexCompress(%s): failed to pack file into ZIP - out of memory (consider increasing -zipmem). Process stopped. ", WriteData->outfile);
					else if (zr == ZR_FAILED)
						Error("TexCompress(%s): failed to pack file into ZIP - previous file failed. Process stopped. ", WriteData->outfile);
					else
						Warning("TexCompress(%s): failed to pack file into ZIP - error code 0x%08X", WriteData->outfile, zr);
				}
			}
			else
			{
				// write file
				CreatePath(WriteData->outfile);
				FILE *f = fopen(WriteData->outfile, "wb");
				if (!f)
					Warning("TexCompress(%s): cannot open file (%s) for writing", WriteData->outfile, strerror(errno));
				else
				{
					if (!fwrite(WriteData->data, WriteData->datasize, 1, f))
						Warning("TexCompress(%s): cannot write file (%s)", WriteData->outfile, strerror(errno));
					fclose(f);
				}
			}
			mem_free(WriteData->data);
			mem_free(WriteData);
		}
	}

	// close zip
	if (outzip)
	{
		if (SharedData->zip_data)
		{
			ZRESULT zr = ZipGetMemory(outzip, &zipdata, &zipdatalen);
			if (zr != ZR_OK)
				Warning("TexCompress(%s): ZipGetMemory failed - error code 0x%08X", tex_destPath, zr);
			else
			{
				FILE *f = fopen(tex_destPath, "wb");
				if (!f)
					Warning("TexCompress(%s): cannot open ZIP file (%s) for writing", tex_destPath, strerror(errno));
				else
				{
					if (!fwrite(zipdata, zipdatalen, 1, f))
						Warning("TexCompress(%s): cannot write ZIP file (%s)", tex_destPath, strerror(errno));
					fclose(f);
				}
				mem_free(SharedData->zip_data);
			}
		}
		SharedData->zip_len = ZipGetMemoryWritten(outzip);
		CloseZip(outzip);
	}
}

/*
==========================================================================================

  Init

==========================================================================================
*/

// TEXCOMPRESS section options
void TexCompress_Option(const char *section, const char *group, const char *key, const char *val, const char *filename, int linenum)
{
	if (!stricmp(group, "options"))
	{
		if (!stricmp(key, "basepath"))
			tex_gameDir = val;
		else if (!stricmp(key, "binaryalpha"))
			tex_detectBinaryAlpha = OptionBoolean(val);
		else if (!stricmp(key, "addpath"))
			tex_addPath = val;
		else if (!stricmp(key, "nonpoweroftwotextures"))
			tex_allowNPOT = OptionBoolean(val);
		else if (!stricmp(key, "srgbtextures"))
			tex_allowSRGB = OptionBoolean(val);
		else if (!stricmp(key, "generatemipmaps"))
			tex_noMipmaps = OptionBoolean(val) ? false : true;
		else if (!stricmp(key, "averagecolorinfo"))
			tex_noAvgColor = OptionBoolean(val) ? false : true;
		else if (!stricmp(key, "binaryalpha_0"))
			tex_binaryAlphaMin = (byte)(min(max(0, atoi(val)), 255));
		else if (!stricmp(key, "binaryalpha_1"))
			tex_binaryAlphaMax = (byte)(min(max(0, atoi(val)), 255));
		else if (!stricmp(key, "binaryalpha_center"))
			tex_binaryAlphaCenter = (byte)(min(max(0, atoi(val)), 255));
		else if (!stricmp(key, "binaryalpha_threshold"))
			tex_binaryAlphaThreshold = min(max(0.0f, (float)atof(val)), 99.9999f);
		else if (!stricmp(key, "scaler"))
			tex_firstScaler = tex_secondScaler = (ImageScaler)OptionEnum(val, ImageScalers, IMAGE_SCALER_SUPER2X);
		else if (!stricmp(key, "scaler2"))
			tex_secondScaler = (ImageScaler)OptionEnum(val, ImageScalers, IMAGE_SCALER_SUPER2X);
		else if (!stricmp(key, "sign"))
			tex_useSign = OptionBoolean(val);
		else if (!stricmp(key, "signword"))
			strlcpy(tex_sign, val, sizeof(tex_sign));
		else if (!stricmp(key, "signversion"))
			tex_signVersion = FOURCC( strlen(val) < 1 ? 0 : val[0], strlen(val) < 2 ? 0 : val[1], strlen(val) < 3 ? 0 : val[2], strlen(val) < 4 ? 0 : val[3] );
		else
			Warning("%s:%i: unknown key '%s'", filename, linenum, key);
		return;
	}
	if (!stricmp(group, "input")) { OptionFCList(&tex_includeFiles, key, val); return; }
	if (!stricmp(group, "nomip")) { OptionFCList(&tex_noMipFiles, key, val); return; }
	if (!stricmp(group, "is_normalmap")) { OptionFCList(&tex_normalMapFiles, key, val); return; }
	if (!stricmp(group, "is_heightmap")) { OptionFCList(&tex_grayScaleFiles, key, val); return; }
	if (!stricmp(group, "archives")) { OptionFCList(&tex_archiveFiles, key, val); return; }
	if (!stricmp(group, "scale") || !stricmp(group, "scale_2x")) { OptionFCList(&tex_scale2xFiles, key, val); return; }
	if (!stricmp(group, "scale_4x")) { OptionFCList(&tex_scale2xFiles, key, val); return; }
	if (!stricmp(group, "srgb")) { OptionFCList(&tex_sRGBcolorspace, key, val); return; }
	Warning("%s:%i: unknown group '%s'", filename, linenum, group);
}

// CODEC: section options
void TexCompress_CodecOption(TexCodec *codec, const char *group, const char *key, const char *val, const char *filename, int linenum)
{
	if (!stricmp(group, "options"))
	{
		if (!stricmp(key, "disabled"))
			codec->disabled = OptionBoolean(val);
		else if (!stricmp(key, "fallback"))
		{
			if (!stricmp(val, "none"))
				codec->fallback = NULL;
			else
			{
				TexCodec *fallback = findCodec(val, true);
				if (fallback)
					codec->fallback = fallback;
				else
					Warning("%s:%i: cannot set fallback codec '%s' (codec not registered)", val, linenum, key);
			}
		}
		else if (!stricmp(key, "compressor"))
		{
			if (!stricmp(val, "default"))
				codec->forceTool = NULL;
			else
			{
				vector<TexTool*>::iterator tool;
				for (tool = codec->tools.begin(); tool < codec->tools.end(); tool++)
					if (!strcmp((*tool)->parmName, val))
						break;
				if (tool < codec->tools.end())
					codec->forceTool = *tool;
				else
					Warning("%s:%i: unknown compression tool name '%s'", val, linenum, key);
			}
		}
		else if (!stricmp(key, "path"))
		{
			strcpy(codec->destDir, val);
			if (codec->destDir[0])
				AddSlash(codec->destDir);
		}
		else
			codec->fOption(group, key, val, filename, linenum);
		return;
	}

	// discard group
	if (!stricmp(group, "discard"))
	{
		OptionFCList(&codec->discardList, key, val);
		return;
	}

	// tool force_ group
	if (!strnicmp(group, "force_", 6))
	{
		// tool force_ group
		for (vector<TexTool*>::iterator i = codec->tools.begin(); i < codec->tools.end(); i++)
		{
			if (!strcmp((*i)->forceGroup, group))
			{
				OptionFCList(&(*i)->forceFileList, key, val);
				return;
			}
		}
		// format force_ group
		for (vector<TexFormat*>::iterator i = codec->formats.begin(); i < codec->formats.end(); i++)
		{
			if (!strcmp((*i)->forceGroup, group))
			{
				OptionFCList(&(*i)->forceFileList, key, val);
				return;
			}
		}
		Warning("%s:%i: unrecognized force group '%s' (tool or format not found)", val, linenum, group);
		return;
	}

	// run custom codec option
	codec->fOption(group, key, val, filename, linenum);
}

// TOOL: section options
void TexCompress_ToolOption(TexTool *tool, const char *group, const char *key, const char *val, const char *filename, int linenum)
{
	tool->fOption(group, key, val, filename, linenum);
}

// load the tools (after options got parsed)
void TexCompress_Load(void)
{
	// select profile
	tex_profile = PROFILE_REGULAR;
	// COMMANDLINEPARM: -fast: sacrifice quality for fast encoding
	if (CheckParm("-fast"))
		tex_profile = PROFILE_FAST;
	// COMMANDLINEPARM: -regular: high quality and average quality, for regular usage (default)
	if (CheckParm("-regular"))
		tex_profile = PROFILE_REGULAR;
	// COMMANDLINEPARM: -best: most exhaustive methods for best quality (take a lot of time)
	if (CheckParm("-best"))
		tex_profile = PROFILE_BEST;

	// force container
	for (TexContainer *c = tex_containers; c; c = c->next)
		if (CheckParm(c->cmdParm))
			tex_container = c;

	// force tool/format
	for (TexCodec *codec = tex_active_codecs; codec; codec = codec->nextActive)
	{
		// disable codec
		if (CheckParm(codec->cmdParmDisabled))
			codec->disabled = true;
		// force tool
		for (vector<TexTool*>::iterator i = codec->tools.begin(); i < codec->tools.end(); i++)
		{
			if (CheckParm((*i)->cmdParm))
			{
				codec->forceTool = *i;
				break;
			}
		}
		// force format
		for (vector<TexFormat*>::iterator i = codec->formats.begin(); i < codec->formats.end(); i++)
		{
			if (CheckParm((*i)->cmdParm))
			{
				codec->forceFormat = *i;
				break;
			}
		}
	}

	// print active codecs
	TexCodec *active = tex_active_codecs;
	if (!active)
		Error("No codecs selected");
	Print("Active codecs: ");
	while(active)
	{
		Print(active->name);
		if (active->fallback)
			Print("/%s", active->fallback->name);
		if (active->nextActive)
			Print(", ");
		active = active->nextActive;
	}
	Print("\n");
	// print other options
	Print("Target container: %s\n", tex_container->fullName);
	if (tex_forceBestPSNR)
		Print("Using best PSNR compression methods\n");
	if (tex_profile == PROFILE_FAST)
		Print("Using fast compression profile (low quality)\n");
	else if (tex_profile == PROFILE_BEST)
		Print("Using best quality compression profile\n");
	if (tex_forceScale2x || tex_forceScale4x)
	{
		if (tex_forceScale2x)
			Print("Upscale textures to 200%% ");
		else
			Print("Upscale textures to 400%% ");
		Print("First scaler: %s\n", OptionEnumName(tex_firstScaler, ImageScalers, "unknown"));
		if (tex_forceScale4x && (tex_secondScaler != tex_firstScaler))
			Print("Second scaler: %s\n", OptionEnumName(tex_secondScaler, ImageScalers, "unknown"));
	}
	if (tex_allowNPOT)
		Print("Allowed non-power-of-two texture dimensions\n");
	if (tex_noMipmaps)
		Print("Not generating mipmaps\n");
	if (tex_noAvgColor)
		Print("Not generating texture average color info\n");
	if (tex_useSuffix)
		Print("Adding file suffix for tool/format\n");
	if (tex_forceSRGB)
		Print("Forcing sRGB colorspace on all textures");
	if (tex_testCompresionError)
	{
		if (tex_testCompresionAllErrors)
			Print("Showing compression errors for all metrics\n");
		else
			Print("Showing compression errors (metric = %s)\n", OptionEnumName(tex_errorMetric, tex_error_metrics, "auto"));
	}
	else if (tex_testCompresion)
		Print("Generating decompressed test files\n");

	// load codecs
	for (TexCodec *codec = tex_active_codecs; codec; codec = codec->nextActive)
	{
		// load codec
		codec->fLoad();
		if (codec->forceTool)
			Print("%s codec: forcing %s compressor\n", codec->name, codec->forceTool->fullName);
		if (codec->forceFormat)
			Print("%s codec: forcing %s format\n", codec->name, codec->forceFormat->fullName);
		// load tools
		for (vector<TexTool*>::iterator i = codec->tools.begin(); i < codec->tools.end(); i++)
			if ((*i)->fLoad)
				(*i)->fLoad();
	}
}