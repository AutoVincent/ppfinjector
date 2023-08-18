#pragma once

#include <ppfbase/diagnostics/assert.h>
#include <ppfbase/logging/log_msg.h>
#include <ppfbase/logging/sharing_mode.h>
#include <ppfbase/stdext/stream_operator.h>

namespace tdd::base::logging {

   void InitSingleProcessLog();

   void InitDllLog(SharingMode mode = SharingMode::SingleProcess);

   void SetMinLogLevel(Severity severity) noexcept;
   Severity GetMinLogLevel() noexcept;
}

#define TDD_LOG_LEVEL(l) tdd::base::logging::Severity::l

#define TDD_LOG_EX(level) \
   tdd::base::logging::LogMsg(__FILE__, __LINE__, __func__, TDD_LOG_LEVEL(level))

#define TDD_LOG_IS_ON(level) \
   (TDD_LOG_LEVEL(level) >= tdd::base::logging::GetMinLogLevel())

#define TDD_LOG_LAZY_STREAM(stream, cond) \
   !(cond) ? (void) 0 : tdd::base::logging::MsgVoidify() & (stream)

#define TDD_LOG_STREAM(level) TDD_LOG_EX(level).stream()

// Logs the message if 'cond' is true
#define TDD_LOG_IF(cond, level) \
   TDD_LOG_LAZY_STREAM( \
      TDD_LOG_STREAM(level), \
      TDD_LOG_IS_ON(level) && (cond))

#define TDD_LOG(level) TDD_LOG_IF(true, level)

#define TDD_VLOG(n) TDD_LOG(TDD_LOG_LEVEL(Verbose_ ## n))

#define TDD_VLOG2()        TDD_VLOG(2)
#define TDD_VLOG1()        TDD_VLOG(1)
#define TDD_LOG_DEBUG()    TDD_LOG(Debug)
#define TDD_LOG_INFO()     TDD_LOG(Info)
#define TDD_LOG_WARN()     TDD_LOG(Warning)
#define TDD_LOG_ERROR()    TDD_LOG(Error)
#define TDD_LOG_FATAL()    TDD_LOG(Fatal)

#define TDD_LOG_DEBUG_IF(cond)    TDD_LOG_IF((cond), Debug)
#define TDD_LOG_INFO_IF(cond)     TDD_LOG_IF((cond), Info)
#define TDD_LOG_WARN_IF(cond)     TDD_LOG_IF((cond), Warning)
#define TDD_LOG_ERROR_IF(cond)    TDD_LOG_IF((cond), Error)
#define TDD_LOG_FATAL_IF(cond)    TDD_LOG_IF((cond), Fatal)

#define TDD_CHECK(cond, msg) TDD_LOG_FATAL_IF(!(cond)) << msg;

#ifdef _DEBUG
#define TDD_DCHECK TDD_CHECK
#else
#define TDD_DCHECK(...) (void)0
#endif //_DEBUG
