// ScarfaceFix.cpp
// Minimal DLL that installs a vectored exception handler to catch
// EXCEPTION_FLT_DIVIDE_BY_ZERO and EXCEPTION_INT_DIVIDE_BY_ZERO in 32-bit Scarface.
// Build as a 32-bit DLL.

#include <windows.h>
#include <stdio.h>
#include <stdarg.h>

static PVOID g_handler = nullptr;

// Simple logger to ScarletFix.log in the working directory
static void Log(const char* fmt, ...)
{
    char buf[512];
    va_list ap;
    va_start(ap, fmt);
    _vsnprintf_s(buf, sizeof(buf), _TRUNCATE, fmt, ap);
    va_end(ap);

    FILE* f = nullptr;
    if (fopen_s(&f, "ScarfaceFix.log", "a") == 0 && f) {
        fprintf(f, "%s\n", buf);
        fclose(f);
    }
}

LONG WINAPI VectoredHandler(EXCEPTION_POINTERS* ep)
{
    if (!ep || !ep->ExceptionRecord) return EXCEPTION_CONTINUE_SEARCH;

    DWORD code = ep->ExceptionRecord->ExceptionCode;
    if (code != EXCEPTION_FLT_DIVIDE_BY_ZERO && code != EXCEPTION_INT_DIVIDE_BY_ZERO) {
        return EXCEPTION_CONTINUE_SEARCH;
    }

    // Safety: ensure we're in a scarface process ( crude check )
    char modname[MAX_PATH] = {0};
    if (GetModuleFileNameA(NULL, modname, MAX_PATH)) {
        // convert a copy to lowercase for simple check
        for (char* p = modname; *p; ++p) if (*p >= 'A' && *p <= 'Z') *p = (char)(*p - 'A' + 'a');
        if (!strstr(modname, "scarface")) {
            // not scarface.exe — don't handle
            return EXCEPTION_CONTINUE_SEARCH;
        }
    }

    CONTEXT* ctx = ep->ContextRecord;
    if (!ctx) return EXCEPTION_CONTINUE_SEARCH;

#ifdef _M_IX86
    // Heuristic: Advance EIP slightly to skip offending instruction.
    // NOTE: instruction length varies; 2 is conservative but may need tuning.
    DWORD oldEip = ctx->Eip;
    ctx->Eip = ctx->Eip + 2;

    Log("Caught divide-by-zero at EIP=0x%08X, skipping to 0x%08X (code=0x%08X)",
        oldEip, ctx->Eip, code);

    return EXCEPTION_CONTINUE_EXECUTION;
#else
    // not supported architecture
    return EXCEPTION_CONTINUE_SEARCH;
#endif
}

BOOL InstallHandler()
{
    if (g_handler) return TRUE;
    g_handler = AddVectoredExceptionHandler(1, VectoredHandler);
    if (!g_handler) {
        Log("Failed to install vectored handler.");
        return FALSE;
    }
    Log("Vectored handler installed.");
    return TRUE;
}

void RemoveHandler()
{
    if (g_handler) {
        RemoveVectoredExceptionHandler(g_handler);
        g_handler = nullptr;
        Log("Vectored handler removed.");
    }
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        InstallHandler();
        break;
    case DLL_PROCESS_DETACH:
        RemoveHandler();
        break;
    }
    return TRUE;
}
