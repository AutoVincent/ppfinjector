#include "cmds/cli.h"

#include "launch_emulator.h"

#include <ppftk/config/app.h>

#include <ppfbase/branding.h>
#include <ppfbase/logging/logging.h>

#include <iostream>


namespace tdd::app::emulauncher {
}

int main(int argc, char** argv)
{
   tdd::base::logging::InitProcessLog(
      tdd::base::logging::SharingMode::MultiProcess);

   tdd::base::logging::SetMinLogLevel(
      tdd::tk::config::App::LogLevel());

   if (argc > 1) {
      return tdd::app::emulauncher::cmd::HandleCmdLine(argc, argv);
   }

   TDD_LOG_INFO() << "EmuLauncher started";

   tdd::app::emulauncher::LaunchEmulator();

   return 0;
}