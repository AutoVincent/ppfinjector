#include "launch_emulator.h"

#include <ppftk/config/app.h>

#include <ppfbase/branding.h>
#include <ppfbase/logging/logging.h>

#include <ppfbase/process/this_process.h>
#include <ppfbase/stdext/string.h>

#include <iostream>

#include <Windows.h>
#include <commdlg.h>

#include <detours/detours.h>

namespace tdd::app::emulauncher {

namespace {
   namespace fs = std::filesystem;
   namespace AppConfig = tk::config::App;

   fs::path PickEmulator()
   {
      std::wstring emulator(MAX_PATH, L'\0');

      OPENFILENAMEW ofn{0};
      ofn.lStructSize = sizeof(ofn);
      ofn.hwndOwner = NULL;
      ofn.lpstrFile = emulator.data();
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
         return {};
      }

      tdd::stdext::StripTrailingNulls(emulator);

      std::wcout << "You picked: " << emulator << std::endl;
      return emulator;
   }

   void Launch(const fs::path& emulator)
   {
      auto& launcher = base::process::ThisProcess::ImagePath();
      const auto injectorPath =
         (launcher.parent_path() / TDD_PPF_INJECTOR_DLL_W).string();

      std::wstring emuCurrentDir = emulator.parent_path().wstring();
      std::wstring emuPath = emulator.wstring();

      STARTUPINFOW si = {0};
      si.cb = sizeof(si);

      const auto success = DetourCreateProcessWithDllExW(
         nullptr,
         emuPath.data(),
         nullptr,
         nullptr,
         FALSE,
         NORMAL_PRIORITY_CLASS,
         nullptr,
         emuCurrentDir.data(),
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

void LaunchEmulator()
{
   fs::path emulator = AppConfig::Emulator();
   if (emulator.empty()) {
      emulator = PickEmulator();

      if (emulator.empty()) {
         std::cout << "No emulator selected" << std::endl;
         return;
      }

      AppConfig::Emulator(emulator);
   }

   Launch(emulator);
}

}
