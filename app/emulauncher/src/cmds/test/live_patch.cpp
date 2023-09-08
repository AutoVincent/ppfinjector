#include "live_patch.h"

#include <ppftk/rom_patch/patch_file_exts.h>
#include <ppftk/rom_patch/cd/spec.h>

#include <ppfbase/branding.h>
#include <ppfbase/logging/logging.h>
#include <ppfbase/process/this_process.h>
#include <ppfbase/stdext/iostream.h>

#include <fstream>

#include <Windows.h>

namespace tdd::app::emulauncher::cmd::test {

namespace {
   namespace fs = std::filesystem;
   namespace cd = tk::rompatch::cd::spec;

   void LoadInjector()
   {
      auto& launcher = base::process::ThisProcess::ImagePath();
      const auto injectorPath =
         (launcher.parent_path() / TDD_PPF_INJECTOR_DLL_W).wstring();

      const auto hInjector = ::LoadLibraryW(injectorPath.c_str());
      if (NULL == hInjector) {
         throw std::runtime_error("Unable to load injector.");
      }

      // Leave injector loaded until process exits.
   }

   [[nodiscard]] size_t GetFileSize(
      std::istream& target,
      std::istream& verification)
   {
      target.seekg(0, std::ios::end);
      const size_t targetSize = target.tellg();

      verification.seekg(0, std::ios::end);
      if (targetSize != verification.tellg()) {
         throw std::runtime_error(
            "Original and verification have different sizes");
      }

      target.seekg(0, std::ios::beg);
      verification.seekg(0, std::ios::beg);
      return targetSize;
   }

   // Expects the 'target' and 'verification' to be identical. No special
   // treatment given to these files.
   void SimpleVerification(std::istream& target, std::istream& verification)
   {
      // 8KB verification chunk size.
      static constexpr size_t kChunkSize = 8 * 1024;

      size_t left = GetFileSize(target, verification);

      std::vector<char> patched(kChunkSize);
      std::vector<char> expected(kChunkSize);
      size_t read = 0;

      while (left > 0) {
         if (left < kChunkSize) {
            patched.resize(left);
            expected.resize(left);
         }

         stdext::Read(target, patched);
         stdext::Read(verification, expected);

         if (patched != expected) {
            for (size_t i = 0; i < patched.size(); ++i) {
               if (patched[i] != expected[i]) {
                  throw std::runtime_error(
                     "Mismatch at file location: " + std::to_string(read + i));
               }
            }
         }

         read += patched.size();
         left -= patched.size();
      }
   }

   void CheckSector(
      const cd::Sector& target,
      const cd::Sector& verification,
      const size_t sectorNumber,
      const bool checkEdc)
   {
      size_t blockSize = 0;
      uint8_t mode = 0;
      uint8_t form = 0;

      bool failForm2Edc = false;

      switch (target.header.parts.mode) {
      default:
         std::cout << "Unknown sector mode for sector " << sectorNumber << ": "
            << target.header.parts.mode << ". Assume Mode 0."
            << std::endl;
      case cd::kMode0:
         blockSize = cd::kSectorSize;
         mode = cd::kMode0;
         break;
      case cd::kMode1:
         mode = cd::kMode1;
         blockSize =
            cd::kSectorSize - cd::kEccSize - (checkEdc ? 0 : cd::kEdcSize);
         break;
      case cd::kMode2:
         mode = 2;
         if (target.xa.subheader[0].full != target.xa.subheader[1].full) {
            std::cout << "Non-XA Mode 2 sector: " << sectorNumber << std::endl;
            blockSize = cd::kSectorSize;
         }
         else if (target.xa.subheader[0].parts.submode.form == cd::kXaForm1) {
            form = 1;
            blockSize =
               cd::kSectorSize - cd::kEccSize - (checkEdc ? 0 : cd::kEdcSize);
         }
         else {
            form = 2;
            blockSize = cd::kSectorSize - cd::kEdcSize;
            const auto tEdc = target.xa.form2.edc.full;
            const auto vEdc = verification.xa.form2.edc.full;
            failForm2Edc = tEdc != 0 && tEdc != vEdc;
         }
         break;
      }

      if (0 != memcmp(&target, &verification, blockSize)) {
         throw std::runtime_error(
            "Sector " + std::to_string(sectorNumber) + " data mismatch");
      }

      if (failForm2Edc) {
         throw std::runtime_error(
            "Form 2 Sector " + std::to_string(sectorNumber) + " EDC mismatch");
      }
   }

   // Don't need to check ECC here. If ECC check were requested,
   // SimpleVerification would have been called instead.
   void CdVerification(
      std::istream& target,
      std::istream& verification,
      const bool checkEdc)
   {
      // The assumption of CD image is that it starts immediately with sector
      // data. The first 12 bytes would be the sync data.

      const auto fileSize = GetFileSize(target, verification);
      if (fileSize % cd::kSectorSize != 0) {
         throw std::runtime_error("Incomplete sector in test files");
      }

      const auto sectorCount = fileSize / cd::kSectorSize;
      std::cout << "Sector count: " << sectorCount << std::endl;
      std::cout << "EDC verification: " << std::boolalpha << checkEdc
                << std::endl;

      for (size_t i = 0; i < sectorCount; ++i) {
         cd::Sector tSec{0};
         cd::Sector vSec{0};

         stdext::Read(target, tSec);
         stdext::Read(verification, vSec);
         CheckSector(tSec, vSec, i, checkEdc);
      }
   }
}

LivePatch::LivePatch(CLI::App& test)
   : m_cmd(test.add_subcommand(
        "live_patch",
        "Verify the live patching logic by patching the original file on the "
        "fly "
        "and compare the data with a pre-patched verification file"))
   , m_original()
   , m_verification()
   , m_checkEdc(false)
   , m_checkEcc(false)
{
   m_cmd->require_option(2, 4);

   m_cmd->add_option(
      "--original",
      m_original,
      "The original file to patch. Requires live patching be configured for "
      "this file.");

   m_cmd->add_option(
      "--verification",
      m_verification,
      "The pre-patched verification file to use to verify the patching "
      "result.");

   m_cmd->add_flag(
      "--check_edc",
      m_checkEdc,
      "Checks the EDC values for CD images.");

   m_cmd->add_flag(
      "--check_ecc",
      m_checkEcc,
      "Check ECC differences in CD images. Implies --check_edc");
}

bool LivePatch::Execute()
{
   if (m_cmd->count() == 0) {
      return false;
   }

   if (m_checkEcc) {
      m_checkEdc = true;
   }

   VerifyPaths();
   DoTest();
   return true;
}

void LivePatch::VerifyPaths()
{
   if (m_original.empty()) {
      throw std::runtime_error("Original file path is empty");
   }

   if (m_verification.empty()) {
      throw std::runtime_error("Verification file path is empty");
   }

   const auto origCano = fs::weakly_canonical(m_original);
   if (!fs::exists(origCano)) {
      throw std::runtime_error(origCano.string() + " doesn't exist");
   }

   const auto veriCano = fs::weakly_canonical(m_verification);
   if (!fs::exists(veriCano)) {
      throw std::runtime_error(veriCano.string() + " doesn't exist");
   }

   {
      auto ppf = origCano;
      ppf.replace_extension(tk::rompatch::exts::kPpf);

      if (!fs::exists(ppf)) {
         throw std::runtime_error("PPF not found for " + origCano.string());
      }
   }

   m_original = origCano;
   m_verification = veriCano;
}

void LivePatch::DoTest() const
{
   LoadInjector();

   std::ifstream target(m_original, std::ios::binary);
   std::ifstream verification(m_verification, std::ios::binary);

   if (m_checkEcc) {
      SimpleVerification(target, verification);
   }
   else {
      CdVerification(target, verification, m_checkEdc);
   }

   std::cout << "File data identical. Live patching is working." << std::endl;
}

}