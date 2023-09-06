#include <ppftk/rom_patch/cd/address.h>

#include <ppftk/rom_patch/cd/spec.h>

#include <ppfbase/logging/logging.h>

#include <iostream>

namespace tdd::tk::rompatch::cd {
namespace {
   static constexpr ByteAddressDiff kByteAddrNoDiff(0);
   static constexpr SectorDiff kSectorNoDiff(0);
}

////////////////////////////////////////////////////////////////////////////////
// ByteAddress operations
////////////////////////////////////////////////////////////////////////////////
bool Is0thSectorByte(const ByteAddress addr) noexcept
{
   return 0 == addr.get() % spec::kSectorSize;
}

SectorNumber ToSectorNumber(const ByteAddress addr) noexcept
{
   return SectorNumber(addr.get() / spec::kSectorSize);
}

SectorOffset GetSectorOffset(const ByteAddress addr) noexcept
{
   return SectorOffset(addr.get() % spec::kSectorSize);
}

ByteAddressDiff operator-(
   const ByteAddress lhs,
   const ByteAddress rhs) noexcept
{
   if (lhs < rhs) {
      const ByteAddressDiff diff(rhs.get() - lhs.get());
      return -diff;
   }
   else {
      return ByteAddressDiff(lhs.get() - rhs.get());
   }
}

ByteAddress operator+(
   const ByteAddress addr,
   const ByteAddressDiff offset) noexcept
{
   auto ret = addr;
   ret+= offset;
   return ret;
}

ByteAddress& operator+=(
   ByteAddress& addr,
   const ByteAddressDiff offset) noexcept
{
   if (offset < kByteAddrNoDiff) {
      return addr -= -offset;
   }

   addr.get() += offset.get();
   return addr;
}

ByteAddress operator-(
   const ByteAddress addr,
   const ByteAddressDiff offset) noexcept
{
   auto ret = addr;
   ret -= offset;
   return ret;
}

ByteAddress& operator-=(
   ByteAddress& addr,
   const ByteAddressDiff offset) noexcept
{
   if (offset < kByteAddrNoDiff) {
      return addr += -offset;
   }

   TDD_CHECK(
      addr.get() >= static_cast<ByteAddress::value_type>(offset.get()),
      "Address can't be negative");

   addr.get() -= offset.get();
   return addr;
}

ByteAddressDiff operator-(const ByteAddressDiff diff) noexcept
{
   return ByteAddressDiff(-diff.get());
}

////////////////////////////////////////////////////////////////////////////////
// SectorNumber operations
////////////////////////////////////////////////////////////////////////////////
ByteAddress ToByteAddress(const SectorNumber sector) noexcept
{
   return ByteAddress(sector.get() * spec::kSectorSize);
}

ByteAddressDiff AsByteAddressDiff(const SectorOffset offset) noexcept
{
   return ByteAddressDiff(offset.get());
}

ByteAddress operator+(
   const SectorNumber sector,
   const SectorOffset offset) noexcept
{
   auto addr = ToByteAddress(sector);
   addr.get() += offset.get();
   return addr;
}

SectorDiff operator-(const SectorNumber lhs, const SectorNumber rhs) noexcept
{
   if (lhs >= rhs) {
      return SectorDiff(lhs.get() - rhs.get());
   }
   else {
      SectorDiff diff(rhs.get() - lhs.get());
      diff.get() *= -1;
      return diff;
   }
}

SectorNumber operator+(const SectorNumber sector, const SectorDiff offset) noexcept
{
   auto ret = sector;
   ret += offset;
   return ret;
}

SectorNumber& operator+=(SectorNumber& sector, const SectorDiff offset) noexcept
{
   if (offset < kSectorNoDiff){
      return sector -= -offset;
   }
   
   sector.get() += offset.get();
   return sector;
}

SectorNumber operator-(
   const SectorNumber sector,
   const SectorDiff offset) noexcept
{
   auto ret = sector;
   ret -= offset;
   return ret;
}

SectorNumber& operator-=(SectorNumber& sector, const SectorDiff offset) noexcept
{
   if (offset < kSectorNoDiff) {
      return sector += -offset;
   }

   TDD_CHECK(
      sector.get() >= static_cast<SectorNumber::value_type>(offset.get()),
      "Sector number can't be negative");

   sector.get() -= offset.get();
   return sector;
}

SectorDiff operator-(const SectorDiff diff) noexcept
{
   return SectorDiff(-diff.get());
}

}

std::ostream& operator<<(
   std::ostream& os,
   const tdd::tk::rompatch::cd::SectorNumber sector)
{
   return os << "Sector " << sector.get();
}
