#include <ppftk/rom_patch/cd/sector_range.h>

#include <ppftk/rom_patch/cd/spec.h>

#include <ppfbase/logging/logging.h>

namespace tdd::tk::rompatch::cd {

namespace {
   static constexpr ByteAddressDiff kOneSector(spec::kSectorSize);
   static constexpr SectorDiff kSectorDiffZero(0);

   [[nodiscard]] SectorNumber EndSector(const ByteAddress endAddr)
   {
      // Add 1 if ends in the middle of a sector.
      const size_t addOne = GetSectorOffset(endAddr).get() != 0;
      return SectorNumber(ToSectorNumber(endAddr).get() + addOne);
   }
}

SectorRange::ConstIterator::ConstIterator() noexcept
   : m_sector()
   , m_rangeStart(0)
   , m_rangeEnd(0)
   , m_range()
{}

SectorRange::ConstIterator::reference SectorRange::ConstIterator::operator*()
   const noexcept
{
   return *(operator->());
}

SectorRange::ConstIterator::pointer SectorRange::ConstIterator::operator->()
   const noexcept
{
   TDD_CHECK(m_sector.SectorAddress() <= m_rangeEnd, "Can't dereference end");

   return &m_sector;
}

SectorRange::ConstIterator& SectorRange::ConstIterator::operator++() noexcept
{
   auto sectorAddr = m_sector.SectorAddress();
   TDD_CHECK(sectorAddr < m_rangeEnd, "Can't increment past end");

   sectorAddr += ByteAddressDiff(spec::kSectorSize);
   LoadSector(sectorAddr);
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
   auto sectorAddr = m_sector.SectorAddress();
   TDD_CHECK(sectorAddr >= m_rangeStart, "Can't decrement past begin");

   sectorAddr.get() -= spec::kSectorSize;
   LoadSector(sectorAddr);

   return *this;
}

SectorRange::ConstIterator SectorRange::ConstIterator::operator--(int) noexcept
{
   const auto tmp = *this;
   this->operator--();
   return tmp;
}

SectorRange::ConstIterator& SectorRange::ConstIterator::operator+=(
   const difference_type offset) noexcept
{
   if (offset == kSectorDiffZero) {
      return *this;
   }
   else if (offset < kSectorDiffZero) {
      return this->operator-=(-offset);
   }

   const auto oneBeforeTarget =
      ToByteAddress(m_sector.SectorNumber() + offset - SectorDiff(1));

   TDD_CHECK(oneBeforeTarget < m_rangeEnd, "Can't seek after end");

   m_sector = SectorView(oneBeforeTarget, {});
   return this->operator++();
}

SectorRange::ConstIterator SectorRange::ConstIterator::operator+(
   const difference_type offset) const noexcept
{
   auto tmp = *this;
   tmp += offset;
   return tmp;
}

SectorRange::ConstIterator& SectorRange::ConstIterator::operator-=(
   const difference_type offset) noexcept
{
   if (offset == kSectorDiffZero) {
      return *this;
   }
   else if (offset < kSectorDiffZero) {
      return this->operator+=(-offset);
   }

   const auto oneAfterTarget =
      ToByteAddress(m_sector.SectorNumber() - offset + SectorDiff(1));

   TDD_CHECK(oneAfterTarget > m_rangeStart, "Can't seek before begin");

   m_sector = SectorView(oneAfterTarget, {});
   return this->operator--();
}

SectorRange::ConstIterator SectorRange::ConstIterator::operator-(
   const difference_type offset) const noexcept
{
   auto tmp = *this;
   tmp -= offset;
   return tmp;
}

SectorRange::ConstIterator::difference_type SectorRange::ConstIterator::
operator-(const ConstIterator& rhs) const noexcept
{
   CheckCompatible(rhs);
   return m_sector.SectorNumber() - rhs.m_sector.SectorNumber();
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
   return m_sector.SectorNumber() == other.m_sector.SectorNumber();
}

std::strong_ordering SectorRange::ConstIterator::operator<=>(
   const ConstIterator& rhs) const noexcept
{
   CheckCompatible(rhs);
   return m_sector.SectorNumber() <=> rhs.m_sector.SectorNumber();
}

SectorRange::ConstIterator::ConstIterator(
   const SectorNumber sectorNumber,
   const ByteAddress rangeAddr,
   std::span<uint8_t> range) noexcept
   : m_sector()
   , m_rangeStart(rangeAddr)
   , m_rangeEnd(rangeAddr + ByteAddressDiff(range.size_bytes()))
   , m_range(range)
{
   const auto startSector = ToSectorNumber(m_rangeStart);
   const auto endSector = EndSector(m_rangeEnd);

   TDD_CHECK(
      startSector <= sectorNumber && sectorNumber <= endSector,
      "Sector out of range");

   const auto sectorAddress = ToByteAddress(sectorNumber);

   if (sectorNumber == endSector) {
      m_sector = SectorView(sectorAddress, {});
      return;
   }

   LoadSector(sectorAddress);
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
void SectorRange::ConstIterator::LoadSector(ByteAddress sectorAddr) noexcept
{
   TDD_DCHECK(
      Is0thSectorByte(sectorAddr),
      "Expect sector aligned address");

   std::span<uint8_t> data;
   if (sectorAddr >= m_rangeEnd) {
      TDD_DCHECK(
         m_rangeEnd + kOneSector >= sectorAddr,
         "Sector address too far after range");
      // We've reached the end. No need to populate 'data'.
   }
   else if (sectorAddr < m_rangeStart) {
      TDD_DCHECK(
         sectorAddr + kOneSector > m_rangeStart,
         "Sector address too far ahead of range");
      const auto count =
         static_cast<size_t>((sectorAddr + kOneSector - m_rangeStart).get());
      data = m_range.subspan(0, std::min(count, m_range.size_bytes()));
      sectorAddr = m_rangeStart;
   }
   else {
      const auto rangeOffset = (sectorAddr - m_rangeStart).get();
      const auto count =
         std::min(m_range.size() - rangeOffset, spec::kSectorSize);
      data = m_range.subspan(rangeOffset, count);
   }

   m_sector = SectorView(sectorAddr, data);
}

SectorRange::Iterator::reference SectorRange::Iterator::operator*()
   const noexcept
{
   return *(this->operator->());
}

SectorRange::Iterator::pointer SectorRange::Iterator::operator->()
   const noexcept
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

SectorRange::Iterator& SectorRange::Iterator::operator+=(
   const difference_type offset) noexcept
{
   Base::operator+=(offset);
   return *this;
}

SectorRange::Iterator SectorRange::Iterator::operator+(
   const difference_type offset) noexcept
{
   auto tmp = *this;
   tmp += offset;
   return tmp;
}

SectorRange::Iterator& SectorRange::Iterator::operator-=(
   const difference_type offset) noexcept
{
   Base::operator-=(offset);
   return *this;
}

SectorRange::Iterator SectorRange::Iterator::operator-(
   const difference_type offset) noexcept
{
   auto tmp = *this;
   tmp -= offset;
   return tmp;
}

SectorRange::Iterator::reference SectorRange::Iterator::operator[](
   const difference_type offset) const noexcept
{
   return const_cast<reference>(Base::operator[](offset));
}

SectorRange::Iterator::Iterator(
   const SectorNumber sectorNumber,
   const ByteAddress rangeAddr,
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

SectorRange::SectorRange(const ByteAddress addr, std::span<uint8_t> data) noexcept
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
   return const_iterator(ToSectorNumber(m_addr), m_addr, m_data);
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
      EndSector(m_addr + ByteAddressDiff(m_data.size_bytes())),
      m_addr,
      m_data);
}

bool SectorRange::empty() const noexcept
{
   return m_data.empty();
}

SectorRange::size_type SectorRange::size() const noexcept
{
   const auto endAddr = m_addr + ByteAddressDiff(m_data.size_bytes());
   return (EndSector(endAddr) - ToSectorNumber(m_addr)).get();
}

}