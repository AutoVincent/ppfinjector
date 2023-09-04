#include <ppftk/rom_patch/cd/sector_patch.h>

#include "test_sector_data.h"

#include <doctest/doctest.h>

namespace tdd::tk::rompatch::cd {

TEST_CASE("SectorPatch: Produce correct EDC after patching")
{
   static constexpr auto kSizeExcludeEcc = spec::kSectorSize -
      sizeof(spec::Mode2Xa1Data::pParity) - sizeof(spec::Mode2Xa1Data::qParity);

   SectorPatch patcher(TestSector::kPatch);

   // 1. Patch the whole sector to generate updated edc. Setup the initial state
   // for the sub-cases.
   {
      CHECK(!patcher.HasUpdatedEdc());
      
      auto sector = TestSector::kOriginalSector;
      SectorView sv(TestSector::kSectorAddr, sector);
      patcher.Patch(sv);
      CHECK(patcher.HasUpdatedEdc());

      const auto patchedSector = sv.AsSector();
      CHECK(
         patchedSector->xa.form1.edc.full ==
         TestSector::kVerificationSector->xa.form1.edc.full);
      CHECK(
         0 ==
         memcmp(
            sector.data(),
            TestSector::kVerificationSectorRaw.data(),
            kSizeExcludeEcc));
   }

   SUBCASE("Patch with cached EDC")
   {
      CHECK(patcher.HasUpdatedEdc());

      auto sector = TestSector::kOriginalSector;
      SectorView sv(TestSector::kSectorAddr, sector);
      patcher.Patch(sv);

      const auto patchedSector = sv.AsSector();
      CHECK(
         patchedSector->xa.form1.edc.full ==
         TestSector::kVerificationSector->xa.form1.edc.full);
      CHECK(
         0 ==
         memcmp(
            sector.data(),
            TestSector::kVerificationSectorRaw.data(),
            kSizeExcludeEcc));
   }

   SUBCASE("Patch unaffected portion")
   {
      auto sector = TestSector::kOriginalSector;
      std::span<uint8_t> targetBlock(sector.begin(), TestSector::kSectorOffset);
      SectorView sv(TestSector::kSectorAddr, targetBlock);
      patcher.Patch(sv);
      CHECK(
         0 ==
         memcmp(
            sector.data(),
            TestSector::kOriginalSector.data(),
            targetBlock.size_bytes()));
   }

   SUBCASE("Patch affected snippet")
   {
      static constexpr auto kSnippetOffset = TestSector::kSectorOffset & (~0xF);
      static constexpr auto kSnippetSize = 16;

      static const std::span<const uint8_t> kPatchedData(
         TestSector::kVerificationSectorRaw.begin() + kSnippetOffset,
         kSnippetSize);

      auto sector = TestSector::kOriginalSector;
      std::span<uint8_t> targetBlock(
         sector.begin() + kSnippetOffset,
         kSnippetSize);

      CHECK(0 != memcmp(kPatchedData.data(), targetBlock.data(), kSnippetSize));

      SectorView sv(TestSector::kSectorAddr + kSnippetOffset, targetBlock);
      patcher.Patch(sv);

      CHECK(0 == memcmp(kPatchedData.data(), targetBlock.data(), kSnippetSize));
   }

   SUBCASE("Patch only EDC")
   {
      static constexpr auto kOffset = kSizeExcludeEcc - sizeof(spec::Edc);
      auto sector = TestSector::kOriginalSector;
      std::span<uint8_t> targetBlock(sector.begin() + kOffset, sector.end());
      CHECK(
         0 !=
         memcmp(
            targetBlock.data(),
            &TestSector::kVerificationSector->xa.form1.edc,
            sizeof(spec::Edc)));

      SectorView sv(TestSector::kSectorAddr + kOffset, targetBlock);
      patcher.Patch(sv);
      CHECK(
         0 ==
         memcmp(
            targetBlock.data(),
            &TestSector::kVerificationSector->xa.form1.edc,
            sizeof(spec::Edc)));
   }

   SUBCASE("Patch individual EDC byte")
   {
      static constexpr auto kOffset = kSizeExcludeEcc - sizeof(spec::Edc);
      
      for (auto i = 0; i < sizeof(spec::Edc); ++i) {
         auto sector = TestSector::kOriginalSector;
         std::span<uint8_t> targetBlock(sector.begin() + kOffset + i, 1);
         CHECK(
            targetBlock[0] !=
            TestSector::kVerificationSector->xa.form1.edc.parts[i]);

         SectorView sv(TestSector::kSectorAddr + kOffset + i, targetBlock);
         patcher.Patch(sv);
         CHECK(
            targetBlock[0] ==
            TestSector::kVerificationSector->xa.form1.edc.parts[i]);
      }
   }
}

}