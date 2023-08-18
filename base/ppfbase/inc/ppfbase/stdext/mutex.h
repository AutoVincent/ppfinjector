#pragma once

#include <ppfbase/preprocessor_utils.h>

#include <ppfbase/stdext/poor_mans_expected.h>

#include <string>

namespace tdd::stdext {
   // Multi-process mutex
   class [[nodiscard]] mp_mutex
   {
   public:
      using native_handle_type = void*;

      mp_mutex(std::wstring_view name);
      mp_mutex(std::wstring&& name);
      ~mp_mutex() noexcept;

      TDD_DEFAULT_MOVE(mp_mutex);

      void lock();
      [[nodiscard]] bool try_lock();
      void unlock();

      [[nodiscard]] native_handle_type native_handle() const noexcept;

      std::wstring_view name() const noexcept;

   private:
      std::wstring m_name;
      native_handle_type m_hMutex;

      TDD_DISABLE_COPY(mp_mutex);
   };
}