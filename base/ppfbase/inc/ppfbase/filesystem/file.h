#pragma once

#include <optional>

#include <Windows.h>

namespace tdd::base::fs::File {

   [[nodiscard]] std::optional<int64_t> GetFilePointer(const HANDLE hFile);
}