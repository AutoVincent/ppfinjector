#pragma once

#include <ppftk/rom_patch/patch_item.h>

#include <ppfbase/preprocessor_utils.h>

#include <fstream>
#include <optional>
#include <string>
#include <vector>

namespace tdd::tk::rompatch {

   class [[nodiscard]] PatchDescriptor
   {
   public:
      using ValidationData = PatchItem;
      using FullPatch = std::vector<PatchItem>;

      TDD_DEFAULT_ALL_SPECIAL_MEMBERS(PatchDescriptor);

      void Compact();
      [[nodiscard]] bool Compact(
         std::ifstream& targetImage,
         std::optional<size_t> gapSizeToFillOverride = std::nullopt);

      [[nodiscard]] bool AddPatchData(
         const size_t address,
         const DataBuffer& data);

      [[nodiscard]] bool AddPatchData(
         const size_t address,
         DataBuffer&& data);

      void SetFullPatch(FullPatch&& patch) noexcept;

      void AddValidationData(
         const size_t address,
         const DataBuffer& data);

      void AddValidationData(
         const size_t address,
         DataBuffer&& data) noexcept;

      void AddFileId(std::string_view id);
      void AddFileId(std::string&& id);

      void AddDescription(std::string_view description);
      void AddDescription(std::string&& description);

      [[nodiscard]] const ValidationData& GetValidationData() const noexcept;
      [[nodiscard]] const FullPatch& GetFullPatch() const noexcept;
      [[nodiscard]] const std::string& GetFileId() const noexcept;
      [[nodiscard]] const std::string& GetDescription() const noexcept;

      [[nodiscard]] FullPatch&& TakeFullPatch() && noexcept;

   private:
      FullPatch m_fullPatch;
      ValidationData m_validationData;
      std::string m_description;
      std::string m_fileId;
   };

}