#include <ppftk/config/app.h>

#include "app_impl.h"


namespace tdd::tk::config::App {

namespace {
   details::AppImpl& GetInstance() {
      static details::AppImpl config;
      return config;
   }
}

const base::logging::Severity LogLevel() noexcept
{
   return GetInstance().LogLevel();
}

const std::filesystem::path& Emulator() noexcept
{
   return GetInstance().Emulator();
}

void Emulator(const std::filesystem::path& emulator)
{
   GetInstance().Emulator(emulator);
}

void ClearEmulator()
{
   GetInstance().ClearEmulator();
}

const std::set<std::string>& TargetExts() noexcept
{
   return GetInstance().TargetExts();
}

}