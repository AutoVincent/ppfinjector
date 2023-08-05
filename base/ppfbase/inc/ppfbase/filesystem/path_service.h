#pragma once

#include <filesystem>
#include <optional>

namespace tdd::base::fs::PathService {

   [[nodiscard]] std::error_code EnsureDirectoryExist(
      const std::filesystem::path& dir);

   [[nodiscard]] std::optional<std::filesystem::path> ProcessLogDirectory();
   [[nodiscard]] std::optional<std::filesystem::path> DllLogDirectory();

}
