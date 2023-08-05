#include <ppfbase/logging/log_msg.h>

#include "logger.h"

#include <ppfbase/chrono/timestamp.h>
#include <ppfbase/diagnostics/debugger.h>
#include <ppfbase/process/this_process.h>
#include <ppfbase/stdext/stream_operator.h>

#include <Windows.h>

namespace tdd::base::logging {

namespace {
   std::string PidTidTag()
   {
      std::ostringstream ss;
      ss << std::setfill('0')
         << 'P' << std::setw(5) << ::GetCurrentProcessId()
         << 'T' << std::setw(5) << ::GetCurrentThreadId();
      return ss.str();
   }
}

LogMsg::LogMsg(const char* file, int line, const char* func, Severity severity)
   : m_os()
   , m_severity(severity)
{
   const std::filesystem::path src(file);

   m_os << chrono::TimeStamp::Now() << " "
      << PidTidTag() << " "
      << std::setw(4) << std::setfill(' ') << m_severity << " "
      << process::ThisProcess::Name().wstring() << " "
      << src.filename().wstring() << "<"
      << line << ">:"
      << func << "(): ";
}

LogMsg::~LogMsg()
{
   m_os << std::endl;
   details::Logger::Write(m_os.str().c_str());

   if (m_severity >= Severity::Fatal) {
      diagnostics::Debugger::Break();
   }
}

std::ostream& LogMsg::stream() noexcept
{
   return m_os;
}

}