#include <ppftk/rom_patch/simple_patcher.h>

#include "apply_patches.h"

#include <algorithm>

namespace tdd::tk::rompatch {

SimplePatcher::SimplePatcher(PatchDescriptor&& fullPatch) noexcept
   : m_patches(std::move(fullPatch).TakeFullPatch())
   , m_addrRange(0, 0)
{
   if (m_patches.empty()) {
      return;
   }

   m_addrRange.first = m_patches.front().address;
   m_addrRange.second = m_patches.back().address + m_patches.back().data.size();
}

std::optional<IPatcher::AdditionalReads> SimplePatcher::Patch(
   const uint64_t addr,
   std::span<uint8_t> buffer)
{
   const auto bufferEnd = addr + buffer.size();
   if (!Overlaps(addr, bufferEnd)) {
      return std::nullopt;
   }

   // std::lower_bound finds the first item that fails the predicate. The
   // predicate used to do the search must match the predicate used to sort
   // the container. We want the entry such that 'p.address <= addr'. For this
   // we need search backwards with 'operator>'.
   const auto rit = std::lower_bound(
      m_patches.rbegin(),
      m_patches.rend(),
      addr,
      [](const auto& p, const auto addr) {
         return p.address > addr;
      }
   );

   // https://en.cppreference.com/w/cpp/iterator/reverse_iterator/base
   auto it = rit.base();
   if (it != m_patches.begin()) {
      --it;
   }

   ApplyPatches(addr, buffer, it, m_patches.end());

   return std::nullopt;
}

bool SimplePatcher::Overlaps(
   const uint64_t tgtStart,
   const uint64_t tgtEnd) const noexcept
{
   // [tgtStart, tgtEnd)
   if (tgtEnd <= m_addrRange.first) {
      return false;
   }

   if (m_addrRange.second <= tgtStart) {
      return false;
   }

   return true;
}

}