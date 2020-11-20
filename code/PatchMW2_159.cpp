// ==========================================================
// MW2 coop
// 
// Component: IW4SP
// Sub-component: clientdll
// Purpose: Patches for version 159
//
// Initial author: momo5502
// Started: 2014-02-15
// ==========================================================

#include "StdInc.h"
#include "159_defs.h"
#include <dbghelp.h>

void PatchMW2_ClientConsole();
void PatchMW2_NoBorder();
void PatchMW2_Coop();
void PatchMW2_Branding();
void PatchMW2_New();
void PatchMW2_UILoading();
void PatchMW2_Minidump();
void PatchMW2_Images();
void PatchMW2_LocalizedStrings();
void PatchMW2_Load();
void PatchMW2_Script();
void PatchMW2_Steam();
void PatchMW2_MusicalTalent();
void PatchMW2_ConsoleStart();
void PatchMW2_Weapons();
void PatchMW2_AssetRestrict();
void PatchMW2_EntsFiles();
void PatchMW2_Achievement();
void PatchMW2_Icon();
void PatchMW2_Materialism();
void PatchMW2_RecoverDevice();
void PatchMW2_OOB();
void PatchMW2_D3D9Ex();

char ingameUsername[32];

char* GetUsername()
{
	if((BOOL)(*dvarName))
	{
		// Quick MW3 patch
		if(version == 382 || version == 358)
			strncpy(ingameUsername, (*((dvar_MW3_t**)dvarName))->current.string, sizeof(ingameUsername));
		else
			strncpy(ingameUsername, (*dvarName)->current.string, sizeof(ingameUsername));
	}
	else
	{
		strncpy(ingameUsername, "Unknown Player", sizeof(ingameUsername));
	}

	return ingameUsername;
}

DWORD SteamUserStuff = 0x4293F0;
DWORD returnSuccess = 0x43FAF9;

void __declspec(naked) steamInitPatch()
{
	__asm
	{
		call SteamUserStuff
		test al, al
		jz returnSafe
		jmp returnSuccess

returnSafe:
		mov al, 1
		retn
	}
}

void** DB_XAssetPool = (void**)0x7337F8;
unsigned int* g_poolSize = (unsigned int*)0x733510;

void* ReallocateAssetPool(assetType_t type, unsigned int newSize)
{
	int elSize = DB_GetXAssetTypeSize(type);
	void* poolEntry = malloc(newSize * elSize);
	DB_XAssetPool[type] = poolEntry;
	g_poolSize[type] = newSize;
	return poolEntry;
}

void* __cdecl memcpy_stub(void* _Dst, void const* _Src, size_t _Size)
{
	// wtf is that shit
	if (_Src == 0)
	{
		Com_Error(ERR_DROP, "You forgot to put rawfile to workspace while building your zone!\n");

		*(char*)_Dst = 0;
		return _Dst;
	}

	return memcpy(_Dst, _Src, _Size);
}

char* __cdecl TryLoadRawfile(int unk1, char* fileName)
{
	Com_Printf(16, "Reading GSC %s\n", fileName);

	return ((char* (__cdecl*)(int, char*))0x4173C0)(unk1, fileName);
}

void PatchMW2_159()
{
	define159Stuff();

	PatchMW2_Minidump();
	PatchMW2_Coop();
	PatchMW2_NoBorder();
	PatchMW2_ClientConsole();
	PatchMW2_Branding();
	PatchMW2_Images();
	PatchMW2_LocalizedStrings();
	PatchMW2_Load();
	PatchMW2_UILoading();
	PatchMW2_Script();
	PatchMW2_Steam();
	// PatchMW2_MusicalTalent();
	//PatchMW2_ConsoleStart();
	PatchMW2_Weapons();
	PatchMW2_AssetRestrict();
	PatchMW2_Achievement();
	// PatchMW2_Icon();
	PatchMW2_Materialism();
	PatchMW2_RecoverDevice();
	PatchMW2_OOB();
	//PatchMW2_D3D9Ex();

	// prevent stat loading from steam
	*(BYTE*)0x43FB33 = 0xC3;

	// remove limit on IWD file loading
	*(BYTE*)0x630FF3 = 0xEB;

	// remove fs_game check for moddable rawfiles - allows non-fs_game to modify rawfiles
	nop(0x612932, 2);

	// Ignore 'steam must be running' error
	nop(0x6040A3, 0x30);

	// Patch steam auth
	call(0x43FAF0, steamInitPatch, PATCH_JUMP);

	// Prevent matchmaking stuff
	*(BYTE*)0x43BAE0 = 0xEB;

	// Remove dvar restrictions
	*(BYTE*)0x635841 = 0xEB; // read only
	*(BYTE*)0x635913 = 0xEB; // cheat protected
	*(BYTE*)0x6358A5 = 0xEB; // write protected
	*(BYTE*)0x635974 = 0xEB; // latched

	// Show intro (or not)
	*(BYTE*)0x6035BD = 0;

	// Unflag dvar intro
	*(BYTE*)0x6035BB = 0;

	// Flag cg_fov as saved
	*(BYTE*)0x41ED35 = DVAR_FLAG_SAVED;

	// Ignore config problems
	*(BYTE*)0x4D3FD3 = 0xEB;

	// Video folders
	*(DWORD*)0x50A0B2 = 0x723390; // raw -> main
	*(DWORD*)0x50A094 = (DWORD)"%s\\" BASEGAME "\\video\\%s.bik"; // main -> data

	// Force debug logging
	nop(0x456BE5, 2);

	// No improper quit popup
	memset((void*)0x4F5B3A, 0x90, 2);

	// Force external console
	memset((void*)0x604071, 0x90, 21);

	// console '%s: %s> ' string
	*(DWORD*)0x579364 = (DWORD)("GFLMOD> ");

	// Remove ''
	nop(0x6030A6, 5);		

	// version string
	//*(DWORD*)0x60426F = (DWORD)(CONSOLESTRING);

	// Apply m2demo stuff	
	*(DWORD*)0x631561 = (DWORD)BASEGAME;

	// Change window titles
	*(DWORD*)0x446A48 = (DWORD)"Girls' Frontline: Modern Warfare 2 - Console";
	*(DWORD*)0x50C110 = (DWORD)"Girls' Frontline: Modern Warfare 2";

	// Yay, hitmarker in sp :D
	Dvar_RegisterBool("scr_damageFeedback", 0, DVAR_FLAG_SAVED, "Show marker when hitting enemies.");

	// Build os path stuff
	*(BYTE*)0x6300BF = 0xEB;

	// Ignore savegame checksum mismatch
	//nop(0x4C78A9, 5);

	// R_MAX_SKINNED_CACHE_VERTICES
	*(DWORD*)0x52046C = 0x480000 * 4;
	*(DWORD*)0x520489 = 0x480000 * 4;
	*(DWORD*)0x52049C = 0x480000 * 4;
	*(DWORD*)0x520506 = 0x480000 * 4;
	*(DWORD*)0x549245 = 0x480000 * 4;
	*(DWORD*)0x549356 = 0x480000 * 4;

	// Increase asset pool
	ReallocateAssetPool(ASSET_TYPE_RAWFILE, 4096);

	// PMem_Init, g_mem size
	*(DWORD*)0x4318ED = 0x140000 * 4;
	*(DWORD*)0x43190E = 0x140000 * 4;
	*(DWORD*)0x431922 = 0x140000 * 4;

	call(0x434634, memcpy_stub, PATCH_CALL);
	call(0x420858, TryLoadRawfile, PATCH_CALL);
}