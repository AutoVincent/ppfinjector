#pragma once
#include <string>

namespace tdd::tk::rompatch::details::ppf::schema {
   inline constexpr auto kMagic1 = "PPF10";
   inline constexpr auto kMagic2 = "PPF20";
   inline constexpr auto kMagic3 = "PPF30";

   enum class [[nodiscard]] Encoding : char
   {
      PPF1 = 0,
      PPF2 = 1,
      PPF3 = 2
   };

   enum class [[nodiscard]] TargetImageType : char
   {
      Any = 0,
      PrimoDvd = 1
   };

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

   // Present after the Header if 'validateImage' is true.
   inline constexpr size_t kValidataionDataLength = 1024;
   // Address in the target file where validation data is located.
   inline constexpr size_t kAnyImageValidationAddress = 0x9320;
   inline constexpr size_t kPrimoDvdValidationAddress = 0x80A0;

   struct [[nodiscard]] PatchEntry
   {
      uint64_t address;
      uint8_t length;
      char data[1];
   };

   inline constexpr auto kPatchEntryHeaderSize = sizeof(uint64_t) + sizeof(uint8_t);

#pragma pack(pop)

   namespace fileid {
      // If the patch has a FILE_ID.DIZ, its length is the last two bytes of the
      // file. '@END_FILE_ID.DIZ' magic string immediately precedes these two bytes.
      inline constexpr int kLengthSize = 2;

      inline constexpr auto kBegin = "@BEGIN_FILE_ID.DIZ";
      inline constexpr auto kBeginLength = std::char_traits<char>::length(kBegin);

      inline constexpr auto kEnd = "@END_FILE_ID.DIZ";
      inline constexpr auto kEndLength = std::char_traits<char>::length(kEnd);

      // The data could be longer. But PPF3.txt says it's 3072.
      inline constexpr size_t kDataMaxLength = 3072;

      inline constexpr int kEndPos = -static_cast<int>(kLengthSize + kEndLength);

      inline constexpr size_t kTotalPadding = kLengthSize + kBeginLength + kEndLength;
   }

}