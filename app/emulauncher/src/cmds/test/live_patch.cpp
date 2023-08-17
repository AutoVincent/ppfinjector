#include "live_patch.h"

#include <ppfbase/branding.h>
#include <ppfbase/logging/logging.h>
#include <ppfbase/process/this_process.h>
#include <ppfbase/stdext/iostream.h>

#include <fstream>

#include <Windows.h>

namespace tdd::app::emulauncher::cmd::test {

namespace {
   namespace fs = std::filesystem;

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
}

LivePatch::LivePatch(CLI::App& test)
   : m_cmd(test.add_subcommand(
      "live_patch",
      "Verify the live patching logic by patching the original file on the fly "
      "and compare the data with a pre-patched verification file"))
   , m_original()
   , m_verification()
{
   m_cmd->require_option(2);

   m_cmd->add_option(
      "--original",
      m_original,
      "The original file to patch. Requires live patching be configured for "
      "this file.");

   m_cmd->add_option(
      "--verification",
      m_verification,
      "The pre-patched verification file to use to verify the patching result.");
}

bool LivePatch::Execute()
{
   if (m_cmd->count() == 0) {
      return false;
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
      ppf.replace_extension(".ppf");

      if (!fs::exists(ppf)) {
         throw std::runtime_error("PPF not found for " + origCano.string());
      }
   }

   m_original = origCano;
   m_verification = veriCano;
}

void LivePatch::DoTest()
{
   // 8KB verification chunk size.
   static constexpr size_t kChunkSize = 8 * 1024;

   LoadInjector();

   std::ifstream target(m_original, std::ios::binary);
   std::ifstream verification(m_verification, std::ios::binary);

   size_t left = 0;
   {
      target.seekg(0, std::ios::end);
      const size_t targetSize = target.tellg();

      verification.seekg(0, std::ios::end);
      left = verification.tellg();
      if (targetSize != left) {
         throw std::runtime_error("Original and verification have different sizes");
      }

      target.seekg(0, std::ios::beg);
      verification.seekg(0, std::ios::beg);
   }

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

   std::cout << "File data identical. Live patching is working." << std::endl;
}

}