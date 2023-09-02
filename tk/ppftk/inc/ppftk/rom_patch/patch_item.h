#pragma once

#include <set>
#include <vector>

namespace tdd::tk::rompatch {

   using DataBuffer = std::vector<uint8_t>;

   struct [[nodiscard]] PatchItem
   {
      uint64_t address;
      DataBuffer data;

      [[nodiscard]] bool operator<(const PatchItem& other) const noexcept
      {
         return address < other.address;
      }
   };

}