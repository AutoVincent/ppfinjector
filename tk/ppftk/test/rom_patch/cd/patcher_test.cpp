#include <ppftk/rom_patch/cd/patcher.h>

#include "test_sector_data.h"

#include <ppftk/rom_patch/patch_descriptor.h>

#include <doctest/doctest.h>

namespace tdd::tk::rompatch::cd {

namespace {
   PatchDescriptor BuildPatches()
   {
      PatchDescriptor patches;
      CHECK(patches.AddPatchData(
         TestSector::kPatch.address,
         TestSector::kPatch.data));
      return patches;
   }
}

TEST_CASE("Patcher: patch 1 complete sector")
{
   auto sectorData = TestSector::kOriginalSector;

   Patcher patcher(BuildPatches());

   CHECK(!patcher.Patch(TestSector::kSectorAddr, sectorData).has_value());

   const auto sector = reinterpret_cast<spec::Sector*>(sectorData.data());
   CHECK(
      sector->xa.form1.edc.full ==
      TestSector::kVerificationSector->xa.form1.edc.full);
}

TEST_CASE("Patcher: patching 1 partial sector")
{
   static constexpr size_t kOffset = 16;

   auto sectorData = TestSector::kOriginalSector;
   std::span<uint8_t> targetPortion(
      sectorData.begin() + kOffset,
      sectorData.end() - kOffset);

   auto additionalRead = TestSector::kOriginalSector;

   Patcher patcher(BuildPatches());

   const auto additionalReads =
      patcher.Patch(TestSector::kSectorAddr + kOffset, targetPortion);

   CHECK(additionalReads.has_value());
   CHECK(additionalReads->firstAddr == TestSector::kSectorAddr);
   CHECK(additionalReads->lastAddr == 0);

   // Nothing has been done to the target portion yet
   CHECK(
      0 ==
      memcmp(
         &TestSector::kOriginalSector[kOffset],
         targetPortion.data(),
         targetPortion.size_bytes()));

   CHECK(!patcher.Patch(TestSector::kSectorAddr, additionalRead).has_value());

   CHECK(!patcher.Patch(TestSector::kSectorAddr + kOffset, targetPortion)
             .has_value());

   const auto sector = reinterpret_cast<spec::Sector*>(sectorData.data());
   CHECK(
      sector->xa.form1.edc.full ==
      TestSector::kVerificationSector->xa.form1.edc.full);
}

}