#include "interface.h"

InterfaceReg* s_pInterfaceRegs;

InterfaceReg::InterfaceReg(InstantiateInterfaceFn fn, const char* pName) :
	m_pName(pName)
{
	m_CreateFn = fn;
	m_pNext = s_pInterfaceRegs;
	s_pInterfaceRegs = this;
}
