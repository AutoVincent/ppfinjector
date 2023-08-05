#pragma once
#include <ppfbase/logging/severity.h>

#include <ppfbase/preprocessor_utils.h>

#include <sstream>

namespace tdd::base::logging {
   class LogMsg
   {
   public:
      LogMsg(const char* file, int line, const char* func, Severity severity);
      ~LogMsg();
      std::ostream& stream() noexcept;

   private:
      std::ostringstream m_os;
      const Severity m_severity;

      TDD_DISABLE_COPY_MOVE(LogMsg);
   };

   // Google base logging magic
   // This class is used to explicitly ignore values in the conditional logging macro.
   // This avoid compiler warnings like "value computed is not used" and "statement has
   // no effect".
   struct MsgVoidify
   {
      MsgVoidify() noexcept {}
      // This has to be an operation with a precedence lower than << but higher than ?:
      void operator&(std::ostream&) noexcept {}
   };
}