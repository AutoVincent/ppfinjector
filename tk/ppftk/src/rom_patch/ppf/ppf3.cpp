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

}

std::error_code WritePpf3Patch(
   const std::filesystem::path& ppf3Path,
   const PatchDescriptor& patch)
{
   std::ofstream ppf3;
   ppf3.exceptions(0);
   ppf3.open(ppf3Path, std::ofstream::binary | std::ofstream::trunc);
   if (!ppf3.good()) {
      TDD_LOG_ERROR() << "Unable to create file: [" << ppf3Path.wstring()
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

   const auto& desc = patch.GetDescription();
   if (desc.size() > sizeof(header.description)) {
      TDD_LOG_WARN() << "Description is longer than allowed length. Truncated.";
   }

   memcpy_s(
      header.description,
      sizeof(header.description),
      desc.c_str(),
      desc.size());

   const auto& validationData = patch.GetValidationData();
   if (validationData.address == v3::kPrimoDvdValidationAddress) {
      header.imageType = v3::TargetImageType::PrimoDvd;
   }
   else {
      header.imageType = v3::TargetImageType::Any;
   }
   
   header.validateImage = !validationData.data.empty();
   header.hasUndoData = false;

   stdext::Write(ppf3, header);

   if (header.validateImage) {
      stdext::Write(ppf3, validationData.data);
   }

   for (const auto& entry : patch.GetFullPatch()) {
      WritePatchEntry(ppf3, entry);
   }

   WriteFileId(ppf3, patch.GetFileId());
   ppf3.close();

   return {};
}

}