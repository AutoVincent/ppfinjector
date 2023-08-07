#pragma once

#include <ppftk/rom_patch/patch_item.h>

#include <ppfbase/preprocessor_utils.h>

#include <set>
#include <vector>

namespace tdd::tk::rompatch {

   class FlatPatch;

   class [[nodiscard]] PatchDescriptor
   {
   public:
      using ValidationData = PatchItem;
      using FullPatch = std::set<PatchItem>;

      TDD_DEFAULT_ALL_SPECIAL_MEMBERS(PatchDescriptor);

      FlatPatch Flatten() const;

      void AddValidationData(
         const size_t address,
         const DataBuffer& data);

      void AddValidationData(
         const size_t address,
         DataBuffer&& data) noexcept;

      [[nodiscard]] bool AddPatchData(
         const size_t address,
         const DataBuffer& data);

      [[nodiscard]] bool AddPatchData(
         const size_t address,
         DataBuffer&& data);

      [[nodiscard]] const ValidationData& GetValidationData() const noexcept;
      [[nodiscard]] const FullPatch& GetFullPatch() const noexcept;

   private:
      ValidationData m_validationData;
      FullPatch m_fullPatch;
   };

}