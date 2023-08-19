#pragma once

#include <ppfbase/preprocessor_utils.h>
#include <ppfbase/logging/severity.h>

#include <filesystem>
#include <set>
#include <string>

namespace tdd::tk::config::details {

   class [[nodiscard]] AppImpl
   {
   public:
      AppImpl();
      ~AppImpl() = default;
      TDD_DEFAULT_COPY_MOVE(AppImpl);

      const base::logging::Severity LogLevel() const noexcept;

      const std::filesystem::path& Emulator() const noexcept;
      void Emulator(const std::filesystem::path& emulator);
      void ClearEmulator();

      const std::set<std::string>& TargetExts() const noexcept;

   private:
      void Load();

      std::filesystem::path m_emulator;
      std::set<std::string> m_targetExts;
      base::logging::Severity m_logLevel;
   };

}