#include <ppfbase/logging/logging.h>

#include "logger.h"

namespace tdd::base::logging {

   void InitProcessLog(SharingMode mode)
   {
      details::Logger::InitProcessLog(mode);
   }

   void InitDllLog()
   {
      details::Logger::InitDllLog();
   }

   void InitDllLog(const std::filesystem::path& sharedLog)
   {
      details::Logger::InitDllLog(sharedLog);
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