#pragma once

#include <ppftk/rom_patch/patch_item.h>

#include <span>

namespace tdd::tk::rompatch {

   template <typename Iter>
   void ApplyPatches(
      const uint64_t addr,
      std::span<uint8_t> target,
      const Iter patchBegin,
      const Iter patchEnd)
   {
      const auto targetEnd = addr + target.size();

      for (auto it = patchBegin; it != patchEnd && it->address < targetEnd; ++it) {
         const auto patchEndAddr = it->address + it->data.size();
         if (patchEndAddr <= addr) {
            continue;
         }

         if (it->address <= addr) {
            // target range starts in the middle of a patch
            const size_t skip = addr - it->address;
            const auto copySize = std::min(
               target.size_bytes(),
               it->data.size() - skip);
            memcpy_s(
               target.data(),
               copySize,
               &it->data[skip],
               copySize);
         }
         else {
            const auto offset = it->address - addr;
            const auto availableBufferSize = target.size_bytes() - offset;
            const auto copySize = std::min(availableBufferSize, it->data.size());
            memcpy_s(
               &target[offset],
               copySize,
               it->data.data(),
               copySize);
         }
      }
   }

   template <typename Container>
   void ApplyPatches(
      const uint64_t addr,
      std::span<uint8_t> target,
      const Container& patches)
   {
      ApplyPatches(addr, target, patches.begin(), patches.end());
   }
}