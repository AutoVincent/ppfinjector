#include "v3.h"

#include "schema.h"

#include <ppftk/rom_patch/ppf/ppf3.h>

#include <ppfbase/logging/logging.h>
#include <ppfbase/stdext/iostream.h>
#include <ppfbase/stdext/poor_mans_expected.h>
#include <ppfbase/stdext/string.h>

#include <array>
#include <istream>

#include <Windows.h>

namespace tdd::tk::rompatch::details::ppf::V3 {

namespace {
   stdext::pm_expected<std::string> GetFileId(std::istream& ppf)
   {
      namespace fileid = schema::fileid;

      ppf.seekg(0, std::istream::end);
      const size_t fileSize = ppf.tellg();

      // Try to find the end tag first to see if we have any file id data.
      ppf.seekg(fileid::kEndPos, std::istream::end);

      std::string endMagic(fileid::kEndLength, '\0');
      stdext::Read(ppf, endMagic);

      if (endMagic != fileid::kEnd) {
         return stdext::make_win32_ec(ERROR_NOT_FOUND);
      }

      int16_t idDataLength = 0;
      stdext::Read(ppf, idDataLength);
      
      const auto idLength = idDataLength + fileid::kTotalPadding;

      if (idDataLength < 0
       || idDataLength > fileid::kDataMaxLength
       || idLength > fileSize) {
         TDD_LOG_WARN() << "Invalid FileId length: " << idDataLength;
         return stdext::make_win32_ec(ERROR_INVALID_PARAMETER);
      }

      int idBeginOffset = -static_cast<int>(idLength);

      ppf.seekg(idBeginOffset, std::istream::end);

      std::string beginMagic(fileid::kBeginLength, '\0');
      stdext::Read(ppf, beginMagic);

      if (beginMagic != fileid::kBegin) {
         TDD_LOG_WARN() << "[" << fileid::kBegin << "] not found";
         return stdext::make_win32_ec(ERROR_INVALID_PARAMETER);
      }

      std::string fileId(idDataLength, '\0');
      stdext::Read(ppf, fileId);
      return fileId;
   }

   [[nodiscard]] size_t DataLength(
      std::istream& ppf,
      const size_t fileIdLength)
   {
      ppf.seekg(0, std::istream::end);
      return static_cast<size_t>(ppf.tellg()) - fileIdLength;
   }

   bool ParsePatchData(
      std::istream& ppf,
      const bool hasUndo,
      size_t unreadLength,
      PatchDescriptor& patch)
   {
      static constexpr size_t kBlockSize = 64 * 1024;
      const auto payloadMultiplier = 1 + (hasUndo ? 1 : 0);
      
      DataBuffer patchBlock;
      do {
         const auto staleSize = patchBlock.size();
         const auto bytesToRead = std::min(kBlockSize, unreadLength);
         const auto newSize = staleSize + bytesToRead;
         patchBlock.resize(newSize);

         ppf.read(
            reinterpret_cast<char*>(patchBlock.data() + staleSize),
            bytesToRead);

         unreadLength -= bytesToRead;

         auto it = patchBlock.cbegin();
         while(true) {
            const auto unreadBlock = static_cast<size_t>(
               std::distance(it, patchBlock.cend()));
            if (unreadBlock < schema::kPatchEntryHeaderSize) {
               // End of current block. Get ready to read the next block
               break;
            }
            const auto entry = reinterpret_cast<const schema::PatchEntry*>(&(*it));

            const auto entrySize =
               schema::kPatchEntryHeaderSize + entry->length * payloadMultiplier;

            if (unreadBlock < entrySize) {
               break;
            }

            std::advance(it, entrySize);

            const auto added = patch.AddPatchData(
               entry->address,
               DataBuffer(entry->data, entry->data + entry->length));

            if (!added) {
               TDD_LOG_WARN() << "Duplicate patch data for address: "
                  << entry->address;
               return false;
            }
         }

         patchBlock.erase(patchBlock.begin(), it);

      } while(unreadLength> 0);

      const bool success = patchBlock.empty();

      if (!success) {
         TDD_LOG_WARN() << "Parsing failed. Malformed patch entry remains.";
      }
      return success;
   }
}

std::optional<PatchDescriptor> Parse(std::istream& ppf)
{
   auto fileIdExpected = GetFileId(ppf);

   PatchDescriptor patch;
   if (fileIdExpected.has_value()) {
      patch.AddFileId(std::move(fileIdExpected).value());
   }
   else {
      const auto ec = fileIdExpected.handle_error(
         [](const std::error_code ec) {
            if (ec.value() != ERROR_NOT_FOUND) {
               return ec;
            }
            return std::error_code{};
         });

      if (ec) {
         return std::nullopt;
      }
   }

   const auto dataLength = DataLength(ppf, patch.GetFileId().length());

   if (dataLength < sizeof(schema::Header)) {
      TDD_LOG_WARN() << "File too small: " << dataLength;
      return std::nullopt;
   }

   ppf.seekg(0);
   schema::Header hdr{0};
   stdext::Read(ppf, hdr);

   std::string description(hdr.description, sizeof(hdr.description));
   stdext::StripTrailingNulls(description);
   patch.AddDescription(std::move(description));

   size_t remainingData = dataLength - sizeof(hdr);

   if (hdr.validateImage) {
      if (remainingData < schema::kValidataionDataLength) {
         TDD_LOG_WARN() << "Not enough validation data: " << remainingData;
         return std::nullopt;
      }

      const auto validationAddress =
         hdr.imageType == schema::TargetImageType::PrimoDvd
            ? schema::kPrimoDvdValidationAddress
            : schema::kAnyImageValidationAddress;

      DataBuffer data(schema::kValidataionDataLength);
      stdext::Read(ppf, data);
      patch.AddValidationData(validationAddress, std::move(data));
      remainingData -= schema::kValidataionDataLength;
   }

   if (!ParsePatchData(ppf, hdr.hasUndoData, remainingData, patch)) {
      return std::nullopt;
   }

   patch.Compact();
   return patch;
}

}