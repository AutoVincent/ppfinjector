#include <ppftk/rom_patch/cd/patcher.h>

#include <ppftk/rom_patch/patch_descriptor.h>
#include <ppftk/rom_patch/cd/sector_range.h>
#include <ppftk/rom_patch/cd/spec.h>

#include <ppfbase/logging/logging.h>

namespace tdd::tk::rompatch::cd {

namespace {
   std::vector<SectorPatch> Convert(PatchDescriptor::FullPatch&& patches)
   {
      std::vector<SectorPatch> sectorPatches(1);
      for (auto&& p : patches) {
         const auto res = sectorPatches.back().AddPatch(std::move(p));
         if (SectorPatch::AddPatchResult::Added == res) {
            continue;
         }

         sectorPatches.emplace_back(std::move(p));
      }
      return sectorPatches;
   }

   // Look for a patch with std::lower_bound. Finding the first one with a
   // sector number that greater than or equal to the target SectorView.
   [[nodiscard]] bool LowerBound(
      const SectorPatch& patch,
      const SectorView& sector)
   {
      return patch.SectorNumber() < sector.SectorNumber();
   }

   // Look for a patch with std::upper_bound. Finding the first one with a
   // sector number that's greater than the target SectorView.
   [[nodiscard]] bool UpperBound(
      const SectorView& sector,
      const SectorPatch& patch) noexcept
   {
      return sector.SectorNumber() < patch.SectorNumber();
   }
}

Patcher::Patcher(PatchDescriptor&& fullPatch)
   : m_patches(Convert(std::move(fullPatch).TakeFullPatch()))
{}

std::optional<IPatcher::AdditionalReads> Patcher::Patch(
   const uint64_t addr,
   std::span<uint8_t> buffer)
{
   return DoPatch(ByteAddress(addr), buffer);
}

std::optional<IPatcher::AdditionalReads> Patcher::DoPatch(
   const ByteAddress addr,
   std::span<uint8_t> buffer)
{
   {
      const auto additionalReads = RequireAdditionalReads(addr, buffer);
      if (!additionalReads.addrs.empty()) {
         return additionalReads;
      }
   }

   // Apply patches.

   SectorRange range(addr, buffer);
   const auto firstSector = range.begin();
   const auto lastSector = range.end() - SectorDiff(1);

   auto firstPatch = std::lower_bound(
      m_patches.begin(),
      m_patches.end(),
      *firstSector,
      LowerBound);

   auto lastPatch = std::upper_bound(
      firstPatch,
      m_patches.end(),
      *lastSector,
      UpperBound);

   auto target = range.begin();
   for (auto patch = firstPatch; patch < lastPatch; ++patch) {
      target += patch->SectorNumber() - target->SectorNumber();
      patch->Patch(*target);
   }

   return std::nullopt;
}

IPatcher::AdditionalReads Patcher::RequireAdditionalReads(
   const ByteAddress targetAddr,
   std::span<uint8_t> buffer) const
{
   const SectorRange range(targetAddr, buffer);

   AdditionalReads read{};
   read.blockSize = spec::kSectorSize;

   if (range.empty()) {
      TDD_DCHECK(false, "Patching empty range");
      return read;
   }

   if (const auto& first = range.begin(); RequireFullSector(*first)) {
      read.addrs.push_back(first->SectorAddress().get());
   }

   if (range.size() == 1) {
      return read;
   }

   if (const auto& last = range.end() - SectorDiff(1); RequireFullSector(*last)) {
      read.addrs.push_back(last->SectorAddress().get());
   }

   return read;
}

bool Patcher::RequireFullSector(const SectorView& sector) const noexcept
{
   if (sector.IsComplete()) {
      return false;
   }

   const auto patch = std::lower_bound(
      m_patches.begin(),
      m_patches.end(),
      sector,
      LowerBound);

   if (patch == m_patches.end()) {
      return false;
   }

   if (patch->SectorNumber() == sector.SectorNumber()) {
      return !patch->HasUpdatedEdc();
   }
   return false;
}

}