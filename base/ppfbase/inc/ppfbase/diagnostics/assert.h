#include <ppfbase/diagnostics/debugger.h>

#define TDD_ASSERT(cond) \
   /* C4127 conditional expression is constant */ \
   _Pragma("warning(suppress: 4127)") \
   if (!(cond)) { \
      tdd::base::diagnostics::Debugger::Break(); \
   } static_assert(true)

#ifdef _DEBUG
#define TDD_DASSERT TDD_ASSERT
#else // ^^^^ DEBUG / RELEASE vvvv
#define TDD_DASSERT(...) void(0)
#endif
