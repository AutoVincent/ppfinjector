#pragma once

#include <span>

namespace tdd::tk::rompatch {
   class [[nodiscard]] IPatcher
   {
   public:
      virtual ~IPatcher() = default;
      virtual void Patch(
         const uint64_t addr,
         std::span<char> buffer) = 0;
   };
}