#include "core.hpp"
#include <windows.h>
#include <psapi.h>
#include <regex>

namespace jni
{
	std::string get_jadx_prefix(void)
	{
		std::string found;

		size_t reqSize;
		getenv_s(&reqSize, NULL, 0, ENV_VAR);
		if ((reqSize > 0) && (reqSize < MAX_PATH)) {
			char* envValue = (char*)malloc(reqSize * sizeof(char));
			if (envValue) {
				getenv_s(&reqSize, envValue, reqSize, ENV_VAR);
				found = std::string(envValue);
				free(envValue);
			}
		}
		else if (reqSize > MAX_PATH) {
			std::cerr << "[-] JADX_PREFIX exceeds MAX_PATH" << std::endl;
		}

		if (found.empty()) {
			HANDLE hProcess = GetCurrentProcess(); //python.exe
			HMODULE hMods[1024];
			DWORD cbNeeded;

			std::regex fn_regex("pyjadx?.+\\.pyd");

			if (EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded))
			{
				for (int i = 0; i < (cbNeeded / sizeof(HMODULE)); i++)
				{
					char szModName[MAX_PATH + 1] = { '\0' };
					if (GetModuleFileNameExA(hProcess, hMods[i], szModName, MAX_PATH)) {
						std::string modulePath(szModName);
						std::string fname = modulePath.substr(modulePath.find_last_of("\\/") + 1);
						if (std::regex_match(fname, fn_regex)) {
							found = modulePath.substr(0, modulePath.find_last_of("\\/"));
						}
					}
				}
			}
			CloseHandle(hProcess);
		}
		return found;
	}
} // namespace jni
