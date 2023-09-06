#include <ppftk/rom_patch/cd/sector_patch.h>

#include "../apply_patches.h"

#include <ppftk/rom_patch/cd/spec.h>

#include <ppfbase/algorithm/crc32.h>
#include <ppfbase/logging/logging.h>

namespace tdd::tk::rompatch::cd {

namespace {
   namespace crc = base::algorithm::crc32;

   static constexpr crc::LookupTable kCrcTable =
      crc::CompileLookupTable(crc::polynomials::kCdRom);

   static constexpr SectorOffset kRequireEdcUpdate(0);
   static constexpr SectorOffset kNoEdcIdx(spec::kSectorSize);

   static constexpr SectorOffset kMode1EdcIdx(
      spec::kSync.size() + spec::kSectorHeaderSize +
      sizeof(spec::Mode1Data::userData));

   static constexpr SectorOffset kXa1EdcIdx(
      spec::kSync.size() + spec::kSectorHeaderSize +
      sizeof(spec::XaSubHeader) * 2 + sizeof(spec::Mode2Xa1Data::data));

   static constexpr SectorOffset kXa2EdcIdx(
      spec::kSectorSize - sizeof(spec::Mode2Xa2Data::edc));

}

SectorPatch::SectorPatch()
   : m_sectorNumber(0)
   , m_patches()
   , m_edc()
   , m_edcIdx(kRequireEdcUpdate)
{}

SectorPatch::SectorPatch(PatchItem&& patch)
   : SectorPatch()
{
   std::ignore = AddPatch(std::move(patch));
}

SectorPatch::SectorPatch(const PatchItem& patch)
   : SectorPatch()
{
   std::ignore = AddPatch(patch);
}

bool SectorPatch::operator<(const SectorPatch& other) const noexcept
{
   return SectorNumber() < other.SectorNumber();
}

SectorPatch::AddPatchResult SectorPatch::AddPatch(PatchItem&& patch)
{
   const auto targetSector = ToSectorNumber(ByteAddress(patch.address));
   if (m_patches.empty()) {
      m_sectorNumber = targetSector;
   }

   TDD_DCHECK(targetSector >= m_sectorNumber, "Patch is for a previous sector");

   if (targetSector > m_sectorNumber) {
      return AddPatchResult::NextSector;
   }

   // Adjust address to be offset from start of the sector
   patch.address %= spec::kSectorSize;

   TDD_DCHECK(
      m_patches.empty() || patch.address > m_patches.back().address,
      "Patch not added in sorted order");

   m_patches.push_back(std::move(patch));
   return AddPatchResult::Added;
}

SectorPatch::AddPatchResult SectorPatch::AddPatch(const PatchItem& patch)
{
   return AddPatch(PatchItem(patch));
}

void SectorPatch::Patch(SectorView& sector) const
{
   ApplyPatches(sector.DataOffset().get(), sector.Data(), m_patches);

   if (HasUpdatedEdc()) {
      PatchEdc(sector);
   }
   else {
      CalculateEdc(sector);
   }
}

SectorNumber SectorPatch::SectorNumber() const noexcept
{
   return m_sectorNumber;
}

bool SectorPatch::HasUpdatedEdc() const noexcept
{
   return m_edcIdx == kNoEdcIdx || m_edc.has_value();
}

void SectorPatch::CalculateChecksum(SectorView& originalSector)
{
   TDD_DCHECK(originalSector.IsComplete(), "Sector data needs to be complete.");
   Patch(originalSector);
}

void SectorPatch::PatchEdc(SectorView& sector) const noexcept
{
   // This sector doesn't need EDC.
   if (m_edcIdx == kNoEdcIdx) {
      return;
   }

   TDD_DCHECK(m_edc.has_value(), "No EDC to patch with");

   for (auto idx = std::max(sector.DataOffset(), m_edcIdx);
        idx.get() < sector.DataOffset().get() + sector.Size();
        ++idx.get()) {
      sector[idx] = m_edc->parts[idx.get() - m_edcIdx.get()];
   }
}

void SectorPatch::CalculateEdc(SectorView& sv) const noexcept
{
   TDD_DCHECK(sv.IsComplete(), "Sector needs to be complete");

   m_edcIdx = kNoEdcIdx;

   auto sector = sv.AsSector();
   switch (sector->header.parts.mode) {
   case spec::kMode0:
      // ECMA-130 14: Mode 0 is full of 0's
      return;
   case spec::kMode1:
      return CalculateMode1Edc(sector);
   case spec::kMode2:
      return CalculateMode2Edc(sector, sv.SectorNumber());
   default:
      TDD_LOG_WARN() << "Invalid [" << sv.SectorNumber() << "]: mode ["
                     << sector->header.parts.mode << "]";
      return;
   }
}

void SectorPatch::CalculateMode1Edc(spec::Sector* sector) const noexcept
{
   // ECMA-130: 14.3
   static constexpr size_t kStartIdx = 0;
   static constexpr size_t kBlockSize = 2064;

   std::span<uint8_t> block(reinterpret_cast<uint8_t*>(sector), 2064);
   sector->mode1.edc.full =
      crc::ComputeCrc(block, kCrcTable, crc::InitialValue::Zero);
   m_edc = sector->mode1.edc;
   m_edcIdx = kMode1EdcIdx;
}

void SectorPatch::CalculateMode2Edc(
   spec::Sector* sector,
   const cd::SectorNumber sectorNumber) const noexcept
{
   if (sector->xa.subheader[0].full != sector->xa.subheader[0].full) {
      // The assumption is that we are dealing with PSX games. PSX CDs are all
      // in XA format. If the sector is a valid Mode 2 non-XA sector, it doesn't
      // have EDC anyway.
      TDD_LOG_WARN() << "[" << sectorNumber << "] is not XA";
      return;
   }

   if (sector->xa.subheader[0].parts.submode.form == spec::kXaForm1) {
      return CalculateXaForm1Edc(sector);
   }

   // 'form' is 1 bit. It has to be Form 2 here.
   TDD_DCHECK(
      sector->xa.subheader[0].parts.submode.form == spec::kXaForm2,
      "Unexpected form value");

   ZeroXaForm2Edc(sector);
}

void SectorPatch::CalculateXaForm1Edc(spec::Sector* sector) const noexcept
{
   // CD-ROM XA: 4.5.2. EDC covers XA subheader and user data.
   static constexpr size_t kBlockSize =
      sizeof(spec::XaSubHeader) * 2 + sizeof(spec::Mode2Xa1Data::data);

   std::span<uint8_t> block(
      reinterpret_cast<uint8_t*>(&sector->xa),
      kBlockSize);

   sector->xa.form1.edc.full =
      crc::ComputeCrc(block, kCrcTable, crc::InitialValue::Zero);
   m_edc = sector->xa.form1.edc;
   m_edcIdx = kXa1EdcIdx;
}

void SectorPatch::CalculateXaForm2Edc(spec::Sector* sector) const noexcept
{
   // Form 2 EDC covers the same range as Form 1, i.e. XA subheader and user
   // data.
   std::span<uint8_t> block(
      reinterpret_cast<uint8_t*>(&sector->xa),
      sizeof(spec::Mode2XaData) - sizeof(spec::Edc));

   sector->xa.form2.edc.full =
      crc::ComputeCrc(block, kCrcTable, crc::InitialValue::Zero);
   m_edc = sector->xa.form2.edc;
   m_edcIdx = kXa2EdcIdx;
}

void SectorPatch::ZeroXaForm2Edc(spec::Sector* sector) const noexcept
{
   // CD-ROM XA: 4.6.2. The 'Reserved' field can either hold a CRC-32 value, or
   // be cleared to 0. Clear to 0 is faster, course.
   sector->xa.form2.edc.full = 0;
   m_edc = sector->xa.form2.edc;
   m_edcIdx = kXa2EdcIdx;
}

}