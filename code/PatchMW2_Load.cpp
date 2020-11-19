// ==========================================================
// MW2 coop
// 
// Component: IW4SP
// Sub-component: clientdll
// Purpose: Fastfile loading modifications
//
// Initial author: momo5502
// Started: 2014-03-08
// ==========================================================

#include "stdinc.h"
#include <ShellAPI.h>
#include <sys/stat.h>
#include <direct.h>
#include <io.h>
#include <stack>

dvar_t* specialops;

void* ReallocateAssetPool(int type, unsigned int newSize);

// Allow civilians to be killed in 'No Russian' if game is censored.
void uncutGame(XZoneInfo* data)
{
	// Only in campaign!
	if(!specialops)
		specialops = Dvar_FindVar("specialops");

	Dvar_SetCommand("friendlyfire_dev_disabled", ((strcmp(data->name, "airport") || specialops->current.boolean) ? "0" : "1"));
}

void cinematic_f()
{
	((void(*)())(version == 159 ? 0x4CC950 : 0x4BDE20))(); // Call cinematics
	*(BOOL*)(version == 159 ? 0x73264C : 0x72F64C) = !strcmp(Cmd_Argv(1), "intro_credits_load"); // Allow skipping if intro
}

char returnPath[MAX_PATH];

char* addAlterZones(char* zone)
{
	if(GetFileAttributes(va( "zone\\custom\\%s", zone)) != INVALID_FILE_ATTRIBUTES)
	{
		strcpy(returnPath, "zone\\custom\\");
	}
	else if(GetFileAttributes(va("zone\\dlc\\%s", zone)) != INVALID_FILE_ATTRIBUTES)
	{ 
		strcpy(returnPath, "zone\\dlc\\");
	}
	else
	{
		sprintf(returnPath, "zone\\%s\\", language); // User's language
	}

	return returnPath;
}

static DWORD gameWorldSP;
static DWORD gameWorldMP;

void GetBSPNameHookFunc(char* buffer, size_t size, const char* format, const char* mapname)
{
	// the usual stuff
	if (!_strnicmp("mp_", mapname, 3))
	{
		format = "maps/mp/%s.d3dbsp";
	}

	_snprintf(buffer, size, format, mapname);

	// check for being MP/SP, and change data accordingly
	if (_strnicmp("mp_", mapname, 3))
	{
		if(version == 159)
		{
			// SP
			*(DWORD*)0x4B0921 = gameWorldSP + 4;		// some game data structure
		}
		else
		{
			// SP
			*(DWORD*)0x4D4AA1 = gameWorldSP + 4;		// some game data structure
		}
	}
	else
	{
		if(version == 159)
		{
			// MP
			*(DWORD*)0x4B0921 = gameWorldMP + 52;		// some game data structure
		}
		else
		{
			// MP
			*(DWORD*)0x4D4AA1 = gameWorldMP + 52;		// some game data structure
		}
	}
}

typedef struct  
{
	char unknown[16];
} xAssetEntry_t;

static xAssetEntry_t xEntries[789312];

void ReallocXAssetEntries()
{
	int newsize = 516 * 2048;
	//newEnts = malloc(newsize);

	// get (obfuscated) memory locations
	unsigned int origMin = 0xB2B360;
	unsigned int origMax = 0xB2B370;

	// scan the .text memory
	char* scanMin = (char*)0x401000;
	char* scanMax = (char*)0x694000;

	if(version == 184)
	{
		origMin = 0xB27DE0;
		origMax = 0xB27DF0;
		scanMax = (char*)0x691000;
	}

	unsigned int difference = (unsigned int)xEntries - origMin;

	char* current = scanMin;

	for (; current < scanMax; current += 1) {
		unsigned int* intCur = (unsigned int*)current;

		// if the address points to something within our range of interest
		if (*intCur == origMin || *intCur == origMax) {
			// patch it
			*intCur += difference;
		}
	}

	if(version == 159)
		*(DWORD*)0x581740 = 789312;
	else
		*(DWORD*)0x57EB40 = 789312;
}

// Experimental 4K resolution
dvar_t* YUNO4K(const char* name, char** enumValues, int default, int flags, const char* description)
{
	int enumSize = 18;
	char** newEnum = (char**)malloc_n(sizeof(DWORD) * (enumSize + 2));
	memcpy(newEnum, enumValues, sizeof(DWORD) * enumSize);
	newEnum[enumSize] = "3840x2160";
	return Dvar_RegisterEnum(name, newEnum, default, flags, description);
}

bool ListFiles(string path, string mask, vector<string>& files) {
	HANDLE hFind = INVALID_HANDLE_VALUE;
	WIN32_FIND_DATA ffd;
	string spec;
	std::stack<string> directories;

	directories.push(path);
	files.clear();

	while (!directories.empty()) {
		path = directories.top();
		spec = path + "\\" + mask;
		directories.pop();

		hFind = FindFirstFile(spec.c_str(), &ffd);
		if (hFind == INVALID_HANDLE_VALUE) {
			return false;
		}

		do {
			if (strcmp(ffd.cFileName, ".") != 0 &&
				strcmp(ffd.cFileName, "..") != 0) {
				if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
					directories.push(path + "\\" + ffd.cFileName);
				}
				else {
					files.push_back(path + "\\" + ffd.cFileName);
				}
			}
		} while (FindNextFile(hFind, &ffd) != 0);

		if (GetLastError() != ERROR_NO_MORE_FILES) {
			FindClose(hFind);
			return false;
		}

		FindClose(hFind);
		hFind = INVALID_HANDLE_VALUE;
	}

	return true;
}

void Load_XSurfaceArrayFix(int shouldLoad, int count)
{
	// read the actual count from the varXModelSurfs ptr
	auto surface = *reinterpret_cast<XModelSurfs**>(0x9DB05C);

	// call original read function with the correct count
	return ((void(*)(int, int))0x44C880)(shouldLoad, surface->numSurfaces);
}

void LoadCustomZones(XZoneInfo* data, int count, int sync)
{
	std::vector<XZoneInfo> infos;

	for (int i = 0; i < count; i++)
		infos.push_back(data[i]);

	std::vector<string> files;

	if (ListFiles("zone\\custom", "*.ff", files))
	{
		for (auto &file : files)
		{
			std::string fastfileName(file.substr(12, file.length() - 15));

			XZoneInfo info = 
			{
				strdup(fastfileName.c_str()),
				1,
				0
			};

			infos.insert(infos.begin(), info);

			//infos.push_back();
		}
	}

	return DB_LoadXAssets(infos.data(), infos.size(), sync);
}

void LoadGfxPatch(XZoneInfo* data, int count, int sync)
{
	std::vector<XZoneInfo> infos;

	for (int i = 0; i < count; i++)
		infos.push_back(data[i]);

	if (GetFileAttributesA("zone\\english\\gfx_patch.ff") != INVALID_FILE_ATTRIBUTES)
	{
		XZoneInfo info =
		{
			"gfx_patch",
			0,
			0
		};

		infos.insert(infos.begin(), info);
	}

	return DB_LoadXAssets(infos.data(), infos.size(), sync);
}

void __cdecl loadTeamFile(XZoneInfo* data, int count, int unknown)
{
	XZoneInfo* newData = (XZoneInfo*)malloc_n(sizeof(XZoneInfo) * (count + 1));
	memcpy(newData, data, sizeof(XZoneInfo) * count);

	// Still bugged. probably need to compile an own fastfile
	newData[count].name = "custom";
	newData[count].type1 = data->type1;
	newData[count++].type2 = data->type2;

	uncutGame(newData);

	_allowZoneChange = true;

	DB_LoadXAssets(newData, count, unknown);
}

void hkCom_Error(const char* message, ...)
{
	printf(message);
}



void __declspec(naked) R_GetCharacterGlyph_UsercallHk()
{
	__asm 
	{

	}
}


DWORD decodeLetterOrigPos = 0x461B85;

uint16_t decodedcrap[2] = { 0, 0 };
char* decodeedstring = (char*)decodedcrap;

uint16_t __cdecl SEH_DecodeLetterStub(uint8_t firstChar, uint8_t secondChar, int* usedCount, int* pbIsTrailingPunctuation)
{
	uint16_t result = ((uint16_t(__cdecl*)(uint8_t, uint8_t, int *, int *))0x461B80)(firstChar, secondChar, usedCount, pbIsTrailingPunctuation);
	
	decodedcrap[0] = result;

	printf("%s", decodeedstring);

	return result;
}

static char defaultFont[32];
static char hudFont[32];
static char objectiveFont[32];

void PatchMW2_Load()
{
	if(version == 159)
	{
		// Ignore zone version missmatch
		*(BYTE*)0x4256D8 = 0xEB;

		// Ignore 'Disc read error.'
		nop(0x4B7335, 2);
		*(BYTE*)0x4B7356 = 0xEB;
		//*(BYTE*)0x413629 = 0xEB;
		//*(BYTE*)0x581227 = 0xEB;
		*(BYTE*)0x4256B9 = 0xEB;

		call(0x45EE95, Load_XSurfaceArrayFix, PATCH_CALL);
		call(0x50B637, LoadCustomZones, PATCH_CALL);
		call(0x50B595, LoadGfxPatch, PATCH_CALL);

		//nop(0x41CCBC, 5);

		//call(0x41611A, SEH_DecodeLetterStub, PATCH_CALL);
		//call(0x508BF6, SEH_DecodeLetterStub, PATCH_CALL);
		//call(0x577623, SEH_DecodeLetterStub, PATCH_CALL);

		gameWorldSP = (*(DWORD*)0x4B0921) - 4;

		//call(0x40BFF0, hkCom_Error, PATCH_JUMP);
	}
	else if(version == 184)
	{
		// Ignore zone version missmatch
		*(BYTE*)0x4A4E98 = 0xEB;

		// Ignore 'Disc read error.'
		nop(0x4F6E05, 2);
		//*(BYTE*)0x4F6E26 = 0xEB;
		//*(BYTE*)0x47FE69 = 0xEB;
		//*(BYTE*)0x57E637 = 0xEB;
		*(BYTE*)0x4A4E79 = 0xEB;

		gameWorldSP = (*(DWORD*)0x4D4AA1) - 4;
	}

	gameWorldMP = (DWORD)ReallocateAssetPool(ASSET_TYPE_GAME_MAP_MP, 1);

	call(getBSPNameHookLoc, GetBSPNameHookFunc, PATCH_CALL);
	//call(ffLoadHook1Loc, loadTeamFile, PATCH_CALL);
	call(zoneLoadHookLoc, addAlterZones, PATCH_CALL);

	ReallocXAssetEntries();

	// Allow campaign intro to be skipped :P
	*(DWORD*)(version == 159 ? 0x47529F : 0x4F39AF) = (DWORD)cinematic_f;

	// 4k stuff causes problems
	//call((version == 159 ? 0x50BBD2 : 0x50B302), YUNO4K, PATCH_CALL);

	//GetPrivateProfileStringA("CODMOD", "default", "fonts/default", defaultFont, 1000, "./iw4sp.ini");
	//GetPrivateProfileStringA("CODMOD", "hud", "fonts/hud", hudFont, 1000, "./iw4sp.ini");
	//GetPrivateProfileStringA("CODMOD", "objective", "fonts/objective", objectiveFont, 1000, "./iw4sp.ini");
	//
	//*(char**)0x620FAD = defaultFont;
	//*(char**)0x620FBE = defaultFont;
	//*(char**)0x620FE0 = defaultFont;
	//*(char**)0x620FF4 = defaultFont;
	//*(char**)0x621005 = defaultFont;
	//
	//*(char**)0x621016 = objectiveFont;
	//
	//*(char**)0x621027 = hudFont;
	//*(char**)0x621038 = hudFont;
}
