#include "hook_status.h"
#include "read_ops_log.h"

#include <ppftk/rom_patch/flat_patch.h>
#include <ppftk/rom_patch/ppf/parser.h>

#include <ppfbase/branding.h>
#include <ppfbase/logging/logging.h>
#include <ppfbase/filesystem/file.h>
#include <ppfbase/filesystem/path_service.h>
#include <ppfbase/process/this_module.h>

#include <atomic>
#include <filesystem>
#include <iostream>
#include <memory>
#include <optional>

#include <Windows.h>

#include <detours/detours.h>

namespace tdd::app::ppfinjector {
namespace {

   namespace fs = std::filesystem;

   auto g_origCreateFileW = CreateFileW;
   auto g_origReadFile = ReadFile;
   auto g_origReadFileEx = ReadFileEx;
   auto g_origCloseHandle = CloseHandle;

   std::unique_ptr<ReadOpsLog> g_readOpsLog;
   tk::rompatch::FlatPatch g_patch;
   std::atomic<HANDLE> g_targetFile = INVALID_HANDLE_VALUE;

   struct [[nodiscard]] LastErrorRestorer
   {
      LastErrorRestorer() noexcept
         : m_lastErr(::GetLastError())
      {}

      ~LastErrorRestorer() noexcept
      {
         ::SetLastError(m_lastErr);
      }

      const DWORD m_lastErr;

   private:
      TDD_DISABLE_COPY_MOVE(LastErrorRestorer);
   };

   [[nodiscard]] bool IsWindowsFile(const fs::path& file)
   {
      static const fs::path kWindows(L"C:\\Windows\\");

      const auto [winEnd, nothing] =
         std::mismatch(kWindows.begin(), kWindows.end(), file.begin());
      return kWindows.end() == winEnd;
   }


   HANDLE WINAPI CreateFileWHook(
      _In_ LPCWSTR lpFileName,
      _In_ DWORD dwDesiredAccess,
      _In_ DWORD dwShareMode,
      _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes,
      _In_ DWORD dwCreationDisposition,
      _In_ DWORD dwFlagsAndAttributes,
      _In_opt_ HANDLE hTemplateFile)
   {
      static constexpr auto kPpfExt = L".ppf";

      static constexpr auto kTarget =
         L"Castlevania - Symphony of the Night (USA) (Track 1).bin";
      
      const auto hFile = g_origCreateFileW(
            lpFileName,
            dwDesiredAccess,
            dwShareMode,
            lpSecurityAttributes,
            dwCreationDisposition,
            dwFlagsAndAttributes,
            hTemplateFile);

      // We are not interested in failed opens.
      if (INVALID_HANDLE_VALUE == hFile) {
         return hFile;
      }

      if (!HookStatus::ShouldExecute()) {
         return hFile;
      }

      LastErrorRestorer lastErr;
      TDD_PPF_DISABLE_FURTHER_HOOKS();

      fs::path target;
      try {
         target = fs::canonical(lpFileName);
      }
      catch (const std::exception& e) {
         TDD_LOG_DEBUG() << "Target [" << lpFileName
            << "] can't be canonicalized: " << e.what();
         return hFile;
      }

      TDD_LOG_DEBUG() << "Opened: " << lpFileName;
      if (IsWindowsFile(target)) {
         return hFile;
      }

      target.replace_extension(kPpfExt);
      if (!fs::exists(target)) {
         return hFile;
      }

      TDD_LOG_INFO() << "Target: " << lpFileName;
      TDD_LOG_INFO() << "Patch: " << target.wstring();

      if (g_targetFile != INVALID_HANDLE_VALUE) {
         TDD_LOG_WARN() << "Already opened before. Replacing with new handle.";
      }

      g_targetFile = hFile;
      g_readOpsLog = std::make_unique<ReadOpsLog>(hFile, lpFileName);

      const auto patch = tk::rompatch::ppf::Parse(target);
      if (!patch.has_value()) {
         TDD_LOG_WARN() << "Unable to parse patch";
         return hFile;
      }

      g_patch = patch->Flatten();

      return hFile;
   }

   BOOL WINAPI ReadFileHook(
         _In_ HANDLE hFile,
         LPVOID lpBuffer,
         _In_ DWORD nNumberOfBytesToRead,
         _Out_opt_ LPDWORD lpNumberOfBytesRead,
         _Inout_opt_ LPOVERLAPPED lpOverlapped)
   {

      const bool skipHook = !HookStatus::ShouldExecute()
         || hFile == NULL
         || hFile == INVALID_HANDLE_VALUE
         || hFile != g_targetFile
         || g_patch.empty();

      if (skipHook) {
         return g_origReadFile(
            hFile,
            lpBuffer,
            nNumberOfBytesToRead,
            lpNumberOfBytesRead,
            lpOverlapped);
      }

      TDD_PPF_DISABLE_FURTHER_HOOKS();

      TDD_DASSERT(g_readOpsLog.get() != nullptr);
      g_readOpsLog->Log(nNumberOfBytesToRead);

      if (nullptr != lpOverlapped) {
         TDD_LOG_WARN() << "Overlapped IO used to read target";
      }
      const auto targetAddr = base::fs::File::GetFilePointer(hFile);
      // 2. ReadFile
      DWORD bytesRead = 0;

      if (nullptr == lpNumberOfBytesRead) {
         lpNumberOfBytesRead = &bytesRead;
      }

      const auto success = g_origReadFile(
         hFile,
         lpBuffer,
         nNumberOfBytesToRead,
         lpNumberOfBytesRead,
         lpOverlapped);

      LastErrorRestorer lastErr;

      // 3. Apply patch
      if (targetAddr.has_value()) {
         g_patch.Patch(
            targetAddr.value(),
            *lpNumberOfBytesRead,
            static_cast<char*>(lpBuffer));
      }
      else {
         TDD_LOG_WARN() << "Unable to get file pointer location for read";
      }

      return success;
   }

   BOOL ReadFileExHook(
         _In_ HANDLE hFile,
         LPVOID lpBuffer,
         _In_ DWORD nNumberOfBytesToRead,
         _Inout_ LPOVERLAPPED lpOverlapped,
         _In_ LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
   {
      const bool skipHook = !HookStatus::ShouldExecute()
         || hFile == NULL
         || hFile == INVALID_HANDLE_VALUE
         || hFile != g_targetFile;

      if (skipHook) {
         return g_origReadFileEx(
            hFile,
            lpBuffer,
            nNumberOfBytesToRead,
            lpOverlapped,
            lpCompletionRoutine);
      }

      TDD_LOG_WARN() << "ReadFileEx is used to read target file!";

      TDD_DASSERT(g_readOpsLog.get() != nullptr);
      g_readOpsLog->Log(nNumberOfBytesToRead);

      // 2. ReadFileEx. We need to replace the original OVERLAPPED structure.
      const auto success = g_origReadFileEx(
         hFile,
         lpBuffer,
         nNumberOfBytesToRead,
         lpOverlapped,
         lpCompletionRoutine);

      // 3. Post read logging

      return success;
   }

   BOOL WINAPI CloseHandleHook(HANDLE hObject)
   {
      const auto success = g_origCloseHandle(hObject);
      if (!success) {
         return success;
      }
      if (g_targetFile != hObject || hObject == INVALID_HANDLE_VALUE) {
         return success;
      }

      LastErrorRestorer lastErr;
      TDD_PPF_DISABLE_FURTHER_HOOKS();
      TDD_LOG_INFO() << "Target file closed";
      g_targetFile = INVALID_HANDLE_VALUE;

      return success;
   }

   void InstallHooks() noexcept
   {
      DetourRestoreAfterWith();
      DetourTransactionBegin();
      DetourUpdateThread(GetCurrentThread());

      DetourAttach(
         reinterpret_cast<PVOID*>(&g_origCreateFileW),
         CreateFileWHook);

      DetourAttach(
         reinterpret_cast<PVOID*>(&g_origReadFile),
         ReadFileHook);

      DetourAttach(
         reinterpret_cast<PVOID*>(&g_origReadFileEx),
         ReadFileExHook);

      DetourAttach(
         reinterpret_cast<PVOID*>(&g_origCloseHandle),
         CloseHandleHook);

      DetourTransactionCommit();
   }

   void RemoveHooks() noexcept
   {
      DetourTransactionBegin();
      DetourUpdateThread(GetCurrentThread());
      DetourDetach(
         reinterpret_cast<PVOID*>(&g_origCreateFileW),
         CreateFileWHook);
      DetourTransactionCommit();
   }
}

void InitLog()
{
   const auto logDir = base::fs::PathService::DllLogDirectory();
   if (!logDir.has_value()) {
      base::logging::InitDllLog();
   }
   else {
      auto sharedLog = logDir.value() / TDD_EMU_LAUNCHER_EXE_W;
      sharedLog.replace_extension(L".log");
      base::logging::InitDllLog(sharedLog);
   }
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
       tdd::app::ppfinjector::InitLog();
       tdd::app::ppfinjector::InstallHooks();
       break;
    case DLL_PROCESS_DETACH:
       tdd::app::ppfinjector::RemoveHooks();
       break;
    }

    return TRUE;
}