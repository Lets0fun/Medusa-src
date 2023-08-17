#include <memory>
#include <clocale>
#include <Windows.h>
#include "Hooks.h"
#include <fstream>
#include <iostream>
#include <mmsystem.h>
#include "xor.h"
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "shlwapi.lib")
extern "C" BOOL WINAPI _CRT_INIT(HMODULE moduleHandle, DWORD reason, LPVOID reserved);


BOOL APIENTRY DllEntryPoint(HMODULE moduleHandle, DWORD reason, LPVOID reserved)
{
    if (!_CRT_INIT(moduleHandle, reason, reserved))
        return FALSE;

    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(moduleHandle);

        auto current_process = GetCurrentProcess();
        auto priority_class = GetPriorityClass(current_process);

        if (priority_class != HIGH_PRIORITY_CLASS && priority_class != REALTIME_PRIORITY_CLASS)
            SetPriorityClass(current_process, HIGH_PRIORITY_CLASS);

        std::setlocale(LC_CTYPE, ".utf8");
        PlaySoundA(skCrypt("C:\\Windows\\Media\\notify"), NULL, SND_SYNC);
        hooks = std::make_unique<Hooks>(moduleHandle);
    }
    return TRUE;
}
