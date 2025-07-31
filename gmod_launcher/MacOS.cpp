#include <sys/syslimits.h>
#include <mach-o/dyld.h>
#include <stdio.h>
#include <string.h>
#include <dlfcn.h>
#include <unistd.h>
#include <fstream>
#include <regex>

typedef int (*LauncherMain_t)(int, char**);

int main(int argc, char** argv)
{
	char executablePath[PATH_MAX];
	unsigned int realPathSizeUInt = sizeof(executablePath);
	int returnCode;

	memset(executablePath, 0, PATH_MAX);
	returnCode = _NSGetExecutablePath(executablePath, &realPathSizeUInt);
	if (returnCode != 0) {
		puts("_NSGetExecutablePath failed");
		return 1;
	}

	for (int i = 1; i <= 4; ++i) {
		char *lastSlash = strrchr(executablePath, '/');
		if (lastSlash != (char *)0) {
			*lastSlash = (char)0;
		}
	}

	returnCode = chdir(executablePath);
	if (returnCode != 0) {
		fprintf(stderr, "Failed to change directory (%s)\n", executablePath);
		return 1;
	}

	void *launcherHandle = dlopen("GarrysMod_Signed.app/Contents/MacOS/launcher.dylib", RTLD_NOW);
	if (!launcherHandle) {
		launcherHandle = dlopen("GarrysMod.app/Contents/MacOS/launcher.dylib", RTLD_NOW);
	}

	if (!launcherHandle) {
		char *errorMsg = dlerror();
		fprintf(stderr, "Failed to load the launcher (%s)\n", errorMsg);
		return 1;
	}

	LauncherMain_t launcherMainFn = (LauncherMain_t)dlsym(launcherHandle, "LauncherMain");

	if (!launcherMainFn) {
		puts("Failed to load the launcher entry proc\n");
		return 1;
	}

	// Add GMOD_CEF_FPS_MAX / -chromium-fps-max
	std::string fps_max = "";
	std::string cmdLine = "";
	for (int i = 1; i < argc; i++) {
		cmdLine += std::string(argv[i]) + " ";
	}

	if (const char* fps_max_env = getenv("GMOD_CEF_FPS_MAX")) {
		fps_max = fps_max_env;
	}
	else if (strstr(cmdLine.c_str(), "-chromium-fps-max")) {
		const std::regex fps_max_regex("-chromium-fps-max([^-]+)");
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

	return launcherMainFn(argc, argv);
}
