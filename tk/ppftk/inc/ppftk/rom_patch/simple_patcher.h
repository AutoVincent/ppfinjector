#pragma once

#include <ppftk/rom_patch/ipatcher.h>
#include <ppftk/rom_patch/patch_descriptor.h>

#include <ppfbase/preprocessor_utils.h>

namespace tdd::tk::rompatch {

   class [[nodiscard]] SimplePatcher : public IPatcher
   {
   public:
      SimplePatcher(PatchDescriptor&& fullPatch);

      TDD_DEFAULT_ALL_SPECIAL_MEMBERS(SimplePatcher);

      void Patch(
         const uint64_t addr,
         std::span<uint8_t> buffer) override;

   private:
      [[nodiscard]] bool Overlaps(
         const uint64_t tgtStart,
         const uint64_t tgtEnd) const noexcept;

      PatchDescriptor m_patches;
      std::pair<uint64_t, uint64_t> m_addrRange;

   };
}