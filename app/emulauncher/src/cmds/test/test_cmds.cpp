#pragma once

#include "../handlers.h"
#include "live_patch.h"

#include <gsl/pointers>

namespace tdd::app::emulauncher::cmd {

namespace {

   class [[nodiscard]] Test : public ISubCmd
   {
   public:
      Test(CLI::App& emulauncher)
         : m_cmd(emulauncher.add_subcommand(
            "test",
            "Performs tests on the injector"))
         , m_livePatching(*m_cmd)
      {
         m_cmd->require_subcommand(1);
      }

      ~Test() override = default;

      bool Execute() override
      {
         return m_livePatching.Execute();
      }

   private:
      gsl::not_null<CLI::App*> m_cmd;
      test::LivePatch m_livePatching;
   };

}

SubCmdPtr AttachTest(CLI::App& emulauncher)
{
   return std::make_unique<Test>(emulauncher);
}

}