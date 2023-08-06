#include "hook_status.h"

#include <ppfbase/diagnostics/assert.h>

namespace tdd::app::ppfinjector::HookStatus {

namespace {
   thread_local int g_nesting = 0;
}

DisableFurtherHooks::DisableFurtherHooks() noexcept
{
   ++g_nesting;
}
DisableFurtherHooks::~DisableFurtherHooks() noexcept
{
   TDD_ASSERT(--g_nesting >= 0);
}

bool ShouldExecute() noexcept
{
   return g_nesting == 0;
}

}
