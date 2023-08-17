#include "cli.h"

#include "handlers.h"

#include <ppfbase/logging/logging.h>

#include <iostream>
#include <vector>

namespace tdd::app::emulauncher::cmd {

namespace {
   std::vector<SubCmdPtr> g_cmds;

   void RegisterSubCmds(CLI::App& emulauncher)
   {
      g_cmds.push_back(AttachTest(emulauncher));
   }

   int ExecuteSubCmds()
   {
      for (const auto& cmd : g_cmds) {
         if (cmd->Execute()) {
            return 0;
         }
      }

      TDD_ASSERT(false);
      return 0;
   }
}

int HandleCmdLine(const int argc, const char* const* const argv)
{
   CLI::App emulauncher("PPF injector emulator launcher");
   emulauncher.set_help_all_flag("--help-all");

   RegisterSubCmds(emulauncher);

   CLI11_PARSE(emulauncher, argc, argv);

   try {
      return ExecuteSubCmds();
   }
   catch (const std::exception& e) {
      std::cout << "Action failed: " << e.what() << std::endl;
      return 1;
   }
}

}