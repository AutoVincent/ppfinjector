#include <ppfbase/process/this_process.h>

#include <ppfbase/stdext/string.h>

#include <Windows.h>

namespace tdd::ppf::base::process::ThisProcess {

namespace {
   namespace fs = std::filesystem;

   fs::path GetImagePath()
   {
      std::wstring path(_MAX_PATH, L'\0');

      if (0 == GetModuleFileNameW(NULL, path.data(), MAX_PATH)) {
         // TODO: build an error hierachy.
         throw std::runtime_error("Can't get process path");
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

}