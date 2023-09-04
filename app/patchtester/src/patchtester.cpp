#include <ppftk/rom_patch/flat_patch.h>
#include <ppftk/rom_patch/cd/spec.h>
#include <ppftk/rom_patch/ppf/parser.h>
#include <ppftk/rom_patch/ppf/ppf3.h>

#include <ppfbase/branding.h>
#include <ppfbase/logging/logging.h>
#include <ppfbase/process/this_process.h>
#include <ppfbase/stdext/iostream.h>

#include <fstream>
#include <iostream>
#include <iomanip>
#include <map>

namespace cd = tdd::tk::rompatch::cd::spec;

template <typename Container>
void DumpBlock(const Container& data)
{
   for (auto i = 0; i < data.size(); ++i) {
      if (i != 0) {
         std::cout << ",";
      }
      if (i % 16 == 0) {
         std::cout << std::endl;
      }
      else {
         std::cout << " ";
      }
      std::cout << "0x" << std::setw(2) << static_cast<uint32_t>(data[i]);
   }
}

using SectorBlock =
   std::array<uint8_t, tdd::tk::rompatch::cd::spec::kSectorSize>;

void DumpSector(const char* varName, const SectorBlock& sector)
{
   std::cout << "std::array<uint8_t, tdd::tk::rompatch::cd::spec::kSectorSize> "
      << varName << " = {";
   std::cout << std::hex << std::setfill('0');
   DumpBlock(sector);
   std::cout << "};" << std::endl << std::endl;
}

void DumpPatchEntry(const tdd::tk::rompatch::PatchItem& patch)
{
   std::cout << "PatchItem address: 0x" << std::hex << std::setfill('0') << std::setw(8)
      << patch.address << std::endl;

   std::cout << "PatchItem data: {";
   DumpBlock(patch.data);
   std::cout << "}" << std::endl << std::endl;
}

int main()
{
   namespace fs = std::filesystem;
   static const fs::path testPatch(
      "C:\\dev\\ppfinjector\\app\\patchtester\\data\\SotN-Randomizer (1691056147921).ppf");

   static const fs::path testTarget(
      "C:\\dev\\ppfinjector\\app\\patchtester\\data\\SotN-Randomizer (1691056147921).bin");

   static const fs::path verification(
      "C:\\dev\\ppfinjector\\app\\patchtester\\data\\verification.bin");

   static const std::map<const char*, fs::path> kDumpTargets{
      {"kOriginalSector", testTarget},
      {"kVerificationSector", verification}
   };

   tdd::base::logging::InitProcessLog();

   auto patch = tdd::tk::rompatch::ppf::Parse(testPatch);

   if (!patch.has_value()) {
      std::cout << "Failed to parse the patch" << std::endl;
      return 1;
   }

   const auto& firstEntry = patch->GetFullPatch().front();
   const auto sectorNumber = firstEntry.address / cd::kSectorSize;
   const auto sectorAddress = sectorNumber * cd::kSectorSize;
   DumpPatchEntry(firstEntry);

   for (const auto& [var, binPath] : kDumpTargets) {
      auto target = std::ifstream(binPath, std::ios::binary);
      SectorBlock sector;
      target.seekg(sectorAddress, std::ios::beg);
      tdd::stdext::Read(target, sector);
      DumpSector(var, sector);
   }
   return 0;
}