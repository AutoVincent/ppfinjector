#include <ppfbase/process/this_module.h>

#include <ppfbase/diagnostics/assert.h>
#include <ppfbase/stdext/string.h>

#include <Windows.h>

namespace tdd::base::process::ThisModule {

namespace {
   namespace fs = std::filesystem;

   fs::path ThisModulePath()
   {
      static const DWORD kFlags =
         GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS
         | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT;

      HMODULE thisDll = NULL;

      auto success = GetModuleHandleExW(
         kFlags,
         reinterpret_cast<LPCWSTR>(&ThisModulePath),
         &thisDll);

      if (!success) {
         TDD_DASSERT(false && "Unable to get module handle");
         return L"";
      }

      std::wstring dllPath(MAX_PATH, L'\0');
      success = GetModuleFileNameW(
         thisDll,
         dllPath.data(),
         static_cast<DWORD>(dllPath.size()));

      if (!success) {
         TDD_DASSERT(false && "Unable to get module path");
         return L"";
      }

      stdext::StripTrailingNulls(dllPath);
      return dllPath;
   }
}

const fs::path& ImagePath()
{
   static const auto kPath = ThisModulePath();
   return kPath;
}

const fs::path& Name()
{
   static const auto kName = ImagePath().filename();
   return kName;
}

}
