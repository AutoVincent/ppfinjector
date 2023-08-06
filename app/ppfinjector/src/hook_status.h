#pragma once

#include <ppfbase/preprocessor_utils.h>

namespace tdd::app::ppfinjector::HookStatus {

   struct [[nodiscard]] DisableFurtherHooks
   {
      DisableFurtherHooks() noexcept;
      ~DisableFurtherHooks() noexcept;
   };

   [[nodiscard]] bool ShouldExecute() noexcept;

}

#define TDD_PPF_DISABLE_FURTHER_HOOKS() \
   const tdd::app::ppfinjector::HookStatus::DisableFurtherHooks \
      TDD_JOIN_TOKEN(disableHookScope, __LINE__)