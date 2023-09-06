#include <ppftk/rom_patch/cd/sector_view.h>

#include <ppftk/rom_patch/cd/spec.h>

#include <ppfbase/logging/logging.h>

namespace tdd::tk::rompatch::cd {

SectorView::SectorView() noexcept
   : m_number(0)
   , m_offset(0)
   , m_data()
{}

SectorView::SectorView(
   const cd::ByteAddress addr,
   std::span<uint8_t> data) noexcept
   : m_number(ToSectorNumber(addr))
   , m_offset(GetSectorOffset(addr))
   , m_data(data)
{
   TDD_DCHECK(data.size_bytes() <= spec::kSectorSize, "Data too big");
   if (data.empty()) {
      m_offset.get() = spec::kSectorSize;
   }
}

SectorNumber SectorView::SectorNumber() const noexcept
{
   return m_number;
}

ByteAddress SectorView::SectorAddress() const noexcept
{
   return ByteAddress(m_number.get() * spec::kSectorSize);
}

SectorOffset SectorView::DataOffset() const noexcept
{
   return m_offset;
}

size_t SectorView::Size() const noexcept
{
   return m_data.size_bytes();
}

bool SectorView::Empty() const noexcept
{
   return m_data.empty();
}

std::span<uint8_t> SectorView::Data() const noexcept
{
   return m_data;
}

bool SectorView::IsComplete() const noexcept
{
   return m_data.size_bytes() == spec::kSectorSize;
}

spec::Sector* SectorView::AsSector() const noexcept
{
   TDD_DCHECK(m_data.size_bytes() == spec::kSectorSize, "Incomplete sector");

   return reinterpret_cast<spec::Sector*>(m_data.data());
}

uint8_t& SectorView::operator[](const SectorOffset idx) noexcept
{
   TDD_DCHECK(
      m_offset <= idx && idx.get() < spec::kSectorSize,
      "Idx out of bound");
   return m_data[idx.get() - m_offset.get()];
}

uint8_t SectorView::operator[](const SectorOffset idx) const noexcept
{
   TDD_DCHECK(
      m_offset <= idx && idx.get() < spec::kSectorSize,
      "Idx out of bound");
   return m_data[idx.get() - m_offset.get()];
}

}