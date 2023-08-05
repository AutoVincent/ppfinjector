#include <ppfbase/process/this_process.h>

#include <ppfbase/stdext/string.h>

#include <Windows.h>

namespace tdd::base::process::ThisProcess {

namespace {
   namespace fs = std::filesystem;

   fs::path GetImagePath()
   {
      std::wstring path(_MAX_PATH, L'\0');

      if (0 == GetModuleFileNameW(NULL, path.data(), MAX_PATH)) {
         return L"";
      }

      stdext::StripTrailingNulls(path);
      return path;
   }
}

const fs::path& ImagePath()
{
   static const auto kPath = GetImagePath();
   return kPath;
}

const fs::path& Name()
{
   static const auto kName = ImagePath().filename();
   return kName;
}

}