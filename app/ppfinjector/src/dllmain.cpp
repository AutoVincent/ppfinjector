#include <iostream>

#include <Windows.h>

#include <detours/detours.h>

namespace {

   auto g_origCreateFileW = CreateFileW;

   HANDLE WINAPI CreateFileWHook(
      _In_ LPCWSTR lpFileName,
      _In_ DWORD dwDesiredAccess,
      _In_ DWORD dwShareMode,
      _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes,
      _In_ DWORD dwCreationDisposition,
      _In_ DWORD dwFlagsAndAttributes,
      _In_opt_ HANDLE hTemplateFile)
   {
      static const auto kConsole = AllocConsole();

      std::wcout << "Opening: " << lpFileName << std::endl;

      return g_origCreateFileW(
         lpFileName,
         dwDesiredAccess,
         dwShareMode,
         lpSecurityAttributes,
         dwCreationDisposition,
         dwFlagsAndAttributes,
         hTemplateFile);
   }

}

BOOL APIENTRY DllMain(HMODULE, DWORD reason, LPVOID)
{
   if (DetourIsHelperProcess()) {
      return TRUE;
   }

    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
       DetourRestoreAfterWith();
       DetourTransactionBegin();
       DetourUpdateThread(GetCurrentThread());
       
       DetourAttach(
          reinterpret_cast<PVOID*>(&g_origCreateFileW),
          CreateFileWHook);

       DetourTransactionCommit();
       break;
    case DLL_PROCESS_DETACH:
       DetourTransactionBegin();
       DetourUpdateThread(GetCurrentThread());
       DetourDetach(
          reinterpret_cast<PVOID*>(&g_origCreateFileW),
          CreateFileWHook);
       DetourTransactionCommit();
        break;
    }

    return TRUE;
}