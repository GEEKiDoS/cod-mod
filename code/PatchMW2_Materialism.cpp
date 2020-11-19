// ==========================================================
// IW4M project
// 
// Component: clientdll
// Sub-component: steam_api
// Purpose: Material/image modification code
//
// Initial author: NTAuthority
// Started: 2011-06-09
// ==========================================================

#include "StdInc.h"
#include <google/dense_hash_map>
#include <string>
#include <xhash>
#include "cursor.h"

#include <D3dx9tex.h>

using google::dense_hash_map;

unsigned int R_HashString(const char* string)
{
	unsigned int hash = 0;

	while (*string)
	{
		hash = (*string | 0x20) ^ (33 * hash);
		string++;
	}

	return hash;
}

// fuck usercall
void Image_PicmipForSemantic_stub(char semantic, int picmap)
{
	__asm
	{
		push eax
		push ebx
		push ecx
		mov al, semantic
		mov ecx, picmap
		mov ebx, 0x520F50
		call ebx
		pop ecx
		pop ebx
		pop eax
	}
}

char __cdecl Image_LoadFromFileWithReaderhk(GfxImage* image, int(__cdecl* OpenFileRead)(const char*, int*))
{
	auto result = ((char(_cdecl*)(GfxImage*, int(__cdecl*)(const char*, int*)))0x544EF0)(image, OpenFileRead);

	if (result)
		return result;

	//for (int i = 0; i < 4; i++)
	{
		auto pngPath = va("images/%s.png", image->name);

		int fh;
		int len = FS_FOpenFileRead(pngPath, &fh, 0);
		if (len != -1)
		{
			std::vector<char> buffer;
			buffer.resize(len);

			FS_Read(buffer.data(), len, fh);
			FS_FCloseFile(fh);

			GfxImageFileHeader iwiHeader =
			{
				{ 'I', 'W', 'i' },
				0x8,
				0x2,
				0x1,
				0,
				{0, 0, 1},
			};


			//image->width = pngImage->GetWidth();
			//image->height = pngImage->GetHeight();
			image->depth = 1;

			iwiHeader.dimensions[0] = image->width;
			iwiHeader.dimensions[1] = image->height;

			//for (int i = 0; i < 8; i++)
			//	iwiHeader.fileSizeForPicmip[i] = pngImage->GetPixelBuffer().size();
			iwiHeader.fileSizeForPicmip[0] += sizeof(GfxImageFileHeader);

			Image_PicmipForSemantic_stub(image->semantic, (int)&image->picmap);
			//Image_LoadBitmap(image, &iwiHeader, NULL, 21, 4); // todo: read real png data
			
			return true;
		}
	}
	
	return false;
}

void PatchMW2_Materialism()
{
	//call(0x521036, Image_LoadFromFileWithReaderhk, PATCH_CALL);
	//call(0x521145, Image_LoadFromFileWithReaderhk, PATCH_CALL);
	//call(0x5213B9, Image_LoadFromFileWithReaderhk, PATCH_CALL);
	//call(0x521446, Image_LoadFromFileWithReaderhk, PATCH_CALL);
}