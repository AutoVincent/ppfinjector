#include <ppftk/rom_patch/patch_descriptor.h>

#include "ppf/schema.h"

#include <ppftk/rom_patch/flat_patch.h>

#include <ppfbase/logging/logging.h>
#include <ppfbase/stdext/iostream.h>

#include <algorithm>

namespace tdd::tk::rompatch {

namespace {
   [[nodiscard]] bool ValidateTarget(
      std::ifstream& target,
      const size_t fileSize,
      const PatchItem& validationData)
   {
      if (validationData.data.empty()) {
         return true;
      }

      if (fileSize < validationData.address + validationData.data.size()) {
         TDD_LOG_ERROR() << "Target image smaller than required";
         return false;
      }

      target.seekg(validationData.address, std::ios::beg);
      if (!target.good()) {
         TDD_LOG_ERROR() << "Unable to seek to validataion location";
         return false;
      }

      DataBuffer targetData(validationData.data.size());
      stdext::Read(target, targetData);
      if (!target.good()) {
         TDD_LOG_ERROR() << "Unable to read target for validation data";
         return false;
      }

      if (targetData != validationData.data) {
         TDD_LOG_ERROR() << "Validation failed";
         return false;
      }

      return true;
   }
}

void PatchDescriptor::Compact()
{
   const auto& patch = std::as_const(m_fullPatch);

   if (patch.empty()) {
      TDD_LOG_DEBUG() << "Nothing to compact";
      return;
   }

   FullPatch compacted;
   PatchItem merged{};

   TDD_DCHECK(
      std::is_sorted(patch.begin(), patch.end()),
      "Patches are not sorted");

   for (const auto& entry : patch) {
      if (entry.data.empty()) {
         continue;
      }

      if (merged.address + merged.data.size() == entry.address) {
         merged.data.insert(
            merged.data.end(),
            entry.data.begin(),
            entry.data.end());
         continue;
      }

      if (!merged.data.empty()) {
         compacted.push_back(merged);
      }
      merged = entry;
   }

   if (!merged.data.empty()) {
      compacted.push_back(merged);
   }

   m_fullPatch = std::move(compacted);
}

bool PatchDescriptor::Compact(
   std::ifstream& targetImage,
   std::optional<size_t> gapSizeToFillOverride)
{
   const auto& patch = std::as_const(m_fullPatch);
   if (patch.empty()) {
      TDD_LOG_DEBUG() << "Nothing to compact";
      return true;
   }

   targetImage.seekg(0, std::ios::end);
   const size_t fileSize = targetImage.tellg();

   if (!ValidateTarget(targetImage, fileSize, GetValidationData())) {
      return false;
   }

   const auto gapSizeToFill = gapSizeToFillOverride.value_or(
      details::ppf::schema::kPatchEntryHeaderSize);

   PatchDescriptor::FullPatch compacted;
   PatchItem merged{};

   for (const auto& entry : patch) {
      if (entry.data.empty()) {
         continue;
      }

      const auto currentEndAddress = merged.address + merged.data.size();
      const auto gap = entry.address - currentEndAddress;

      // If the gap is less than equal to the entry header size, we can save
      // space by pulling in the data from the target image.
      if (gap <= gapSizeToFill) {
         if (gap > 0) {
            if (currentEndAddress + gap + entry.data.size() > fileSize) {
               TDD_LOG_ERROR() << "Patch does not apply to the target image";
               return false;
            }

            targetImage.seekg(currentEndAddress, std::ios::beg);
            if (!targetImage.good()) {
               TDD_LOG_ERROR() << "Unable to seek to filler location";
               return false;
            }

            DataBuffer filler(gap);
            stdext::Read(targetImage, filler);
            if (!targetImage.good()) {
               TDD_LOG_ERROR() << "Unable to read target image for filler data";
               return false;
            }

            merged.data.insert(
               merged.data.end(),
               filler.begin(),
               filler.end());
         }

         merged.data.insert(
            merged.data.end(),
            entry.data.begin(),
            entry.data.end());
         continue;
      }

      if (!merged.data.empty()) {
         compacted.push_back(merged);
      }
      merged = entry;
   }

   if (!merged.data.empty()) {
      compacted.push_back(merged);
   }

   m_fullPatch = std::move(compacted);

   return true;
}

bool PatchDescriptor::AddPatchData(
   const size_t address,
   const DataBuffer& data)
{
   return AddPatchData(address, DataBuffer(data));
}

bool PatchDescriptor::AddPatchData(
   const size_t address,
   DataBuffer&& data)
{
   PatchItem newPatch{
      .address = address,
      .data = std::move(data)};

   const auto it = std::lower_bound(
      m_fullPatch.begin(),
      m_fullPatch.end(),
      newPatch);

   if (it != m_fullPatch.end() && it->address == address) {
      TDD_LOG_ERROR() << "Patch data for address [" << address
         << "] already exists.";
      return false;
   }

   m_fullPatch.insert(it, std::move(newPatch));
   return true;
}

void PatchDescriptor::SetFullPatch(FullPatch&& patch) noexcept
{
   m_fullPatch = std::move(patch);
}

void PatchDescriptor::AddValidationData(
   const size_t address,
   const DataBuffer& data)
{
   AddValidationData(address, DataBuffer(data));
}

void PatchDescriptor::AddValidationData(
   const size_t address,
   DataBuffer&& data) noexcept
{
   m_validationData.address = address;
   m_validationData.data = std::move(data);
}

void PatchDescriptor::AddFileId(std::string_view info)
{
   AddFileId(std::string(info));
}

void PatchDescriptor::AddFileId(std::string&& info)
{
   m_fileId = std::move(info);
}

void PatchDescriptor::AddDescription(std::string_view description)
{
   AddDescription(std::string(description));
}

void PatchDescriptor::AddDescription(std::string&& description)
{
   m_description = std::move(description);
}

const PatchDescriptor::FullPatch&
PatchDescriptor::GetFullPatch() const noexcept
{
   return m_fullPatch;
}

const PatchDescriptor::ValidationData&
PatchDescriptor::GetValidationData() const noexcept
{
   return m_validationData;
}

const std::string& PatchDescriptor::GetFileId() const noexcept
{
   return m_fileId;
}

const std::string& PatchDescriptor::GetDescription() const noexcept
{
   return m_description;
}

}