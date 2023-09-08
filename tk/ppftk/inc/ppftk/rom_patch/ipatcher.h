#pragma once

#include <span>
#include <optional>
#include <vector>

namespace tdd::tk::rompatch {

class [[nodiscard]] IPatcher
{
public:
   // Additional blocks of data to feed the patcher with before the whole
   // range can be succesfully patched. For file format with fixed data blocks
   // such as CDs, the range being patched could start or end in the middle of
   // a block. It is expected that the Patcher would not request more than 2
   // additional full blocks of data.
   struct [[nodiscard]] AdditionalReads
   {
      std::vector<uint64_t> addrs;
      uint64_t blockSize;
   };

   virtual ~IPatcher() = default;
   [[nodiscard]] virtual std::optional<AdditionalReads> Patch(
      const uint64_t addr,
      std::span<uint8_t> buffer) = 0;
};

}