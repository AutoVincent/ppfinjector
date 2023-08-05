#include <ppfbase/logging/logging.h>

#include "logger.h"

namespace tdd::base::logging {

   void InitSingleProcessLog()
   {
      details::Logger::InitSingleProcessLog();
   }

   void InitDllLog(SharingMode mode)
   {
      details::Logger::InitDllLog(mode);
   }

   void SetMinLogLevel(Severity severity) noexcept
   {
      details::Logger::SetMinLogLevel(severity);
   }

   Severity GetMinLogLevel() noexcept
   {
      return details::Logger::GetMinLogLevel();
   }
}