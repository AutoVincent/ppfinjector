#include "handlers.h"

#include "../launch_emulator.h"

#include <ppftk/config/app.h>

#include <gsl/pointers>

namespace tdd::app::emulauncher::cmd {

namespace {
   class [[nodiscard]] ChooseEmulator : public ISubCmd
   {
   public:
      ChooseEmulator(CLI::App& emulauncher)
         : m_cmd(emulauncher.add_subcommand(
              "choose_emulator",
              "Choose a new emulator to launch instead of the one stored in "
              "configuration"))
      {}

      ~ChooseEmulator() override = default;

      bool Execute() override
      {
         if (m_cmd->count() == 0) {
            return false;
         }

         tk::config::App::ClearEmulator();
         LaunchEmulator();
         return true;
      }

   private:
      gsl::not_null<CLI::App*> m_cmd;
   };
}

SubCmdPtr AttachChooseEmulator(CLI::App& emulauncher)
{
   return std::make_unique<ChooseEmulator>(emulauncher);
}

}
