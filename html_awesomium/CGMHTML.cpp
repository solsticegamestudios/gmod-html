#include "interface.h"
#include "IGMHTML.h"
#include <iostream>
#include <Windows.h>

#if __x86_64__ || _WIN64
#define ENVIRONMENT64
#else
#define ENVIRONMENT32
#endif

extern "C" __declspec(dllexport) void* CreateInterface(const char* pName, int* pReturnCode)
{
	MessageBoxA(NULL, pName, "CGMHTML::CreateInterface", 0);

	if (pReturnCode) {
		*pReturnCode = 69;
	}

	return 0;
}

class CGMHTML : public IGMHTML
{
public:
	CGMHTML()
	{
		MessageBoxA(NULL, "", "CGMHTML()", 0);
	}

	~CGMHTML()
	{
		MessageBoxA(NULL, "", "~CGMHTML()", 0);
	}

	virtual void Release()
	{
		MessageBoxA(NULL, "", "CGMHTML::Release", 0);
		delete this;
	}

	// You must call this first to set the hwnd.
	virtual void Init(CreateInterfaceFn factory, void* parentHwnd)
	{
		MessageBoxA(NULL, "", "CGMHTML::Init", 0);
	}

};

EXPOSE_INTERFACE(CGMHTML, IGMHTML, IGMHTML_VERSION);
