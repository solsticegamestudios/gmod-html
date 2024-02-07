#ifndef IGMHTML_H
#define IGMHTML_H
#ifdef _WIN32
#pragma once
#endif

#include "interface.h"

#define NULL 0
#define IGMHTML_VERSION	"IGMHTML001"

class IGMHTML
{
public:
	// You must call this first to set the hwnd.
	virtual void Init(CreateInterfaceFn factory, void* parentHwnd) = 0;

	// Call this to free the instance.
	virtual void Release() = 0;
};

#endif // IGMHTML_H
