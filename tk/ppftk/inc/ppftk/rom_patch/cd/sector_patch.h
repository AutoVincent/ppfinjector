#pragma once

#include <ppftk/rom_patch/patch_item.h>
#include <ppftk/rom_patch/cd/sector_view.h>
#include <ppftk/rom_patch/cd/spec.h>

#include <ppfbase/preprocessor_utils.h>

#include <optional>
#include <span>

namespace tdd::tk::rompatch::cd {

   class [[nodiscard]] SectorPatch
   {
   public:
      SectorPatch();
      SectorPatch(PatchItem&& patch);
      SectorPatch(const PatchItem& patch);

      ~SectorPatch() = default;

      TDD_DEFAULT_COPY_MOVE(SectorPatch);

      [[nodiscard]] bool operator<(const SectorPatch& other) const noexcept;

      enum class [[nodiscard]] AddPatchResult
      {
         Added,
         NextSector
      };

      AddPatchResult AddPatch(PatchItem&& patch);
      AddPatchResult AddPatch(const PatchItem& patch);

      // Also calculate checksum data if required.
      void Patch(SectorView& sector) const;

      [[nodiscard]] SectorNumber SectorNumber() const noexcept;

      [[nodiscard]] bool HasUpdatedEdc() const noexcept;

      // For when we need to patch a sector that the buffer in ReadFile doesn't
      // cover completely. The hook needs to perform extra read to get either
      // the start or the end of the sector.
      void CalculateChecksum(SectorView& originalSector);
   private:
      void PatchEdc(SectorView& sector) const noexcept;
      void CalculateEdc(SectorView& sv) const noexcept;

      void CalculateMode1Edc(spec::Sector* sector) const noexcept;
      void CalculateMode2Edc(
         spec::Sector* sector,
         const cd::SectorNumber sectorNumber) const noexcept;
      void CalculateXaForm1Edc(spec::Sector* sector) const noexcept;
      void CalculateXaForm2Edc(spec::Sector* sector) const noexcept;
      void ZeroXaForm2Edc(spec::Sector* sector) const noexcept;

      // filePtr / kSectorSize
      cd::SectorNumber m_sectorNumber;
      std::vector<PatchItem> m_patches;
      mutable std::optional<spec::Edc> m_edc;
      mutable SectorOffset m_edcIdx;
      // May not be needed if edc alone can fool the emulator.
      // mutable DataBuffer m_ecc;
   };

}