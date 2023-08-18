#include "hook_status.h"

#include <ppfbase/logging/logging.h>

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
   TDD_CHECK(--g_nesting >= 0, "Nesting count mismatch");
}

bool ShouldExecute() noexcept
{
   return g_nesting == 0;
}

}
