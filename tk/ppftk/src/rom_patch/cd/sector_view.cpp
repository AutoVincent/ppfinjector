#include <ppftk/rom_patch/cd/sector_view.h>

#include <ppftk/rom_patch/cd/spec.h>

#include <ppfbase/logging/logging.h>

namespace tdd::tk::rompatch::cd {

SectorView::SectorView()
   : m_number(0)
   , m_offset(0)
   , m_data()
{}

SectorView::SectorView(const uint64_t addr, std::span<uint8_t> data)
   : m_number(addr / spec::kSectorSize)
   , m_offset(addr % spec::kSectorSize)
   , m_data(data)
{
   TDD_DCHECK(data.size_bytes() <= spec::kSectorSize, "Data too big");
   if (data.empty()) {
      m_offset = spec::kSectorSize;
   }
}

uint64_t SectorView::SectorNumber() const noexcept
{
   return m_number;
}

uint64_t SectorView::SectorAddress() const noexcept
{
   return m_number * spec::kSectorSize;
}

size_t SectorView::DataOffset() const noexcept
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
uint8_t& SectorView::operator[](const size_t idx) noexcept
{
   TDD_DCHECK(m_offset <= idx && idx < spec::kSectorSize, "Idx out of bound");
   return m_data[idx-m_offset];
}

uint8_t SectorView::operator[](const size_t idx) const noexcept
{
   TDD_DCHECK(m_offset <= idx && idx < spec::kSectorSize, "Idx out of bound");
   return m_data[idx - m_offset];
}

}