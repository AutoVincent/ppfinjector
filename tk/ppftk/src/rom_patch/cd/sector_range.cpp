#include <ppftk/rom_patch/cd/sector_range.h>

#include <ppftk/rom_patch/cd/spec.h>

#include <ppfbase/logging/logging.h>

namespace tdd::tk::rompatch::cd {

namespace {
   [[nodiscard]] uint64_t StartSector(const uint64_t startAddr)
   {
      return startAddr / spec::kSectorSize;
   }

   [[nodiscard]] uint64_t EndSector(const uint64_t endAddr)
   {
      // Add 1 if ends in the middle of a sector.
      return endAddr / spec::kSectorSize + !!(endAddr % spec::kSectorSize);
   }
}

bool SectorRange::SectorView::IsComplete() const noexcept
{
   return data.size_bytes() == spec::kSectorSize;
}

spec::Sector* SectorRange::SectorView::AsSector() const noexcept
{
   TDD_DCHECK(data.size_bytes() == spec::kSectorSize, "Incomplete sector");

   return reinterpret_cast<spec::Sector*>(data.data());
}

SectorRange::ConstIterator::ConstIterator() noexcept
   : m_sector{0}
   , m_rangeStart(0)
   , m_rangeEnd(0)
   , m_range()
{}

SectorRange::ConstIterator::reference
SectorRange::ConstIterator::operator*() const noexcept
{
   return *(operator->());
}

SectorRange::ConstIterator::pointer
SectorRange::ConstIterator::operator->() const noexcept
{
   TDD_CHECK(
      m_sector.number * spec::kSectorSize <= m_rangeEnd,
      "Can't dereference end");

   return &m_sector;
}

SectorRange::ConstIterator& SectorRange::ConstIterator::operator++() noexcept
{
   auto sectorAddr = m_sector.number * spec::kSectorSize;
   TDD_CHECK(sectorAddr < m_rangeEnd, "Can't increment past end");

   ++m_sector.number;
   sectorAddr += spec::kSectorSize;
   if (sectorAddr >= m_rangeEnd) {
      // we've reached the end;
      m_sector.data = std::span<uint8_t>();
      return *this;
   }

   const auto count = std::min(m_rangeEnd - sectorAddr, spec::kSectorSize);

   m_sector.data = m_range.subspan(sectorAddr - m_rangeStart, count);

   return *this;
}

SectorRange::ConstIterator SectorRange::ConstIterator::operator++(int) noexcept
{
   const auto tmp = *this;
   this->operator++();
   return tmp;
}

SectorRange::ConstIterator& SectorRange::ConstIterator::operator--() noexcept
{
   auto sectorAddr = m_sector.number * spec::kSectorSize;
   TDD_CHECK(sectorAddr >= m_rangeStart, "Can't decrement past begin");

   --m_sector.number;
   sectorAddr -= spec::kSectorSize;
   if (sectorAddr < m_rangeStart) {
      const auto count = spec::kSectorSize + sectorAddr - m_rangeStart;
      m_sector.data    = m_range.subspan(0, count);
   }
   else {
      m_sector.data =
         m_range.subspan(sectorAddr - m_rangeStart, spec::kSectorSize);
   }

   return *this;
}

SectorRange::ConstIterator SectorRange::ConstIterator::operator--(int) noexcept
{
   const auto tmp = *this;
   this->operator--();
   return tmp;
}

SectorRange::ConstIterator&
SectorRange::ConstIterator::operator+=(const difference_type offset) noexcept
{
   if (offset == 0) {
      return *this;
   }
   else if (offset < 0) {
      return this->operator-=(-offset);
   }

   const auto targetSectorAddr =
      (m_sector.number + offset - 1) * spec::kSectorSize;
   TDD_CHECK(targetSectorAddr < m_rangeEnd, "Can't seek after end");

   m_sector.number += offset - 1;
   return this->operator++();
}

SectorRange::ConstIterator SectorRange::ConstIterator::operator+(
   const difference_type offset) const noexcept
{
   auto tmp = *this;
   tmp += offset;
   return tmp;
}

SectorRange::ConstIterator&
SectorRange::ConstIterator::operator-=(const difference_type offset) noexcept
{
   if (offset == 0) {
      return *this;
   }
   else if (offset < 0) {
      return this->operator+=(-offset);
   }

   const auto targetSectorAddr =
      (m_sector.number - offset + 1) * spec::kSectorSize;
   TDD_CHECK(targetSectorAddr > m_rangeStart, "Can't seek before begin");

   m_sector.number -= offset - 1;
   return this->operator--();
}

SectorRange::ConstIterator SectorRange::ConstIterator::operator-(
   const difference_type offset) const noexcept
{
   auto tmp = *this;
   tmp -= offset;
   return tmp;
}

SectorRange::ConstIterator::difference_type
SectorRange::ConstIterator::operator-(const ConstIterator& rhs) const noexcept
{
   CheckCompatible(rhs);
   const int64_t start = m_sector.number;
   const int64_t end   = rhs.m_sector.number;
   return start - end;
}

SectorRange::ConstIterator::reference SectorRange::ConstIterator::operator[](
   const difference_type offset) const noexcept
{
   return *(this->operator+(offset));
}

bool SectorRange::ConstIterator::operator==(
   const ConstIterator& other) const noexcept
{
   CheckCompatible(other);
   return m_sector.number == other.m_sector.number;
}


std::strong_ordering
SectorRange::ConstIterator::operator<=>(const ConstIterator& rhs) const noexcept
{
   CheckCompatible(rhs);
   return m_sector.number <=> rhs.m_sector.number;
}

SectorRange::ConstIterator::ConstIterator(
   const uint64_t sectorNumber,
   const uint64_t rangeAddr,
   std::span<uint8_t> range) noexcept
   : m_sector()
   , m_rangeStart(rangeAddr)
   , m_rangeEnd(rangeAddr + range.size_bytes())
   , m_range(range)
{
   const auto startSector = StartSector(m_rangeStart);
   const auto endSector   = EndSector(m_rangeEnd);

   TDD_CHECK(
      startSector <= sectorNumber && sectorNumber <= endSector,
      "Sector out of range");

   m_sector.number = sectorNumber;

   if (m_sector.number == endSector) {
      return;
   }

   ++m_sector.number;
   this->operator--();
}

void SectorRange::ConstIterator::CheckCompatible(
   const ConstIterator& other) const noexcept
{
   TDD_CHECK(
      m_rangeStart == other.m_rangeStart,
      "Range doesn't start from the same address");

   TDD_CHECK(
      m_rangeEnd == other.m_rangeEnd,
      "Range doesn't end at the same address");

   TDD_DCHECK(
      0 == memcmp(m_range.data(), other.m_range.data(), m_range.size_bytes()),
      "Different memory content");
}

SectorRange::Iterator::reference
SectorRange::Iterator::operator*() const noexcept
{
   return *(this->operator->());
}

SectorRange::Iterator::pointer
SectorRange::Iterator::operator->() const noexcept
{
   return const_cast<pointer>(Base::operator->());
}

SectorRange::Iterator& SectorRange::Iterator::operator++() noexcept
{
   Base::operator++();
   return *this;
}

SectorRange::Iterator SectorRange::Iterator::operator++(int) noexcept
{
   auto tmp = *this;
   Base::operator++();
   return tmp;
}

SectorRange::Iterator& SectorRange::Iterator::operator--() noexcept
{
   Base::operator--();
   return *this;
}

SectorRange::Iterator SectorRange::Iterator::operator--(int) noexcept
{
   auto tmp = *this;
   Base::operator--();
   return tmp;
}

SectorRange::Iterator&
SectorRange::Iterator::operator+=(const difference_type offset) noexcept
{
   Base::operator+=(offset);
   return *this;
}

SectorRange::Iterator
SectorRange::Iterator::operator+(const difference_type offset) noexcept
{
   auto tmp = *this;
   tmp += offset;
   return tmp;
}

SectorRange::Iterator&
SectorRange::Iterator::operator-=(const difference_type offset) noexcept
{
   Base::operator-=(offset);
   return *this;
}

SectorRange::Iterator
SectorRange::Iterator::operator-(const difference_type offset) noexcept
{
   auto tmp = *this;
   tmp -= offset;
   return tmp;
}

SectorRange::Iterator::reference
SectorRange::Iterator::operator[](const difference_type offset) const noexcept
{
   return const_cast<reference>(Base::operator[](offset));
}

SectorRange::Iterator::Iterator(
   const uint64_t sectorNumber,
   const uint64_t rangeAddr,
   std::span<uint8_t> range) noexcept
   : Base(sectorNumber, rangeAddr, range)
{}

SectorRange::Iterator::Iterator(const ConstIterator& other)
   : Base(other)
{}

SectorRange::SectorRange() noexcept
   : m_addr(0)
   , m_data()
{}

SectorRange::SectorRange(const uint64_t addr, std::span<uint8_t> data) noexcept
   : m_addr(addr)
   , m_data(data)
{}

SectorRange::iterator SectorRange::begin() noexcept
{
   return cbegin();
}

SectorRange::const_iterator SectorRange::begin() const noexcept
{
   return cbegin();
}

SectorRange::const_iterator SectorRange::cbegin() const noexcept
{
   return const_iterator(StartSector(m_addr), m_addr, m_data);
}

SectorRange::iterator SectorRange::end() noexcept
{
   return cend();
}

SectorRange::const_iterator SectorRange::end() const noexcept
{
   return cend();
}

SectorRange::const_iterator SectorRange::cend() const noexcept
{
   return const_iterator(
      EndSector(m_addr + m_data.size_bytes()),
      m_addr,
      m_data);
}

bool SectorRange::empty() const noexcept
{
   return m_data.empty();
}

SectorRange::size_type SectorRange::size() const noexcept
{
   return EndSector(m_addr + m_data.size_bytes()) - StartSector(m_addr);
}

SectorRange::reference SectorRange::front() noexcept
{
   return *begin();
}

SectorRange::const_reference SectorRange::front() const noexcept
{
   return *begin();
}

SectorRange::reference SectorRange::back() noexcept
{
   auto last = end();
   --last;
   return *last;
}

SectorRange::const_reference SectorRange::back() const noexcept
{
   auto last = end();
   --last;
   return *last;
}

}