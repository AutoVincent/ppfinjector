#pragma once

namespace tdd::app::emulauncher::cmd {
   [[nodiscard]] int HandleCmdLine(
      const int argc,
      const char* const* const argv);
}