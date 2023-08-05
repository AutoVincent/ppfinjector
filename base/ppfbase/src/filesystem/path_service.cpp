#include <ppfbase/filesystem/path_service.h>

#include <ppfbase/diagnostics/assert.h>
#include <ppfbase/process/this_module.h>
#include <ppfbase/process/this_process.h>
#include <ppfbase/stdext/string.h>

#include <Windows.h>


namespace tdd::base::fs::PathService {

namespace {
   namespace fs = std::filesystem;

   static constexpr auto kLogDir = L"logs";

}

std::error_code EnsureDirectoryExist(const std::filesystem::path& dir)
{
   std::error_code ec;
   if (!dir.is_absolute()) {
      ec.assign(ERROR_INVALID_PARAMETER, std::system_category());
      return ec;
   }

   if (!fs::exists(dir)) {
      fs::create_directories(dir, ec);
      return ec;
   }

   if (!fs::is_directory(dir)) {
      ec.assign(ERROR_ALREADY_EXISTS, std::system_category());
   }

   return ec;
}

std::optional<fs::path> ProcessLogDirectory()
{
   auto logDir = process::ThisProcess::ImagePath().parent_path() / kLogDir;

   const auto ec = EnsureDirectoryExist(logDir);
   if (ec) {
      return std::nullopt;
   }
   else {
      return logDir;
   }
}

std::optional<fs::path> DllLogDirectory()
{
   auto dllPath = process::ThisModule::ImagePath();
   if (dllPath.empty()) {
      return std::nullopt;
   }

   auto logDir = dllPath.parent_path() / kLogDir;

   const auto ec = EnsureDirectoryExist(logDir);
   if (ec) {
      return std::nullopt;
   }

   return logDir;
}

}