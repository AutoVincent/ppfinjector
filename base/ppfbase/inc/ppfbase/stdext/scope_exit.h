#pragma once
#include <ppfbase/preprocessor_utils.h>

#include <utility>

namespace tdd::stdext {

   template <typename F>
   struct scope_exit
   {
      scope_exit(F&& f) noexcept : m_f(std::forward<F>(f)) {}
      ~scope_exit() noexcept { m_f(); }

   private:
      F m_f;

      TDD_DISABLE_COPY_MOVE(scope_exit);
   };

   template <typename F> scope_exit(F&&) -> scope_exit<F>;

   template <typename F>
   scope_exit<F> make_scope_exit(F&& f)
   {
      return scope_exit<F>(std::forward<F>(f));
   }

}

#define TDD_ON_SCOPE_EXIT(code) \
   const auto TDD_JOIN_TOKEN(autoScopeExit, __LINE__) = \
      tdd::stdext::make_scope_exit([&]() noexcept { code })