#include "logger.h"

#include "multi_process_log.h"
#include "single_process_log.h"

#include <ppfbase/diagnostics/assert.h>
#include <ppfbase/diagnostics/debugger.h>
#include <ppfbase/filesystem/path_service.h>
#include <ppfbase/process/this_module.h>
#include <ppfbase/process/this_process.h>

#include <atomic>
#include <memory>

#include <Windows.h>

namespace tdd::base::logging::details::Logger {

namespace {
   // 10mb
   static constexpr size_t kBytesPerFile = 10 * 1024 * 1024;
   static constexpr size_t kFilesToKeep = 10;

   std::atomic<Severity> g_severity = Severity::Info;
   std::atomic<ILog*> g_log = nullptr;

   template <typename LogT>
   void InitLog(const std::filesystem::path& logPath)
   {
      try {
         g_log = std::make_unique<LogT>(
            logPath,
            kBytesPerFile,
            kFilesToKeep).release();
      }
      catch (const std::exception&) {
         g_log = std::make_unique<NullLog>().release();
      }
   }
}

void InitProcessLog(SharingMode shareMode)
{
   const auto logDir = fs::PathService::ProcessLogDirectory();
   if (!logDir.has_value()) {
      g_log = std::make_unique<NullLog>().release();
      return;
   }

   const auto logFileName = process::ThisProcess::Name().stem() += ILog::kExt;
   const auto logPath = logDir.value() / logFileName;

   if (shareMode == SharingMode::SingleProcess) {
      InitLog<SingleProcessLog>(logPath);
   }
   else {
      InitLog<MultiProcessLog>(logPath);
   }
}

void InitDllLog()
{
   if (g_log != nullptr) {
      return;
   }

   const auto logDir = fs::PathService::DllLogDirectory();
   if (!logDir.has_value()) {
      g_log = std::make_unique<NullLog>().release();
      return;
   }
   
   std::wostringstream logFileName;
   logFileName << process::ThisModule::Name().stem().wstring() << "."
      << process::ThisProcess::Name().stem().wstring() << "."
      << ::GetCurrentProcessId() << ILog::kExt;

   const auto logPath = logDir.value() / logFileName.str();
   InitLog<SingleProcessLog>(logPath);
}

void InitDllLog(const std::filesystem::path& sharedLog)
{
   const auto ec = fs::PathService::EnsureDirectoryExist(
      sharedLog.parent_path());
   if (ec) {
      g_log = std::make_unique<NullLog>().release();
      return;
   }

   InitLog<MultiProcessLog>(sharedLog);
}

void SetMinLogLevel(Severity severity) noexcept
{
   g_severity = severity;
}

Severity GetMinLogLevel() noexcept
{
   return g_severity.load();
}

void Write(const char* msg)
{
   diagnostics::Debugger::DbgPrint(msg);
   g_log.load()->Write(msg);
}

}