#include "read_ops_log.h"

#include <ppfbase/filesystem/path_service.h>
#include <ppfbase/logging/logging.h>
#include <ppfbase/process/this_process.h>

#include <sstream>

#include <Windows.h>

namespace tdd::app::ppfinjector {

namespace {

   std::optional<int64_t> GetFilePointer(const HANDLE hFile)
   {
      static constexpr LARGE_INTEGER kDontMoveFilePointer{ 0 };

      LARGE_INTEGER location{ 0 };
      const auto success = SetFilePointerEx(
         hFile,
         kDontMoveFilePointer,
         &location,
         FILE_CURRENT);

      if (success) {
         return location.QuadPart;
      }

      const auto err = GetLastError();
      TDD_LOG_WARN() << "Unable to get current file pointer location: "
         << std::error_code(err, std::system_category());
      return std::nullopt;
   }

}

ReadOpsLog::ReadOpsLog(HANDLE hFile, const std::filesystem::path& targetFile)
   : m_hFile(hFile)
   , m_log()
   , m_lock()
{
   static constexpr std::ios::iostate kNoException = 0;

   const auto logDir = base::fs::PathService::DllLogDirectory();
   if (!logDir.has_value()) {
      TDD_LOG_ERROR() << "Unable to get logging directory";
      return;
   }

   std::wostringstream logFileName;
   logFileName << base::process::ThisProcess::Name().stem().wstring() << "."
      << ::GetCurrentProcessId() << "."
      << targetFile.filename().wstring() << ".writeops.log";

   const auto logPath = logDir.value() / logFileName.str();
   m_log.exceptions(kNoException);
   m_log.open(logPath, std::fstream::trunc);
   if (!m_log.good()) {
      TDD_LOG_ERROR() << "Unable to open: [" << logPath.wstring() << "]";
   }
}

void ReadOpsLog::Log(const size_t dataLength)
{
   if (!m_log.good()) {
      return;
   }

   const auto filePtr = GetFilePointer(m_hFile);

   m_log <<::GetCurrentThreadId() << ":";
   if (filePtr.has_value()) {
      m_log << filePtr.value();
   }
   else {
      m_log << "XXX";
   }

   m_log << ":" << dataLength << std::endl;
}

}
