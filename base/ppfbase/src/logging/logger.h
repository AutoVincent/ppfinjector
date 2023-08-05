#pragma once

#include <ppfbase/logging/severity.h>
#include <ppfbase/logging/sharing_mode.h>


namespace tdd::base::logging::details::Logger {

   void InitSingleProcessLog();

   void InitDllLog(SharingMode shareMode);

   void SetMinLogLevel(Severity severity) noexcept;
   Severity GetMinLogLevel() noexcept;

   void Write(const char* msg);
}