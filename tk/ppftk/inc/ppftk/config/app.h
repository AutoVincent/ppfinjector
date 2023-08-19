#pragma once

#include <ppfbase/preprocessor_utils.h>
#include <ppfbase/logging/severity.h>

#include <filesystem>
#include <set>
#include <string>

namespace tdd::tk::config::App {

   const base::logging::Severity LogLevel() noexcept;

   const std::filesystem::path& Emulator() noexcept;
   void Emulator(const std::filesystem::path& emulator);
   void ClearEmulator();

   const std::set<std::string>& TargetExts() noexcept;


}