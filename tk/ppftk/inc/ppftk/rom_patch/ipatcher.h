#pragma once

#include <span>
#include <optional>

namespace tdd::tk::rompatch {

class [[nodiscard]] IPatcher
{
public:
   // Additional complete sectors to feed the patcher with before the whole
   // range can be succesfully patched.
   struct [[nodiscard]] AdditionalReads
   {
      uint64_t firstAddr;
      uint64_t lastAddr;
   };

   virtual ~IPatcher() = default;
   [[nodiscard]] virtual std::optional<AdditionalReads> Patch(
      const uint64_t addr,
      std::span<uint8_t> buffer) = 0;
};

}