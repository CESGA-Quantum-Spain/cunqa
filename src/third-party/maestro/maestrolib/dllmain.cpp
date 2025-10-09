/**
 * @file dllmain.cpp
 * @version 1.0
 *
 * @section DESCRIPTION
 * Implementation for the library functionality.
 */

#include "../Simulators/Factory.cpp"

#ifndef __linux__

#include "framework.h"

// Defines the entry point for the DLL application.

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

#endif

