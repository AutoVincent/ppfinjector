#include <ppftk/rom_patch/flat_patch.h>
#include <ppftk/rom_patch/ppf/parser.h>
#include <ppftk/rom_patch/ppf/ppf3.h>

#include <ppfbase/branding.h>
#include <ppfbase/logging/logging.h>
#include <ppfbase/process/this_process.h>
#include <ppfbase/stdext/iostream.h>

#include <fstream>
#include <iostream>

#include <Windows.h>

namespace tdd::app::patchtester {
   void TestInjector()
   {
      static constexpr size_t kBufferSize = 20480;

      static const std::filesystem::path targetFile(
         "C:\\dev\\ppfinjector\\app\\patchtester\\data\\SotN-Randomizer (1691056147921).bin");

      static const std::filesystem::path verificationFile(
         "C:\\dev\\ppfinjector\\app\\patchtester\\data\\verification.bin");


      auto& launcher = base::process::ThisProcess::ImagePath();
      const auto injectorPath =
         (launcher.parent_path() / TDD_PPF_INJECTOR_DLL_W).wstring();

      const auto hInjector = ::LoadLibraryW(injectorPath.c_str());
      if (NULL == hInjector) {
         std::cout << "Unable to load injector." << std::endl;
         return;
      }

      std::ifstream target(targetFile, std::ios::binary);
      std::ifstream verification(verificationFile, std::ios::binary);

      std::vector<char> tBuf(kBufferSize);
      std::vector<char> vBuf(kBufferSize);

      verification.seekg(0, std::ios::end);
      size_t read = 0;
      size_t left = verification.tellg();
      verification.seekg(0, std::ios::beg);
      while (left > 0) {
         if (left < kBufferSize) {
            tBuf.resize(left);
            vBuf.resize(left);
         }

         stdext::Read(target, tBuf);
         stdext::Read(verification, vBuf);

         if (tBuf != vBuf) {
            std::cout << "Mismatch at " << read << std::endl;
            return;
         }
         read += tBuf.size();
         left -= tBuf.size();
      }

      std::cout << "Identical data read. Patch injector is working"
         << std::endl;

   }
}

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

   {
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
      const auto patchCount = std::distance(flatPatch.begin(), flatPatch.end());
      std::cout << "Flattend patch has " << patchCount << " entries" << std::endl;

      auto lastFromEnd = flatPatch.end();
      --lastFromEnd;
      auto lastFromBegin = flatPatch.begin();
      std::advance(lastFromBegin, patchCount - 1);
      if (lastFromBegin != lastFromEnd) {
         std::cout << "Last iterator not equal" << std::endl;
      }
   }


   tdd::app::patchtester::TestInjector();

   return 0;
}