#pragma once

#include "sub_cmd.h"

namespace tdd::app::emulauncher::cmd {

   [[nodiscard]] SubCmdPtr AttachTest(CLI::App& emulauncher);

}