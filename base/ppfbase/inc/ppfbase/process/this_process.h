#pragma once

#include <filesystem>

namespace tdd::base::process::ThisProcess {
   const std::filesystem::path& ImagePath();
   const std::filesystem::path& Name();
}