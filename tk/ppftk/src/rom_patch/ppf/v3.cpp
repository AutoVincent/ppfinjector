#include "v3.h"

#include "schema.h"

#include <ppfbase/logging/logging.h>

#include <array>
#include <istream>


namespace tdd::tk::rompatch::details::ppf::V3 {

namespace {

#pragma pack(push, 1)
   struct [[nodiscard]] Header
   {
      char magic[5];
      schema::Encoding encoding;
      char description[50];
      schema::TargetImageType imageType;
      bool validateImage;
      bool hasUndoData;
      bool dummy;
   };
   static_assert(sizeof(Header) == 60);

   struct [[nodiscard]] PatchEntry
   {
      uint64_t address;
      uint8_t length;
      char data[1];
   };

   static constexpr auto kPatchEntryHeaderSize = sizeof(uint64_t) + sizeof(uint8_t);

#pragma pack(pop)

   // Present after the Header if 'validateImage' is true.
   static constexpr size_t kValidataionDataLength = 1024;
   static constexpr size_t kAnyImageValidationAddress = 0x9320;
   static constexpr size_t kPrimoDvdValidationAddress = 0x80A0;


   template <
      typename T,
      std::enable_if_t<std::is_trivial_v<T>, void*> = nullptr>
   void Read(std::istream& ppf, T& value)
   {
      ppf.read(reinterpret_cast<char*>(&value), sizeof(T));
   }

   template <typename ContainerT,
      std::enable_if_t<!std::is_trivial_v<ContainerT>, void*> = nullptr>
   void Read(std::istream& ppf, ContainerT& value)
   {
      ppf.read(reinterpret_cast<char*>(value.data()), value.size());
   }

   std::optional<size_t> GetFileIdLength(std::istream& ppf)
   {
      // If the patch has a FILE_ID.DIZ, its length is the last two bytes of the
      // file. 'FILE_ID.DIZ' magic string immediately precedes these two bytes.
      static constexpr int kFileIdLengthLength = 2;
      static constexpr auto kFileIdBegin = "@BEGIN_FILE_ID.DIZ";
      static constexpr auto kFileIdBeginLength =
         std::char_traits<char>::length(kFileIdBegin);
      static constexpr auto kFileIdEnd = "@END_FILE_ID.DIZ";
      static constexpr auto kFileIdEndLength =
         std::char_traits<char>::length(kFileIdEnd);
      static constexpr size_t kFileIdMaxLength = 3072;

      static constexpr int kFileIdEndPos = 
         -static_cast<int>(kFileIdLengthLength + kFileIdEndLength);

      static constexpr size_t kTotalPadding =
         kFileIdLengthLength + kFileIdBeginLength + kFileIdEndLength;

      ppf.seekg(kFileIdEndPos, std::istream::end);

      std::string endMagic(kFileIdEndLength, '\0');
      Read(ppf, endMagic);

      if (endMagic != kFileIdEnd) {
         return 0;
      }

      int16_t idDataLength = 0;
      Read(ppf, idDataLength);
      if (idDataLength < 0 || idDataLength > kFileIdMaxLength) {
         TDD_LOG_WARN() << "Invalid FileId length: " << idDataLength;
         return std::nullopt;
      }

      const auto idLength = idDataLength + kTotalPadding;
      int idBeginOffset = -static_cast<int>(idLength);

      ppf.seekg(idBeginOffset, std::istream::end);

      std::string beginMagic(kFileIdBeginLength, '\0');
      Read(ppf, beginMagic);

      if (beginMagic != kFileIdBegin) {
         TDD_LOG_WARN() << "[" << kFileIdBegin << "] not found";
         return std::nullopt;
      }
      return idLength;
   }

   std::optional<size_t> DataLength(std::istream& ppf)
   {
      const auto fileIdLength = GetFileIdLength(ppf);
      if (!fileIdLength.has_value()) {
         return std::nullopt;
      }

      ppf.seekg(0, std::istream::end);
      return static_cast<size_t>(ppf.tellg()) - fileIdLength.value();
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
            if (unreadBlock < kPatchEntryHeaderSize) {
               // End of current block. Get ready to read the next block
               break;
            }
            const auto entry = reinterpret_cast<const PatchEntry*>(&(*it));

            const auto entrySize =
               kPatchEntryHeaderSize + entry->length * payloadMultiplier;

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
   const auto dataLengthOpt = DataLength(ppf);
   if (!dataLengthOpt.has_value()) {
      return std::nullopt;
   }

   if (dataLengthOpt.value() < sizeof(Header)) {
      TDD_LOG_WARN() << "File too small: " << dataLengthOpt.value();
      return std::nullopt;
   }

   PatchDescriptor patch;

   ppf.seekg(0);
   Header hdr{0};
   Read(ppf, hdr);

   size_t remainingData = dataLengthOpt.value() - sizeof(hdr);

   if (hdr.validateImage) {
      if (remainingData < kValidataionDataLength) {
         TDD_LOG_WARN() << "Not enough validation data: " << remainingData;
         return std::nullopt;
      }

      const auto validationAddress =
         hdr.imageType == schema::TargetImageType::PrimoDvd
            ? kPrimoDvdValidationAddress
            : kAnyImageValidationAddress;

      DataBuffer data(kValidataionDataLength);
      Read(ppf, data);
      patch.AddValidationData(validationAddress, std::move(data));
      remainingData -= kValidataionDataLength;
   }

   if (!ParsePatchData(ppf, hdr.hasUndoData, remainingData, patch)) {
      return std::nullopt;
   }

   return patch;
}

}