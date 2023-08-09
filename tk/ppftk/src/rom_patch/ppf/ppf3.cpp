#include <ppftk/rom_patch/ppf/ppf3.h>

#include "schema.h"

#include <ppfbase/logging/logging.h>
#include <ppfbase/stdext/iostream.h>
#include <ppfbase/stdext/system_error.h>

#include <fstream>

#include <Windows.h>

namespace tdd::tk::rompatch::ppf {

namespace {
   namespace v3 = details::ppf::schema;

   void WritePatchEntry(std::ofstream& os, const PatchItem& item)
   {
      static constexpr size_t kMaxPayload = std::numeric_limits<uint8_t>::max();

      std::vector<char> entryData(v3::kPatchEntryHeaderSize + kMaxPayload);
      
      auto pEntry = reinterpret_cast<v3::PatchEntry*>(entryData.data());
      pEntry->address = item.address;

      for (auto it = item.data.begin(); it != item.data.end();) {
         const auto remaining = static_cast<size_t>(
            std::distance(it, item.data.end()));

         pEntry->length = static_cast<uint8_t>(
            std::min(remaining, kMaxPayload));

         memcpy_s(pEntry->data, pEntry->length, &(*it), pEntry->length);

         os.write(entryData.data(), v3::kPatchEntryHeaderSize + pEntry->length);

         it+= pEntry->length;
         pEntry->address += pEntry->length;
      }
   }

   void WriteFileId(std::ofstream& os, std::string fileId)
   {
      if (fileId.empty()) {
         return;
      }

      if (fileId.size() > v3::fileid::kDataMaxLength) {
         // write a null to the file at the last character.
         fileId.resize(v3::fileid::kDataMaxLength - 1);
      }

      auto fileIdLength = static_cast<uint16_t>(fileId.size());

      os.write(v3::fileid::kBegin, v3::fileid::kBeginLength);
      stdext::Write(os, fileId);
      if (fileId.back() != '\0') {
         os.write("\0", 1);
         ++fileIdLength;
      }

      os.write(v3::fileid::kEnd, v3::fileid::kEndLength);
      stdext::Write(os, fileIdLength);
   }

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

Ppf3::Ppf3(PatchDescriptor&& patch)
   : m_patch(std::move(patch))
{}

const PatchDescriptor::ValidationData& Ppf3::GetValidationData() const noexcept
{
   return m_patch.GetValidationData();
}

const PatchDescriptor::FullPatch& Ppf3::GetFullPatch() const noexcept
{
   return m_patch.GetFullPatch();
}

void Ppf3::Compact()
{
   const auto& patch = m_patch.GetFullPatch();

   if (patch.empty()) {
      TDD_LOG_DEBUG() << "Nothing to compact";
      return;
   }

   PatchDescriptor::FullPatch compacted;
   PatchItem merged{};

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
         compacted.insert(merged);
      }
      merged = entry;
   }

   if (!merged.data.empty()) {
      compacted.insert(merged);
   }

   m_patch.SetFullPatch(std::move(compacted));
}

bool Ppf3::Compact(std::ifstream& targetImage)
{
   const auto& patch = m_patch.GetFullPatch();
   if (patch.empty()) {
      TDD_LOG_DEBUG() << "Nothing to compact";
      return true;
   }

   targetImage.seekg(0, std::ios::end);
   const size_t fileSize = targetImage.tellg();

   if (!ValidateTarget(targetImage, fileSize, m_patch.GetValidationData())) {
      return false;
   }

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
      if (gap <= v3::kPatchEntryHeaderSize) {
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
         compacted.insert(merged);
      }
      merged = entry;
   }

   if (!merged.data.empty()) {
      compacted.insert(merged);
   }

   m_patch.SetFullPatch(std::move(compacted));

   return true;
}

std::error_code Ppf3::ToFile(const std::filesystem::path& patchPath)
{
   
   std::ofstream patch;
   patch.exceptions(0);
   patch.open(patchPath, std::ofstream::binary | std::ofstream::trunc);
   if (!patch.good()) {
      TDD_LOG_ERROR() << "Unable to create file: [" << patchPath.wstring()
         << "]";
      return stdext::make_win32_ec(static_cast<uint32_t>(E_FAIL));
   }

   v3::Header header{0};
   memcpy_s(
      header.magic,
      sizeof(header.magic),
      v3::kMagic3,
      std::char_traits<char>::length(v3::kMagic3));

   header.encoding = v3::Encoding::PPF3;

   const auto& desc = m_patch.GetDescription();
   if (desc.size() > sizeof(header.description)) {
      TDD_LOG_WARN() << "Description is longer than allowed length. Truncated.";
   }

   memcpy_s(
      header.description,
      sizeof(header.description),
      desc.c_str(),
      desc.size());

   const auto& validationData = m_patch.GetValidationData();
   if (validationData.address == v3::kPrimoDvdValidationAddress) {
      header.imageType = v3::TargetImageType::PrimoDvd;
   }
   else {
      header.imageType = v3::TargetImageType::Any;
   }
   
   header.validateImage = !validationData.data.empty();
   header.hasUndoData = false;

   stdext::Write(patch, header);

   if (header.validateImage) {
      stdext::Write(patch, validationData.data);
   }

   for (const auto& entry : m_patch.GetFullPatch()) {
      WritePatchEntry(patch, entry);
   }

   WriteFileId(patch, m_patch.GetFileId());
   patch.close();

   return {};
}

}