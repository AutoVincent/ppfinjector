#pragma once

#include <array>
#include <span>
#include <limits>

namespace tdd::base::algorithm::crc32 {

// All polynomials are in reversed form.
namespace polynomials {
   // CRC-32/CKSUM. The polynomial used in typical CRC-32 calculation
   inline constexpr uint32_t kDefault = 0xedb8'8320;
   // CD-ROM polynomial. ECMA-130 14.3
   inline constexpr uint32_t kCdRom = 0xd801'8001;

   // Additional common polynomials in normal form and their initial values can
   // be found on https://reveng.sourceforge.io/crc-catalogue/17plus.htm
}

using LookupTable = std::array<uint32_t, std::numeric_limits<uint8_t>::max() + 1>;

// Hacker's Delight 2nd Ed. 14-3, p329. 'polynomial' is in Reversed form.
inline constexpr LookupTable GenerateLookupTable(const uint32_t polynomial)
{
   LookupTable table{0};
   for (uint32_t i = 0; i <= std::numeric_limits<uint8_t>::max(); ++i) {
      uint32_t crc = i;
      for (auto bit = 0; bit < 8; ++bit) {
#pragma warning(suppress: 4146) // negation of unsigned still unsigned
         const auto mask = -(crc & 1);
         crc = (crc >> 1) ^ (polynomial & mask);
      }
      table[i] = crc;
   }
   return table;
}

// Force compile time generation
inline consteval LookupTable CompileLookupTable(const uint32_t polynomial)
{
   return GenerateLookupTable(polynomial);
}

// Some CRC calculations require the starting value to be 0. The default
// expectation is that we start with 0xFFFF'FFFF, which is unsigned -1.
enum class InitialValue
{
   Zero,
   MinusOne
};

[[nodiscard]] uint32_t ComputeCrc(
   std::span<const uint8_t> block,
   const LookupTable& table,
   const InitialValue init) noexcept;

}