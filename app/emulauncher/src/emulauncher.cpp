#include "cmds/cli.h"

#include <ppfbase/branding.h>
#include <ppfbase/logging/logging.h>
#include <ppfbase/process/this_process.h>
#include <ppfbase/stdext/string.h>

#include <iostream>

#include <Windows.h>
#include <commdlg.h>

#include <detours/detours.h>

namespace tdd::app::emulauncher {
   namespace fs = std::filesystem;

   void LaunchBizHawk(const std::wstring& emuHawk)
   {
      auto& launcher = base::process::ThisProcess::ImagePath();
      const auto injectorPath =
         (launcher.parent_path() / TDD_PPF_INJECTOR_DLL_W).string();

      std::wstring hawkPath = emuHawk;
      std::wstring hawkCurrentDir = fs::path(hawkPath).parent_path().wstring();

      STARTUPINFOW si = {0};
      si.cb = sizeof(si);

      const auto success = DetourCreateProcessWithDllExW(
         nullptr,
         hawkPath.data(),
         nullptr,
         nullptr,
         FALSE,
         NORMAL_PRIORITY_CLASS,
         nullptr,
         hawkCurrentDir.data(),
         &si,
         nullptr,
         injectorPath.c_str(),
         nullptr);

      if (!success) {
         const auto err = GetLastError();
         std::cout << "Unable to launch emulator: " << err << std::endl;
         return;
      }

      std::cout << "Emulator launched with injector" << std::endl;
   }
}

int main(int argc, char** argv)
{
   tdd::base::logging::InitProcessLog(
      tdd::base::logging::SharingMode::MultiProcess);

   if (argc > 1) {
      return tdd::app::emulauncher::cmd::HandleCmdLine(argc, argv);
   }

   TDD_LOG_INFO() << "EmuLauncher started";
   std::wstring bizhawk(MAX_PATH, L'\0');

   OPENFILENAMEW ofn{0};
   ofn.lStructSize = sizeof(ofn);
   ofn.hwndOwner = NULL;
   ofn.lpstrFile = bizhawk.data();
   ofn.nMaxFile = MAX_PATH;
   ofn.lpstrFilter = L"Emulator\0*.exe\0";
   ofn.nFilterIndex = 1;
   ofn.lpstrFileTitle = nullptr;
   ofn.nMaxFileTitle = 0;
   ofn.lpstrInitialDir = nullptr;
   ofn.Flags = OFN_PATHMUSTEXIST | OFN_PATHMUSTEXIST;

   const auto res = GetOpenFileNameW(&ofn);
   if (!res) {
      std::wcout << "Failed: " << CommDlgExtendedError() << std::endl;
      return 1;
   }

   tdd::stdext::StripTrailingNulls(bizhawk);

   std::wcout << "Found: " << bizhawk << std::endl;

   tdd::app::emulauncher::LaunchBizHawk(bizhawk);

   return 0;
}