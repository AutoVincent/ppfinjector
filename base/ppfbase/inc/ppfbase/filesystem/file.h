#pragma once

#include <ppfbase/stdext/type_traits.h>

#include <optional>
#include <system_error>

#include <Windows.h>

namespace tdd::base::fs::File {

   namespace  details {
      struct FilePointerTag{};
   }

   using FilePointer = stdext::strong_type<int64_t, details::FilePointerTag>;


[[nodiscard]] std::optional<FilePointer> GetFilePointer(
   const HANDLE hFile) noexcept;

[[nodiscard]] std::error_code Seek(
   const HANDLE hFile,
   const FilePointer pos) noexcept;

}