#pragma once

#include <ppfbase/logging/severity.h>
#include <ppfbase/logging/sharing_mode.h>

#include <filesystem>

namespace tdd::base::logging::details::Logger {

   void InitProcessLog(SharingMode shareMode);

   void InitDllLog();
   void InitDllLog(const std::filesystem::path& sharedLog);

   void SetMinLogLevel(Severity severity) noexcept;
   Severity GetMinLogLevel() noexcept;

   void Write(const char* msg);
}