#pragma once

#include <ppfbase/preprocessor_utils.h>

#include <filesystem>

namespace tdd::tk::config {

   class [[nodiscard]] Patch
   {
   public:
      Patch(const std::filesystem::path& target);
      TDD_DEFAULT_ALL_SPECIAL_MEMBERS(Patch);

      [[nodiscard]] const std::filesystem::path& PatchFile() const noexcept;
      [[nodiscard]] bool CalculateEdc() const noexcept;

   private:
      std::filesystem::path m_patch;
      bool m_calculateEdc;
   };

}