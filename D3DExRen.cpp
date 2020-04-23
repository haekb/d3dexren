// D3DExRen.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <Windows.h>
#include <iostream>
#include <fstream>
#include <vector>


#include <SDL.h>
#include <detours.h>
#include "ltlink.h"

#include "LTRen.h"

SDL_Window* g_hSDLWindow = NULL;

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
// D3D.Ren
typedef void (*FreeModeListFn)(void* param_1);
typedef RMode* (*GetSupportedModesFn)(void);
typedef void (*RenderDLLSetupFn)(DLLRenderStruct* pRenderStruct);

void (*d3d_FreeModeList)(void* param_1);
RMode* (*d3d_GetSupportedModes)(void);
void (*d3d_RenderDLLSetup)(DLLRenderStruct* pRenderStruct);

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
	return d3d_RenderDLLSetup(pRenderStruct);
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