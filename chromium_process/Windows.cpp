#include <cstdlib>
#include <string>

#include <Windows.h>
#include <shlwapi.h>

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

typedef int (*LauncherMain_t)(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow) {
	// TODO: Unicode paths (GetModuleFileNameW, etc)
	char executable_path[MAX_PATH] = { 0 };
	GetModuleFileNameA(NULL, executable_path, MAX_PATH);

	std::string::size_type last_slash = std::string(executable_path).find_last_of("\\/");
	std::string executable_dir = std::string(executable_path).substr(0, last_slash);

	// Find launcher.dll: Next to us (x86-64, gmod.exe in bin/), else bin\win64\ or bin\ (public/dev, gmod.exe at game root)
	std::string bin_dir = executable_dir;
	if (PathFileExistsA((executable_dir + "\\launcher.dll").c_str())) {
		bin_dir = executable_dir;
	}
#ifdef ENVIRONMENT64
	else if (PathFileExistsA((executable_dir + "\\bin\\win64\\launcher.dll").c_str())) {
		bin_dir = executable_dir + "\\bin\\win64";
	}
#else
	else if (PathFileExistsA((executable_dir + "\\bin\\launcher.dll").c_str())) {
		bin_dir = executable_dir + "\\bin";
	}
#endif

	// Pre-load delay-loaded libcef.dll by full path BEFORE any CEF call, so it resolves when gmod.exe is at the game root
	// Must run before the "--type=" subprocess branch too: That CEF subprocess also needs libcef.dll
	// NOT SetDllDirectory: That removes CWD from the DLL search path and aborts garrysmod_common binary modules
	std::string libcef_path = bin_dir + "\\libcef.dll";
	HMODULE hLibcef = LoadLibraryExA(libcef_path.c_str(), NULL, LOAD_WITH_ALTERED_SEARCH_PATH);

	if (!hLibcef) {
		// Grab the error now; the bare-name retry below would clobber it
		DWORD err = GetLastError();

		// Last chance: The normal search might still find one, ex. if bin_dir somehow came out wrong
		hLibcef = LoadLibraryA("libcef.dll");

		if (!hLibcef) {
			// We can't show UI from a subprocess (it might be sandboxed onto another desktop), and Chromium respawns it anyway
			// Exit 0xCEF0xxxx (xxxx = the Win32 error) so chromium.log's exit_code tells us why instead of a delay-load crash
			if (strstr(lpCmdLine, "--type=")) {
				return 0xCEF00000 | (err & 0xFFFF);
			}

			LPSTR err_msg = NULL;
			FormatMessageA(
				FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL,
				err,
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				(LPSTR) &err_msg,
				0,
				NULL
			);

			std::string msg = "Couldn't load the Chromium html engine:\n" + libcef_path
				+ "\n\nError " + std::to_string(err) + ": " + (err_msg ? err_msg : "<Couldn't format error message>")
				+ "\nThe main menu and other web content will not work."
				+ "\nSomething may have removed or blocked the file (ex. antivirus)."
				+ "\n\nRe-run GModPatchTool to repair the game, and include this exact error if you report the problem.";
			MessageBoxA(NULL, msg.c_str(), "Launch Error: libcef.dll", MB_ICONERROR);

			if (err_msg) {
				LocalFree(err_msg);
			}
		}
	}

	// Keep bin_dir on PATH for the bare-name libraries CEF loads at runtime
	std::string new_path = "PATH=" + bin_dir + ";";
	if (const char* old_path = getenv("PATH")) {
		new_path += old_path;
	}
	_putenv(new_path.c_str());

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

	// launcher.dll derives the base dir from our module path and _chdir's there itself
	// At the game root that's correct; in bin/ Source walks up from there to find the game
	HMODULE hLauncher = LoadLibraryExA((bin_dir + "\\launcher.dll").c_str(), NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
	LauncherMain_t mainFn = (LauncherMain_t)(GetProcAddress(hLauncher, "LauncherMain"));

	if (!mainFn) {
		DWORD err = GetLastError();
		LPVOID err_msg;

		int err_format_result = FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			err,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPWSTR) &err_msg,
			0,
			NULL
		);

		if (err_format_result == 0) {
			MessageBoxW(NULL, L"<Couldn't format error message>", L"Launch Error: GetProcAddress", MB_ICONERROR);
		} else {
			MessageBoxW(NULL, (LPWSTR) err_msg, L"Launch Error: GetProcAddress", MB_ICONERROR);
			LocalFree(err_msg);
		}

		return err;
	}

	return mainFn(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
}
