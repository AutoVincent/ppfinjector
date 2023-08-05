#pragma once

#include <filesystem>

namespace tdd::base::process::ThisModule {
   const std::filesystem::path& ImagePath();
   const std::filesystem::path& Name();
}