
#include <cstdlib>
#include <string>
#include <iostream>

#include <Windows.h>
#include <shlwapi.h>
#include <direct.h>

#if __x86_64__ || _WIN64
	#define ENVIRONMENT64
#else
	#define ENVIRONMENT32
#endif

#include "include/cef_app.h"
#include "ChromiumApp.h"

#ifdef CEF_USE_SANDBOX
	#include "include/cef_sandbox_win.h"

	extern "C"
	{
		__declspec( dllexport ) void* CreateCefSandboxInfo()
		{
			return cef_sandbox_info_create();
		}

		__declspec( dllexport ) void DestroyCefSandboxInfo( void* info )
		{
			cef_sandbox_info_destroy( info );
		}
	}
#endif

using FuncLauncherMain = int (*)(HINSTANCE, HINSTANCE, LPSTR, int);

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow) {
	// Check if "--type=" is in the command arguments. If it is, we are a chromium subprocess.
	if (strstr(lpCmdLine, "--type=")) {
		void* sandbox_info = nullptr;

#ifdef CEF_USE_SANDBOX
		CefScopedSandboxInfo scoped_sandbox;
		sandbox_info = scoped_sandbox.sandbox_info();
#endif

		CefMainArgs main_args(hInstance);
		CefRefPtr<ChromiumApp> app(new ChromiumApp());

		int exit_code = CefExecuteProcess(main_args, app, sandbox_info);
		if (exit_code >= 0) {
			return exit_code;
		}
	}

	char executable_path[MAX_PATH] = { 0 };
	GetModuleFileNameA(NULL, executable_path, MAX_PATH);

	std::string::size_type last_slash = std::string(executable_path).find_last_of("\\/");
	std::string executable_dir = std::string(executable_path).substr(0, last_slash);

#ifdef ENVIRONMENT64
	executable_dir += "\\..\\..";
#else
	executable_dir += "\\..";
	MessageBoxA(NULL, "You may encounter stability issues with GModCEFCodecFix in 32-bit mode. Please launch Garry's Mod in 64-bit mode instead if possible.", "32-bit Warning", 0);
#endif

	_chdir(executable_dir.c_str());

	// Launch GarrysMod's main function from this process. We needed this so the "main" process could provide sandbox information above.
	HMODULE hLauncher = LoadLibraryA("launcher.dll");

	/*
	// If we ever wanted to support launching on the main branch (Awesomium version), we'd use this
	char pathEnvWithBin[MAX_PATH] = { 0 };
	snprintf(pathEnvWithBin, sizeof(pathEnvWithBin), "PATH=%s\\bin\\;%s", executable_dir.c_str(), getenv("PATH"));
	putenv(pathEnvWithBin);

	// Launch GarrysMod's main function from this process. We needed this so the "main" process could provide sandbox information above.
	// 0x0008 = LOAD_WITH_ALTERED_SEARCH_PATH
	HMODULE hLauncher = LoadLibraryExA("bin\\launcher.dll", NULL, 0x0008);
	if (hLauncher == (HMODULE) 0x0) {
		HINSTANCE *lpBuffer = &hInstance;
		DWORD dwLanguageId = 0x400;
		HMODULE Arguments = hLauncher;
		DWORD DVar2 = GetLastError();
		FormatMessageA(0x1300, NULL, DVar2, dwLanguageId, (LPSTR)lpBuffer, (DWORD)hLauncher, (va_list*)Arguments);

		char errorMsg[764];
		snprintf(errorMsg, sizeof(errorMsg), "Failed to load the launcher DLL:\n\n%s", "TODO: Error Msg");
		MessageBoxA(NULL, errorMsg, "Launcher Error", 0);
		LocalFree(hInstance);
		return 0;
	}
	*/

	void* mainFn = static_cast<void*>(GetProcAddress(hLauncher, "LauncherMain"));
	return (static_cast<FuncLauncherMain>(mainFn))(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
}
