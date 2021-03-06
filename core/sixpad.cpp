#include "global.h"
#include "sixpad.h"

HINSTANCE dllHinstance=0;
SixpadData* sp = 0;

extern "C" BOOL export SixpadDLLInit (SixpadData* d) {
sp = d;
sp->dllHinstance = dllHinstance;
return true;
}

extern "C" BOOL export CallSixPadDLLInit (SixpadDLLInitFunc func) {
return func(sp);
}

extern "C" BOOL WINAPI __declspec(dllexport) DllMain (HINSTANCE hDll, DWORD reason, LPVOID unused) {
if (reason==DLL_PROCESS_ATTACH) {
dllHinstance = hDll;
//DisableThreadLibraryCalls(hDll);
}
return true;
}

