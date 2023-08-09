#pragma once

#include <ppftk/rom_patch/patch_descriptor.h>

#include <ppfbase/preprocessor_utils.h>

#include <filesystem>

namespace tdd::tk::rompatch::ppf {

   class [[nodiscard]] Ppf3{
   public:
      Ppf3(PatchDescriptor&& patch);
      TDD_DEFAULT_ALL_SPECIAL_MEMBERS(Ppf3);

      [[nodiscard]] const PatchDescriptor::ValidationData&
         GetValidationData() const noexcept;
      [[nodiscard]] const PatchDescriptor::FullPatch&
         GetFullPatch() const noexcept;

      void Compact();
      [[nodiscard]] bool Compact(std::ifstream& targetImage);

      [[nodiscard]] std::error_code ToFile(
         const std::filesystem::path& patchPath);

   private:
      PatchDescriptor m_patch;
   };

}