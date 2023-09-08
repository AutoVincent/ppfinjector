#pragma once

#include <ppfbase/filesystem/file.h>
#include <ppfbase/logging/logging.h>
#include <ppfbase/stdext/system_error.h>

namespace tdd::base::fs::File {

namespace {
   LARGE_INTEGER ToLargeInt(const FilePointer pos) noexcept
   {
      return LARGE_INTEGER{.QuadPart = pos.get()};
   }
}

std::optional<FilePointer> GetFilePointer(const HANDLE hFile) noexcept
{
   static constexpr LARGE_INTEGER kDontMoveFilePointer{0};

   LARGE_INTEGER location{0};
   const auto success =
      SetFilePointerEx(hFile, kDontMoveFilePointer, &location, FILE_CURRENT);

   if (success) {
      return FilePointer(location.QuadPart);
   }

   const auto err = GetLastError();
   TDD_LOG_WARN() << "Unable to get current file pointer location: "
                  << std::error_code(err, std::system_category());
   return std::nullopt;
}

std::error_code Seek(const HANDLE hFile, const FilePointer pos) noexcept
{
   if (::SetFilePointerEx(hFile, ToLargeInt(pos), nullptr, FILE_BEGIN)) {
      return {};
   }

   return stdext::make_last_error();
}

}