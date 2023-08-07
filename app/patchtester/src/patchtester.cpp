#include <ppftk/rom_patch/flat_patch.h>
#include <ppftk/rom_patch/ppf/parser.h>
#include <ppftk/rom_patch/ppf/ppf3.h>

#include <ppfbase/logging/logging.h>

#include <fstream>
#include <iostream>

int main()
{
   tdd::base::logging::InitSingleProcessLog();

   const std::filesystem::path testPatch(
      "C:\\dev\\ppfinjector\\app\\patchtester\\data\\SotN-Randomizer (1691056147921).ppf");

   const std::filesystem::path testTarget(
      "C:\\Games\\Castlevania-Symphony of the Night[U] [SLUS-00067]\\Castlevania - Symphony of the Night (USA) (Track 1).bin");

   const std::filesystem::path compactedPatch(
      "C:\\dev\\ppfinjector\\app\\patchtester\\data\\compacted.ppf");

   auto patch = tdd::tk::rompatch::ppf::Parse(testPatch);

   if (!patch.has_value()) {
      std::cout << "Failed to parse the patch" << std::endl;
      return 1;
   }

   std::cout << "Success." << std::endl;
   std::cout << "Validation data: "
      << (patch->GetValidationData().data.empty() ? "not" : "")
      << " present" << std::endl;
   std::cout << "Patch entries: " << patch->GetFullPatch().size()
      << std::endl;

   //patch->Compact();
   //std::cout << "After compact" << std::endl;
   //std::cout << "Patch entries: " << patch->GetFullPatch().size()
   //   << std::endl;

   auto target = std::ifstream(testTarget, std::ios::binary);
   if (!patch->Compact(target, 128)) {
      std::cout << "Unable to compact with fillers";
      return 2;
   }
   std::cout << "After compact" << std::endl;
   std::cout << "Patch entries: " << patch->GetFullPatch().size()
      << std::endl;

   if (tdd::tk::rompatch::ppf::WritePpf3Patch(compactedPatch, patch.value())) {
      std::cout << "Unable to write compacted PFF" << std::endl;
   }
   else {
      std::cout << "Compacted patch saved" << std::endl;
   }

   const auto flatPatch = patch->Flatten();
   std::cout << "Flattend patch has "
      << std::distance(flatPatch.begin(), flatPatch.end()) << " entries"
      << std::endl;

   return 0;
}