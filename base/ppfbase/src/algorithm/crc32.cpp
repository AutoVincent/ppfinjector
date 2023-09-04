#include <ppfbase/algorithm/crc32.h>

namespace tdd::base::algorithm::crc32 {

uint32_t ComputeCrc(
   std::span<const uint8_t> block,
   const LookupTable& table,
   const InitialValue init) noexcept
{
   const uint32_t initValue = init == InitialValue::Zero ? 0 : 0xFFFF'FFFF;

   uint32_t crc = initValue;

   for (const auto c : block) {
      crc = (crc >> 8) ^ table[(crc ^ c) & 0xFF];
   }

   // Hacker's Delight stars with '-1' and ends with '~crc'. This negation is
   // the same as 'crc ^ -1' if we started with '-1'. XORing 0 is a nop.
   return crc ^ initValue;
}

}