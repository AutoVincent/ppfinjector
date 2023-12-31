#include "hook_status.h"

#include <ppftk/config/app.h>
#include <ppftk/config/patch.h>
#include <ppftk/rom_patch/patch_file_exts.h>
#include <ppftk/rom_patch/patchers.h>
#include <ppftk/rom_patch/ppf/parser.h>

#include <ppfbase/branding.h>
#include <ppfbase/logging/logging.h>
#include <ppfbase/filesystem/file.h>
#include <ppfbase/filesystem/path_service.h>
#include <ppfbase/process/this_module.h>
#include <ppfbase/stdext/scope_exit.h>
#include <ppfbase/stdext/string.h>
#include <ppfbase/stdext/system_error.h>

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
   namespace AppConfig = tk::config::App;

   auto g_origCreateFileW = CreateFileW;
   auto g_origReadFile = ReadFile;
   auto g_origReadFileEx = ReadFileEx;
   auto g_origCloseHandle = CloseHandle;

   std::unique_ptr<tk::rompatch::IPatcher> g_patch;
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

   [[nodiscard]] bool IsTargetable(const fs::path& file)
   {
      static const fs::path kWindows(L"C:\\Windows\\");

      const auto targetExt = stdext::WideToUtf8(file.extension().wstring());

      if (AppConfig::TargetExts().count(targetExt) == 0) {
         return false;
      }

      const auto [winEnd, nothing] =
         std::mismatch(kWindows.begin(), kWindows.end(), file.begin());
      return kWindows.end() != winEnd;
   }

   void LogReadOp(const HANDLE hFile, DWORD readSize)
   {
      if (!TDD_LOG_IS_ON(Verbose_2)) {
         return;
      }

      std::stringstream log;
      const auto filePtr = base::fs::File::GetFilePointer(hFile);
      if (filePtr.has_value()) {
         log << filePtr->get();
      }
      else {
         log << "XXX";
      }

      log << ":" << readSize;
      TDD_VLOG2() << log.str();
   }

   void PatchExtraRead(
      const HANDLE hFile,
      const tk::rompatch::IPatcher::AdditionalReads& extraReads)
   {
      std::vector<uint8_t> extraBlock(extraReads.blockSize, 0);
      for (const auto addr : extraReads.addrs) {
         const auto err =
            base::fs::File::Seek(hFile, base::fs::File::FilePointer(addr));
         TDD_CHECK(!err, "Unable to seek for extra read");

         DWORD bytesRead = 0;
         g_origReadFile(
            hFile,
            extraBlock.data(),
            static_cast<DWORD>(extraBlock.size()),
            &bytesRead,
            nullptr);

         TDD_CHECK(
            extraBlock.size() == bytesRead,
            "Unable to read requested amount");

         const auto check = g_patch->Patch(addr, extraBlock);
         TDD_CHECK(!check.has_value(), "Unexpected extra read requests");
      }
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

      std::error_code ec;
      const auto target = fs::canonical(lpFileName, ec);
      if (ec) {
         TDD_LOG_DEBUG() << "Target [" << lpFileName
                         << "] can't be canonicalized: " << ec.message();
         return hFile;
      }

      TDD_LOG_DEBUG() << "Opened: " << lpFileName;
      if (!IsTargetable(target)) {
         return hFile;
      }

      tk::config::Patch patchConfig(target);

      if (patchConfig.PatchFile().empty()) {
         return hFile;
      }

      TDD_LOG_INFO() << "Target: " << lpFileName;
      TDD_LOG_INFO() << "Patch: " << patchConfig.PatchFile().wstring();

      if (g_targetFile != INVALID_HANDLE_VALUE) {
         TDD_LOG_WARN() << "Already opened before. Replacing with new handle.";
      }

      g_targetFile = hFile;

      auto patch = tk::rompatch::ppf::Parse(patchConfig.PatchFile());
      if (!patch.has_value()) {
         TDD_LOG_WARN() << "Unable to parse patch";
         return hFile;
      }

      if (patchConfig.CalculateEdc()) {
         TDD_LOG_INFO() << "EDC calculation required.";
         g_patch = std::make_unique<tk::rompatch::cd::Patcher>(
            std::move(patch).value());
      }
      else {
         g_patch = std::make_unique<tk::rompatch::SimplePatcher>(
            std::move(patch).value());
      }

      return hFile;
   }

   BOOL WINAPI ReadFileHook(
      _In_ HANDLE hFile,
      LPVOID lpBuffer,
      _In_ DWORD nNumberOfBytesToRead,
      _Out_opt_ LPDWORD lpNumberOfBytesRead,
      _Inout_opt_ LPOVERLAPPED lpOverlapped)
   {
      const bool skipHook = !HookStatus::ShouldExecute() || hFile == NULL ||
         hFile == INVALID_HANDLE_VALUE || hFile != g_targetFile ||
         nullptr == g_patch;

      if (skipHook) {
         return g_origReadFile(
            hFile,
            lpBuffer,
            nNumberOfBytesToRead,
            lpNumberOfBytesRead,
            lpOverlapped);
      }

      TDD_PPF_DISABLE_FURTHER_HOOKS();

      LogReadOp(hFile, nNumberOfBytesToRead);

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
      if (!targetAddr.has_value()) {
         TDD_LOG_WARN() << "Unable to get file pointer location for read";
         return success;
      }

      const auto extraReads = g_patch->Patch(
         targetAddr.value().get(),
         std::span(static_cast<uint8_t*>(lpBuffer), *lpNumberOfBytesRead));

      if (!extraReads.has_value()) {
         return success;
      }

      TDD_ON_SCOPE_EXIT(
         const LARGE_INTEGER location{
            .QuadPart = targetAddr.value().get() + *lpNumberOfBytesRead};
         if (!::SetFilePointerEx(hFile, location, nullptr, FILE_BEGIN)) {
            const auto err = stdext::make_last_error();
            TDD_LOG_ERROR()
               << "Unable to restore file pointer: " << err.message();
         });

      PatchExtraRead(hFile, extraReads.value());
      const auto check = g_patch->Patch(
         targetAddr.value().get(),
         std::span(static_cast<uint8_t*>(lpBuffer), *lpNumberOfBytesRead));
      TDD_CHECK(!check.has_value(), "Unexpected extra read requests");
      return success;
   }

   BOOL ReadFileExHook(
      _In_ HANDLE hFile,
      LPVOID lpBuffer,
      _In_ DWORD nNumberOfBytesToRead,
      _Inout_ LPOVERLAPPED lpOverlapped,
      _In_ LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
   {
      const bool skipHook = !HookStatus::ShouldExecute() || hFile == NULL ||
         hFile == INVALID_HANDLE_VALUE || hFile != g_targetFile;

      if (skipHook) {
         return g_origReadFileEx(
            hFile,
            lpBuffer,
            nNumberOfBytesToRead,
            lpOverlapped,
            lpCompletionRoutine);
      }

      TDD_LOG_WARN() << "ReadFileEx is used to read target file!";

      LogReadOp(hFile, nNumberOfBytesToRead);

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

      const bool skipHook = !HookStatus::ShouldExecute() ||
         g_targetFile != hObject || hObject == INVALID_HANDLE_VALUE;

      if (skipHook) {
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

      DetourAttach(reinterpret_cast<PVOID*>(&g_origReadFile), ReadFileHook);

      DetourAttach(reinterpret_cast<PVOID*>(&g_origReadFileEx), ReadFileExHook);

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

   base::logging::SetMinLogLevel(AppConfig::LogLevel());
}

}

BOOL APIENTRY DllMain(HMODULE, DWORD reason, LPVOID)
{
   if (DetourIsHelperProcess()) {
      return TRUE;
   }

   switch (reason) {
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