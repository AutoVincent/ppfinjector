#pragma once

#include "../sub_cmd.h"

#include <gsl/pointers>

#include <filesystem>

namespace tdd::app::emulauncher::cmd::test {

   class [[nodiscard]] LivePatch : public ISubCmd
   {
   public:
      LivePatch(CLI::App& test);
      ~LivePatch() override = default;
      bool Execute() override;
   private:
      void VerifyPaths();
      void DoTest();

      gsl::not_null<CLI::App*> m_cmd;
      std::filesystem::path m_original;
      std::filesystem::path m_verification;
   };

}