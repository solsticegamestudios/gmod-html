#include <cstdlib>
#include <string>
#include <fstream>
#include <regex>

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

	// Make bin_dir searchable BEFORE any CEF call, so delay-loaded libcef.dll resolves when gmod.exe is at the game root
	// Must run before the "--type=" subprocess branch too: That CEF subprocess also needs libcef.dll
	SetDllDirectoryA(bin_dir.c_str());
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

	// Add GMOD_CEF_FPS_MAX / -chromium_fps_max
	std::string fps_max = "";
	std::string cmdLine = lpCmdLine;

	if (const char* fps_max_env = getenv("GMOD_CEF_FPS_MAX")) {
		fps_max = fps_max_env;
	} else if (strstr(cmdLine.c_str(), "-chromium_fps_max")) {
		const std::regex fps_max_regex("-chromium_fps_max([^-]+)");
		std::smatch fps_max_match;

		if (std::regex_search(cmdLine, fps_max_match, fps_max_regex)) {
			fps_max = fps_max_match[1].str();
		}
	}

	// Write it to a file since APPARENTLY we lose the env var on Linux/macOS
	std::remove("GMOD_CEF_FPS_MAX.txt");
	if (fps_max != "") {
		std::ofstream fps_max_file("GMOD_CEF_FPS_MAX.txt");
		fps_max_file << fps_max;
		fps_max_file.close();
	}

	return mainFn(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
}
