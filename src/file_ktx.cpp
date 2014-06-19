////////////////////////////////////////////////////////////////
//
// RwgTex / KTX file format
// (c) Pavel [VorteX] Timofeyev
// See LICENSE text file for a license agreement
//
////////////////////////////////

#define F_FILE_KTX_C
#include "main.h"
#include "tex.h"

TexContainer CONTAINER_KTX =
{
	"KTX", "Khronos Texture (.KTX)", "ktx", 12,
	&KTX_Scan,
	sizeof(KTX_HEADER), 4, 4,
	&KTX_PrintHeader,
	&KTX_CreateHeader,
	&KTX_WriteMipHeader,
	&KTX_Read,
};

bool KTX_Scan(byte *data)
{
	if (memcmp(data, &KTX_IDENTIFIER, 12))
		return false;
	return true;
}

// extract new key/value pair from KTX metadata, returns allocated value
char *KTX_ReadKeyPair(byte **stream, char **key, uint *valueSize)
{
	uint keyAndValueSize, i;
	size_t valsize = 0;
	char *value, *s;

	keyAndValueSize = *((uint *)*stream);
	*key = (char *)(*stream + 4);
	for (i = 0; i < keyAndValueSize; i++)
	{
		if (!*key[i])
		{
			i++;
			value = *key + i;
			valsize = keyAndValueSize - i;
			break;
		}
	}
	*stream += 4 + keyAndValueSize + ((keyAndValueSize/4)*4 - keyAndValueSize);
	// extract value as string
	s = (char *)mem_alloc(valsize + 1);
	if (valsize)
		memcpy(s, value, valsize);
	s[valsize] = 0;
	if (valueSize)
		*valueSize = valsize;
	return s;
}

// write key/value pair to KTX metadata datastream
void KTX_WriteKeyPair(char *key, byte *data, uint dataSize, byte **keyData, uint *keyDataSize)
{
	uint keyLen, newSize, keyAndValueSize, valuePadding;
	byte *stream;

	// allocate and expand
	keyLen = strlen(key) + 1;
	keyAndValueSize = keyLen + dataSize;
	valuePadding = ((keyAndValueSize/4)*4 - keyAndValueSize);
	newSize = 4 + keyAndValueSize + valuePadding;
	if (!*keyDataSize)
	{
		*keyDataSize = newSize;
		*keyData = (byte *)mem_alloc(newSize);
		stream = *keyData;
	}
	else
	{
		*keyData = (byte *)mem_realloc(*keyData, *keyDataSize + newSize);
		stream = *keyData + *keyDataSize;
		*keyDataSize = *keyDataSize + newSize;
	}
	// write
	memcpy(stream, &keyAndValueSize, 4);
	memcpy(stream + 4, key, keyLen);
	memcpy(stream + 4 + keyLen, data, dataSize);
	memset(stream + 4 + keyLen + dataSize, 0, valuePadding);
}

void KTX_WriteKeyPair(char *key, char *string, byte **keyData, uint *keyDataSize)
{
	KTX_WriteKeyPair(key, (byte *)string, strlen(string), keyData, keyDataSize);
}

void KTX_PrintHeader(byte *data)
{
	byte *kv, *end;
	char *key, *value;
	size_t valueSize;
	KTX_HEADER *header;

	header = (KTX_HEADER *)data;
	Print("KTX header:\n");
	Print("  endianness: 0x%08X\n", header->endianness);
	Print("  glType: 0x%08X\n", header->glType);
	Print("  glTypeSize: 0x%08X\n", header->glTypeSize);
	Print("  glFormat: 0x%08X\n", header->glFormat);
	Print("  glInternalFormat: 0x%08X\n", header->glInternalFormat);
	Print("  glBaseInternalFormat: 0x%08X\n", header->glBaseInternalFormat);
	Print("  pixelWidth: %i\n", header->pixelWidth);
	Print("  pixelHeight: %i\n", header->pixelHeight);
	Print("  pixelDepth: %i\n", header->pixelDepth);
	Print("  numberOfArrayElements: %i\n", header->numberOfArrayElements);
	Print("  numberOfFaces: %i\n", header->numberOfFaces);
	Print("  numberOfMipmapLevels: %i\n", header->numberOfMipmapLevels);
	Print("  bytesOfKeyValueData: %i\n", header->bytesOfKeyValueData);
	if (header->bytesOfKeyValueData)
	{
		Print("KTX metadata:\n");
		kv = data + sizeof(KTX_HEADER);
		end = kv + header->bytesOfKeyValueData;
		while(kv < end)
		{
			value = KTX_ReadKeyPair(&kv, &key, &valueSize);
			Print("  %s: %s\n", key, value);
			mem_free(value);
		}
	}
}

byte *KTX_CreateHeader(LoadedImage *image, TexFormat *format, size_t *outsize)
{
	// generate KTX metadata key/value pairs
	byte *keyData = NULL;
	uint keyDataSize = 0;
	KTX_WriteKeyPair("KTXorientation", "S=r,T=d,R=i", &keyData, &keyDataSize);
	KTX_WriteKeyPair("fourCC", (byte *)format->fourCC, 4, &keyData, &keyDataSize);
	if (tex_useSign)
		KTX_WriteKeyPair("comment", tex_sign, &keyData, &keyDataSize);
	if (image->hasAverageColor)
		KTX_WriteKeyPair("avgColor", image->averagecolor, 3, &keyData, &keyDataSize);
	if (image->maps->sRGB)
		KTX_WriteKeyPair("sRGBcolorspace", 0, 0, &keyData, &keyDataSize);
	if (image->datatype == IMAGE_NORMALMAP)
		KTX_WriteKeyPair("normalmap", 0, 0, &keyData, &keyDataSize);

	// create header
	byte *head = (byte *)mem_alloc(sizeof(KTX_HEADER) + keyDataSize);
	memcpy(head + sizeof(KTX_HEADER), keyData, keyDataSize);
	mem_free(keyData);
	KTX_HEADER *ktx = (KTX_HEADER *)head;
	memcpy(ktx->identifier, KTX_IDENTIFIER, sizeof(KTX_IDENTIFIER));
	ktx->endianness = 0x04030201;
	ktx->glType = format->glType;
	ktx->glTypeSize = 1; // no support for big endian...
	ktx->glFormat = format->glFormat;
	ktx->glInternalFormat = format->glInternalFormat;
	ktx->glBaseInternalFormat = ktx->glFormat;
	ktx->pixelWidth = image->width;
	ktx->pixelHeight = image->height;
	ktx->pixelDepth = 0;
	ktx->numberOfArrayElements = 0;
	ktx->numberOfFaces = 0;
	ktx->numberOfMipmapLevels = 0;
	for (ImageMap *map = image->maps; map; map = map->next) ktx->numberOfMipmapLevels++;
	ktx->bytesOfKeyValueData = keyDataSize;

	return head;
}

size_t KTX_WriteMipHeader(byte *stream, size_t width, size_t height, size_t pixeldatasize)
{
	*(uint *)stream = (uint)pixeldatasize;
	return 4;
}

bool KTX_Read(TexDecodeTask *task)
{
	return false;
}