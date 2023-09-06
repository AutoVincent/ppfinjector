#pragma once

#include <ppfbase/stdext/type_traits.h>

#include <iosfwd>

namespace tdd::tk::rompatch::cd {
namespace details {
   // clang-format off
   struct ByteAddressTag {};
   struct ByteAddressDiffTag {};
   struct SectorNumberTag {};
   struct SectorDiffTag {};
   struct SectorOffsetTag {};
   // clang-format on
}

using ByteAddress = stdext::strong_type<uint64_t, details::ByteAddressTag>;
using ByteAddressDiff =
   stdext::strong_type<int64_t, details::ByteAddressDiffTag>;

using SectorNumber = stdext::strong_type<uint64_t, details::SectorNumberTag>;
using SectorDiff = stdext::strong_type<int64_t, details::SectorDiffTag>;
// Byte location within a sector
using SectorOffset = stdext::strong_type<size_t, details::SectorOffsetTag>;

// ByteAddress opperations
[[nodiscard]] bool Is0thSectorByte(const ByteAddress addr) noexcept;
SectorNumber ToSectorNumber(const ByteAddress addr) noexcept;
SectorOffset GetSectorOffset(const ByteAddress addr) noexcept;

ByteAddressDiff operator-(
   const ByteAddress lhs,
   const ByteAddress rhs) noexcept;

ByteAddress operator+(
   const ByteAddress addr,
   const ByteAddressDiff offset) noexcept;

ByteAddress& operator+=(
   ByteAddress& sector,
   const ByteAddressDiff offset) noexcept;

ByteAddress operator-(
   const ByteAddress sector,
   const ByteAddressDiff offset) noexcept;

ByteAddress& operator-=(
   ByteAddress& sector,
   const ByteAddressDiff offset) noexcept;

ByteAddressDiff operator-(const ByteAddressDiff diff) noexcept;

// SectorNumber operations
ByteAddress ToByteAddress(const SectorNumber sector) noexcept;
ByteAddressDiff AsByteAddressDiff(const SectorOffset offset) noexcept;

ByteAddress operator+(
   const SectorNumber sector,
   const SectorOffset offset) noexcept;

SectorDiff operator-(const SectorNumber lhs, const SectorNumber rhs) noexcept;

SectorNumber operator+(
   const SectorNumber sector,
   const SectorDiff offset) noexcept;

SectorNumber& operator+=(
   SectorNumber& sector,
   const SectorDiff offset) noexcept;

SectorNumber operator-(
   const SectorNumber sector,
   const SectorDiff offset) noexcept;

SectorNumber& operator-=(
   SectorNumber& sector,
   const SectorDiff offset) noexcept;

SectorDiff operator-(const SectorDiff diff) noexcept;

}

std::ostream& operator<<(
   std::ostream& os,
   const tdd::tk::rompatch::cd::SectorNumber sector);
