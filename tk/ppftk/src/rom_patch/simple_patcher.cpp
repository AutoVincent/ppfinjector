#include <ppftk/rom_patch/simple_patcher.h>

#include <algorithm>

namespace tdd::tk::rompatch {

SimplePatcher::SimplePatcher(PatchDescriptor&& fullPatch)
   : m_patches(std::move(fullPatch))
   , m_addrRange(0, 0)
{
   const auto& patches = m_patches.GetFullPatch();
   if (patches.empty()) {
      return;
   }

   m_addrRange.first = patches.front().address;
   m_addrRange.second = patches.back().address + patches.back().data.size();
}

void SimplePatcher::Patch(const uint64_t addr, std::span<char> buffer)
{
   const auto bufferEnd = addr + buffer.size();
   if (!Overlaps(addr, bufferEnd)) {
      return;
   }

   const auto& patches = m_patches.GetFullPatch();

   // std::lower_bound finds the first item that fails the predicate. The
   // predicate used to do the search must match the predicate used to sort
   // the container. We want the entry such that 'p.address <= addr'. For this
   // we need search backwards with 'operator>'.
   const auto rit = std::lower_bound(
      patches.rbegin(),
      patches.rend(),
      addr,
      [](const auto& p, const auto addr) {
         return p.address > addr;
      }
   );

   // https://en.cppreference.com/w/cpp/iterator/reverse_iterator/base
   auto it = rit.base();
   if (it != patches.begin()) {
      --it;
   }

   for(; it != patches.end() && it->address < bufferEnd; ++it) {
      const auto patchEnd = it->address + it->data.size();
      if (patchEnd <= addr) {
         // The first patch we found preceed the target range completely. We
         // need to skip at most 1 patch.
         continue;
      }

      if (it->address <= addr) {
         // target range starts in the middle of a patch
         const size_t skip = addr - it->address;
         const auto copySize = std::min(
            buffer.size_bytes(),
            it->data.size() - skip);
         memcpy_s(
            buffer.data(),
            copySize,
            &it->data[skip],
            copySize);
      }
      else {
         const auto offset = it->address - addr;
         const auto availableBufferSize = buffer.size_bytes() - offset;
         const auto copySize = std::min(availableBufferSize, it->data.size());
         memcpy_s(
            &buffer[offset],
            copySize,
            it->data.data(),
            copySize);
      }
   }
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