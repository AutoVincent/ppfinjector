#pragma once

#include <ppfbase/filesystem/file.h>
#include <ppfbase/logging/logging.h>

namespace tdd::base::fs::File {

std::optional<int64_t> GetFilePointer(const HANDLE hFile)
{
   static constexpr LARGE_INTEGER kDontMoveFilePointer{ 0 };

   LARGE_INTEGER location{ 0 };
   const auto success = SetFilePointerEx(
      hFile,
      kDontMoveFilePointer,
      &location,
      FILE_CURRENT);

   if (success) {
      return location.QuadPart;
   }

   const auto err = GetLastError();
   TDD_LOG_WARN() << "Unable to get current file pointer location: "
      << std::error_code(err, std::system_category());
   return std::nullopt;
}
}