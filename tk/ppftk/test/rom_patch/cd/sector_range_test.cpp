#include <ppftk/rom_patch/cd/sector_range.h>

#include <ppftk/rom_patch/cd/spec.h>

#include <doctest/doctest.h>

#include <array>

namespace tdd::tk::rompatch::cd {

namespace {
   static constexpr size_t kTestSectorCount = 12;
   using TestSectors = std::array<spec::Sector, kTestSectorCount>;

   TestSectors BuildTestSectors() noexcept
   {
      TestSectors sectors;
      for (auto& s : sectors) {
         std::copy(spec::kSync.begin(), spec::kSync.end(), s.sync);
      }
      return sectors;
   }

   static auto kTestSectors = BuildTestSectors();
   static const std::span<uint8_t> kSectorData(
      reinterpret_cast<uint8_t*>(kTestSectors.data()),
      kTestSectorCount* spec::kSectorSize);

   static constexpr ByteAddress kTestSectorStart(123 * spec::kSectorSize);

   static constexpr ByteAddressDiff kSectorOffset(spec::kSectorSize / 2);

   [[nodiscard]] size_t CountSectors(const SectorRange& range)
   {
      size_t count = 0;
      for (const auto& s : range) {
         ++count;
         if (s.IsComplete()) {
            const auto res = memcmp(
               s.AsSector()->sync,
               spec::kSync.data(),
               spec::kSync.size());
            CHECK(0 == res);
         }
      }
      return count;
   }
}

TEST_CASE("SectorRange: empty range")
{
   SectorRange range;
   CHECK(0 == range.size());
   CHECK(0 == std::distance(range.begin(), range.end()).get());
}

TEST_CASE("SectorRange: size")
{
   SUBCASE("Complete sectors")
   {
      SectorRange range(kTestSectorStart, kSectorData);
      CHECK(kTestSectorCount == range.size());
   }

   SUBCASE("Partial first sector")
   {
      SectorRange range(
         kTestSectorStart + kSectorOffset,
         kSectorData.subspan(kSectorOffset.get()));
      CHECK(kTestSectorCount == range.size());
   }

   SUBCASE("Partial last sector")
   {
      SectorRange range(
         kTestSectorStart,
         kSectorData.subspan(
            kSectorOffset.get(),
            kSectorData.size_bytes() - kSectorOffset.get()));
      CHECK(kTestSectorCount == range.size());
   }

   SUBCASE("Partial first and last sector")
   {
      // Instead of adjusting the start and end address, the whole range is
      // simply shifted by kSectorOffset. This makes the data invalid sectors.
      // But this test isn't using the data.
      SectorRange range(kTestSectorStart + kSectorOffset, kSectorData);
      CHECK(kTestSectorCount + 1 == range.size());
   }
}

TEST_CASE("SectorRange: Iterate complete sectors")
{
   SectorRange range(kTestSectorStart, kSectorData);
   CHECK(kTestSectorCount == std::distance(range.begin(), range.end()).get());
   CHECK(kTestSectorCount == CountSectors(range));
}

TEST_CASE("SectorRange: Iterate sectors with partial first")
{
   SectorRange range(
      kTestSectorStart + kSectorOffset,
      kSectorData.subspan(kSectorOffset.get()));
   CHECK(kTestSectorCount == std::distance(range.begin(), range.end()).get());
   CHECK(kTestSectorCount == CountSectors(range));
}

TEST_CASE("SectorRange: Iterate sectors with partial last")
{
   SectorRange range(
      kTestSectorStart,
      kSectorData.subspan(0, kSectorData.size_bytes() - kSectorOffset.get()));
   CHECK(kTestSectorCount == std::distance(range.begin(), range.end()).get());
   CHECK(kTestSectorCount == CountSectors(range));
}

TEST_CASE("SectorRange: Iterate sectors with partial first and last")
{
   SectorRange range(
      kTestSectorStart + kSectorOffset,
      kSectorData.subspan(
         kSectorOffset.get(),
         kSectorData.size_bytes() - spec::kSectorSize));

   auto end = range.end();

   CHECK(kTestSectorCount == std::distance(range.begin(), end).get());
   CHECK(kTestSectorCount == CountSectors(range));
}

TEST_CASE("SectorRange: Mid portion of a single sector")
{
   const SectorRange range(
      kTestSectorStart + kSectorOffset,
      kSectorData.subspan(kSectorOffset.get(), 16));

   const auto beg = range.begin();
   auto end = range.end();
   --end;
   CHECK(beg == end);
}

TEST_CASE("SectorRange: 2 partial sectors")
{
   const SectorRange range(
      kTestSectorStart + kSectorOffset,
      kSectorData.subspan(kSectorOffset.get(), spec::kSectorSize));
   CHECK(range.size() == 2);

   const auto beg = range.begin();
   CHECK(AsByteAddressDiff(beg->DataOffset()) == kSectorOffset);
   const auto last = beg + SectorDiff(1);
   auto end = range.end();
   --end;
   CHECK(last == end);
}

}