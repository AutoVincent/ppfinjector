#include <ppfbase/algorithm/crc32.h>

#include <doctest/doctest.h>

#include <string>

namespace tdd::base::algorithm::crc32 {

namespace {

   // copied from zlib/crc32.c
   LookupTable ZlibCrcTable()
   {
      LookupTable table{0};

      unsigned int i, j;
      uint32_t p;

      for (i = 0; i < 256; i++) {
         p = i;
         for (j = 0; j < 8; j++)
            p = p & 1 ? (p >> 1) ^ polynomials::kDefault : p >> 1;
         table[i] = p;
      }
      return table;
   }

   static constexpr auto kDefaultTable =
      CompileLookupTable(polynomials::kDefault);

   static constexpr auto kCdTable = CompileLookupTable(polynomials::kCdRom);

   // Test value provided by https://simplycalc.com/crc32-text.php
   static const std::string kTestData("123456789abcdefg");
   static constexpr uint32_t kExpectedDefaultCrc = 0xA2CA'AFFF;
   static constexpr uint32_t kExpectedCdRomCrc = 0x9AAD'A303;
}

TEST_CASE("Crc32: Generate correct lookup table")
{
   const auto zlibTable = ZlibCrcTable();
   CHECK(zlibTable == kDefaultTable);
}

TEST_CASE("Crc32: Produces expected CRC-32 checksum")
{
   std::span block(
      reinterpret_cast<const uint8_t*>(kTestData.data()),
      kTestData.size());

   SUBCASE("Default polynomial")
   {
      const auto crc = ComputeCrc(block, kDefaultTable, InitialValue::MinusOne);
      CHECK(kExpectedDefaultCrc == crc);
   }

   SUBCASE("CD-ROM polynomial")
   {
      const auto crc = ComputeCrc(block, kCdTable, InitialValue::MinusOne);
      CHECK(kExpectedCdRomCrc == crc);
   }
}

}