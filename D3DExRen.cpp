// D3DExRen.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <Windows.h>
#include <iostream>
#include <fstream>
#include <vector>

#include <ddraw.h>

#include <SDL.h>
#include <detours.h>
#include "ltlink.h"

#include "LTRen.h"

SDL_Window* g_hSDLWindow = nullptr;
SDL_Surface* g_hScreenSurface = nullptr;
SDL_Renderer* g_hSoftRenderer = nullptr;
SDL_Texture* g_hMainTexture = nullptr;

intptr_t* pBaseAddress;

LPDIRECTDRAW7 g_pDirectDraw = nullptr;
LPDIRECTDRAWSURFACE7 g_pPrimarySurface = nullptr;

////////////////////
// D3D.re
//
typedef void (*FreeModeListFn)(void* param_1);
typedef RMode* (*GetSupportedModesFn)(void);
typedef void (*RenderDLLSetupFn)(DLLRenderStruct* pRenderStruct);

typedef void (*SwapBuffersFn)(uint32 nFlags);

void (*d3d_FreeModeList)(void* param_1);
RMode* (*d3d_GetSupportedModes)(void);
void (*d3d_RenderDLLSetup)(DLLRenderStruct* pRenderStruct);

void (*d3d_SwapBuffers)(uint32 nFlags);
uint32 (*d3d_Init)(InitStruct* pInitStruct);
////////////////////
#if 1
// SDL Logging
std::fstream g_SDLLogFile;

void SDLLog(void* userdata, int category, SDL_LogPriority priority, const char* message)
{
	// Open up SDL Log File
	g_SDLLogFile.open("D3DRenEx.log", std::ios::out | std::ios::app);

	g_SDLLogFile << message << "\n";

	g_SDLLogFile.close();
}
#endif

// EXPORTS
#define DllExport   __declspec( dllexport )
// DLL Export Functions
extern "C"
{
	DllExport int entry(unsigned int param_1, int param_2, unsigned int param_3);
	DllExport void __cdecl FreeModeList(void* param_1);
	DllExport RMode* GetSupportedModes(void);
	DllExport void __cdecl RenderDLLSetup(DLLRenderStruct* pRenderStruct);
};

//////////////////////////
// D3DExRen.ren

struct drawStruct {
	unsigned int surfacePtr;
	unsigned int unknown[2];
	unsigned int srcPtr;
	unsigned int dstPtr;
	float fadePercentage; // 0..1
	unsigned int unknown2;
	unsigned int colourKey;
};
// Gets r, g, and b as 0-255 integers.
#define GETR(val) (((val) >> 16) & 0xFF)
#define GETG(val) (((val) >> 8) & 0xFF)
#define GETB(val) ((val) & 0xFF)
#define GETRGB(val, r, g, b) \
	{\
		(r) = GETR(val);\
		(g) = GETG(val);\
		(b) = GETB(val);\
	}


uint32 d3dex_Init(InitStruct* pInitStruct)
{

	auto nInitResult = d3d_Init(pInitStruct);


	//0x44CBF74 D:\Games\NOLF - dev\d3d.ren	0000000004440000	00000000000B5000
	intptr_t* d3dRenBase = pBaseAddress;//(intptr_t*)0x4440000;

	//D:\Games\NOLF - dev\d3d.ren	0000000003C20000	00000000000B5000

	intptr_t* nD3DRenSize = (intptr_t*)(((char*)pBaseAddress) + 0x0B5000);

	//intptr_t* pEnd = (intptr_t*)0x0B5000;
	//intptr_t* nD3DRenSize = pBaseAddress + pEnd;

	intptr_t* pDirectDraw = *(intptr_t**)(((char*)pBaseAddress) + 0x8bf74);//0x8BF74;
	intptr_t* pPrimarySurface = *(intptr_t**)(((char*)pBaseAddress) + 0x8bf6c);//0x8bf68);//0x8bf6c);

	// Offsets
	// d3d_CreateDevice = 0x331e0

	
	g_pDirectDraw = (LPDIRECTDRAW7)pDirectDraw;
	g_pPrimarySurface = (LPDIRECTDRAWSURFACE7)pPrimarySurface;

	LPDIRECTDRAW pdd = (LPDIRECTDRAW)pDirectDraw;
	
	DDSURFACEDESC      ddsd;
	ZeroMemory(&ddsd, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);

	pdd->GetDisplayMode(&ddsd);

	g_hSDLWindow = SDL_CreateWindowFrom(GetFocus());

	if (!g_hScreenSurface)
	{
		g_hScreenSurface = SDL_CreateRGBSurfaceWithFormat(0, pInitStruct->renderMode.m_Width, pInitStruct->renderMode.m_Height, 32, SDL_PIXELFORMAT_RGB888);
		//g_hSoftRenderer = SDL_CreateSoftwareRenderer(g_hScreenSurface);
		g_hSoftRenderer = SDL_CreateRenderer(g_hSDLWindow, -1, 0);

		g_hMainTexture = SDL_CreateTexture(g_hSoftRenderer, SDL_PIXELFORMAT_RGB888, SDL_TEXTUREACCESS_STREAMING, pInitStruct->renderMode.m_Width, pInitStruct->renderMode.m_Height);//SDL_CreateTextureFromSurface(g_hSoftRenderer, g_hScreenSurface);

	}

	return nInitResult;
}

uint32 d3dex_BlitToScreen(intptr_t* pBlitRequest)
{
	//SDL_Log("Calling BlitToScreen");
#if 0
	// 3d mode?
	if (DAT_1008d7c8 != 0) {
		// Throw = "Warning: drawing a nonoptimized surface while in 3D mode."
	}

#endif

	drawStruct* ds = (drawStruct*)pBlitRequest;

	unsigned int* unknownPtr = (unsigned int*)ds->unknown[1];
	unsigned int* unknownPtr2 = (unsigned int*)ds->unknown2;

	SDL_Surface* surface = (SDL_Surface*)ds->surfacePtr;

	// These come pre-clipped!
	LTRect* src = (LTRect*)ds->srcPtr;
	LTRect* dst = (LTRect*)ds->dstPtr;

	SDL_Rect sdlSrc = { src->left, src->top, src->right, src->bottom };
	SDL_Rect sdlDst = { dst->left, dst->top, dst->right, dst->bottom };

	// SDL doesn't want left or bottom, it wants width and height!
	sdlSrc.w -= sdlSrc.x;
	sdlSrc.h -= sdlSrc.y;
	sdlDst.w -= sdlDst.x;
	sdlDst.h -= sdlDst.y;

	if (ds->fadePercentage != 1.0f) {
		SDL_SetSurfaceAlphaMod(surface, ds->fadePercentage * 255);
		SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_BLEND);
	}

	unsigned int colourKeyLT = ds->colourKey;

	// SDL uses a per format rgb, LT probably just uses 8 bits? Convert it!
	int r, g, b;
	GETRGB(colourKeyLT, r, g, b);

	SDL_SetColorKey(surface, 1, SDL_MapRGB(surface->format, r, g, b));

	int result = SDL_BlitScaled(surface, &sdlSrc, g_hScreenSurface, &sdlDst);

	if (result != 0) {
		const char* error = SDL_GetError();
	}

	if (ds->fadePercentage != 1.0f) {
		SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_NONE);
	}

	return 1;
}

intptr_t* d3dex_CreateSurface(uint32 nWidth, uint32 nHeight)
{
	SDL_Log("Calling CreateSurface");
	SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(0, nWidth, nHeight, 32, SDL_PIXELFORMAT_RGB888);

	//g_OpenRen->m_SurfaceCache.push_back(surface);

	return (intptr_t*)surface;
}
//0xcc
// Most likely DestroySurface
void d3dex_DestroySurface(intptr_t* pSurface)
{
	SDL_Log("Calling DestroySurface");

	SDL_Surface* surface = (SDL_Surface*)pSurface;

	SDL_FreeSurface(surface);
	pSurface = nullptr;

	return;
}

// Lock Surface!
void* d3dex_LockSurface(intptr_t* pSurface)
{
	//SDL_Log("Calling LockSurface");
	SDL_Surface* surface = (SDL_Surface*)pSurface;

	if (surface == NULL) {
		return 0;
	}

	return (void*)surface->pixels;
}

//0xd8
void d3dex_UnlockSurface(intptr_t* pSurface)
{
	//SDL_Log("Calling UnlockSurface");
	if (pSurface == NULL) {
		return;
	}

	SDL_Surface* surface = (SDL_Surface*)pSurface;
}

void d3dex_GetSurfaceDims(intptr_t* pSurface, uint32* pWidth, uint32* pHeight, uint32* pPitch)
{
	SDL_Log("Calling GetSurfaceDims");
	if (pSurface == nullptr) {
		return;
	}
	SDL_Surface* surface = (SDL_Surface*)pSurface;

	if (surface == nullptr) {
		return;
	}

	// Maybe it's surface info? Odd how it already has the info filled in :thinking:
	*pWidth = surface->w;
	*pHeight = surface->h;
	*pPitch = surface->pitch;
}

uint32 d3dex_OptimizeSurface(intptr_t* pSurface)
{
	SDL_Log("Calling OptimizeSurface");

	return 1;
}

void d3dex_UnoptimizeSurface(intptr_t* pSurface)
{
	SDL_Log("Calling UnoptimizeSurface");
}

void d3dex_SwapBuffers(uint32 nFlags)
{
	//SDL_Log("Calling Flip");

	//d3d_SwapBuffers(nFlags);

	DDSURFACEDESC2      ddsd;
	ZeroMemory(&ddsd, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);

	auto result = g_pPrimarySurface->Lock(NULL, &ddsd, DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT, NULL);




	
	SDL_RenderClear(g_hSoftRenderer);

	if (g_hScreenSurface == nullptr) {
		return;
	}

	void* pixels = nullptr;
	int pitch = 0;

	if (SDL_LockTexture(g_hMainTexture, NULL, &pixels, &pitch))
	{
		return;
	}

	SDL_Rect dstrct = { 0, 0, g_hScreenSurface->w, g_hScreenSurface->h};

	memcpy(pixels, g_hScreenSurface->pixels, pitch * g_hScreenSurface->h);
	//memcpy_s(pixels, pitch * m_height, srcPixels, srcPitch * m_height);


	SDL_UnlockTexture(g_hMainTexture);

	SDL_RenderCopy(g_hSoftRenderer, g_hMainTexture, NULL, NULL);

	SDL_RenderPresent(g_hSoftRenderer);

	SDL_FillRect(g_hScreenSurface, NULL, 0x000000);

	g_pPrimarySurface->Unlock(NULL);

	return;
}

//////////////////////////

//////////////////////////
// D3D.Ren


DllExport void FreeModeList(void* param_1)
{
	d3d_FreeModeList(param_1);
}

DllExport RMode* GetSupportedModes(void)
{
	RMode* pRenderMode = d3d_GetSupportedModes();

	return pRenderMode;
}

DllExport void RenderDLLSetup(DLLRenderStruct* pRenderStruct)
{
	d3d_RenderDLLSetup(pRenderStruct);

	// Hook in our functions
	pRenderStruct->CreateSurface = d3dex_CreateSurface;
	pRenderStruct->DeleteSurface = d3dex_DestroySurface;
	pRenderStruct->LockSurface = d3dex_LockSurface;
	pRenderStruct->UnlockSurface = d3dex_UnlockSurface;
	pRenderStruct->GetSurfaceDims = d3dex_GetSurfaceDims;

	pRenderStruct->OptimizeSurface = d3dex_OptimizeSurface;
	pRenderStruct->UnoptimizeSurface = d3dex_UnoptimizeSurface;

	pRenderStruct->BlitToScreen = d3dex_BlitToScreen;

	d3d_Init = pRenderStruct->Init;
	pRenderStruct->Init = d3dex_Init;

	d3d_SwapBuffers = pRenderStruct->SwapBuffers;
	pRenderStruct->SwapBuffers = d3dex_SwapBuffers;



	bool end = true;
}


//////////////////////////


// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file

///
/// MAIN
///
BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	// Ignore if detour
	if (DetourIsHelperProcess()) {
		return TRUE;
	}

	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	{
		SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);

		// If we're not debug, clear the debug log
#ifdef NDEBUG
		g_SDLLogFile.open("Lithfix.log", std::ios::out | std::ios::trunc);
		g_SDLLogFile << "";
		g_SDLLogFile.close();
#endif
		// Setup the logging function
		//SDL_LogSetOutputFunction(&SDLLog, NULL);

		SDL_Log("D3DEx Renderer Beta 1");
		SDL_Log("Hello world, Let's get going!");

		TCHAR szExeFileName[MAX_PATH];
		GetModuleFileName(NULL, szExeFileName, MAX_PATH);

		SDL_Log("EXE NAME: <%s>", szExeFileName);

		// Grab the hWND and turn it into a SDL_Window
		//g_hSDLWindow = SDL_CreateWindowFrom(GetFocus());

		// TODO: Throw an error?
		if (!g_hSDLWindow) {
			//SDL_Quit();
		}


		HMODULE hLib;

		// Load the real dll file. Should be in the game's root
		// TODO: Make configurable
		hLib = LoadLibrary("./d3d.ren");

		if (!hLib) {
			return FALSE;
		}

		pBaseAddress = (intptr_t *) hLib;

		d3d_FreeModeList = (FreeModeListFn)GetProcAddress(hLib, "FreeModeList");
		d3d_GetSupportedModes = (GetSupportedModesFn)GetProcAddress(hLib, "GetSupportedModes");
		d3d_RenderDLLSetup = (RenderDLLSetupFn)GetProcAddress(hLib, "RenderDLLSetup");



		break;
	}
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}