#include <ppftk/rom_patch/ppf/parser.h>

#include <ppfbase/logging/logging.h>

#include <iostream>

int main()
{
   tdd::base::logging::InitSingleProcessLog();

   const std::filesystem::path testPatch(
      "C:\\dev\\ppfinjector\\app\\patchtester\\data\\SotN-Randomizer (1691056147921).ppf");

   const auto patch = tdd::tk::rompatch::ppf::Parse(testPatch);

   if (patch.has_value()) {
      std::cout << "Success." << std::endl;
      std::cout << "Validation data: "
         << (patch->GetValidationData().data.empty() ? "not" : "")
         << " present" << std::endl;
      std::cout << "Patch entries: " << patch->GetFullPatch().size()
         << std::endl;
   }
   else {
      std::cout << "Failed to parse the patch" << std::endl;
   }
   return 0;
}