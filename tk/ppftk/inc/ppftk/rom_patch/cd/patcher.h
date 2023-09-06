#pragma once

#include <ppftk/rom_patch/ipatcher.h>

#include <ppftk/rom_patch/cd/sector_patch.h>

#include <ppfbase/preprocessor_utils.h>

namespace tdd::tk::rompatch {
class PatchDescriptor;
}

namespace tdd::tk::rompatch::cd {
   class SectorView;

   class [[nodiscard]] Patcher : public IPatcher
   {
   public:
      Patcher(PatchDescriptor&& fullPatch);

      TDD_DEFAULT_ALL_SPECIAL_MEMBERS(Patcher);

      [[nodiscard]] std::optional<AdditionalReads> Patch(
         const uint64_t addr,
         std::span<uint8_t> buffer) override;

   private:
      [[nodiscard]] std::optional<AdditionalReads> DoPatch(
         const ByteAddress addr,
         std::span<uint8_t> buffer);

      AdditionalReads RequireAdditionalReads(
         const ByteAddress targetAddr,
         std::span<uint8_t> buffer) const;

      [[nodiscard]] bool RequireFullSector(
         const SectorView& sector) const noexcept;

      std::vector<SectorPatch> m_patches;
   };
}