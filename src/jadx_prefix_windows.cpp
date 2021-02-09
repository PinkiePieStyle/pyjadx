#include "core.hpp"
#include <windows.h>
#include <psapi.h>

namespace jni
{
    std::string get_jadx_prefix(void)
    {
        std::string found = "";

        HANDLE hProcess = GetCurrentProcess(); //python.exe
        HMODULE hMods[1024];
        DWORD cbNeeded;

        if (EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded))
        {
            for (int i = 0; i < (cbNeeded / sizeof(HMODULE)); i++)
            {
                char szModName[MAX_PATH] = {0};
                if (GetModuleFileNameExA(hProcess, hMods[i], szModName,
                                         sizeof(szModName) / sizeof(TCHAR)))
                {
                    std::string modulePath(szModName);
                    std::string fname = modulePath.substr(modulePath.find_last_of("\\/") + 1);
                    if (fname._Starts_with("pyjadx"))
                    {
                        found = modulePath.substr(0, modulePath.find_last_of("\\/"));
                    }
                }
            }
        }
        CloseHandle(hProcess);
        return found;
    }

} // namespace jni