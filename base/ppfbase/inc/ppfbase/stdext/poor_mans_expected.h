#pragma once

#include <ppfbase/stdext/system_error.h>
#include <ppfbase/preprocessor_utils.h>
#include <ppfbase/diagnostics/assert.h>

#include <variant>

namespace tdd::stdext {

   template <typename T>
   class [[nodiscard]] pm_expected
   {
   public:
      template <
         typename U = T,
         std::enable_if_t<std::is_convertible_v<U, T>, void*> = nullptr>
      pm_expected(U&& value)
         noexcept(std::is_nothrow_convertible_v<decltype(m_data), U>)
         : m_data(std::forward<U>(value))
      {}

      pm_expected(std::error_code ec) noexcept
         : m_data(ec)
      {}

      [[nodiscard]] std::error_code error() const noexcept
      {
         if (has_value()) {
            return {};
         }
         
         return std::get<std::error_code>(m_data);
      }

      template <typename Fn>
      [[nodiscard]] std::error_code handle_error(Fn&& fn) const
      {
         if (has_value()) {
            return {};
         }

         return fn(error());
      }

      [[nodiscard]] bool has_value() const noexcept
      {
         return std::holds_alternative<T>(m_data);
      }

      [[nodiscard]] T const& value() const & noexcept
      {
         TDD_ASSERT(has_value());
         return std::get<T>(m_data);
      }
      
      [[nodiscard]] T& value() & noexcept
      {
         TDD_ASSERT(has_value());
         return std::get<T>(m_data);
      }

      [[nodiscard]] T&& value() && noexcept
      {
         TDD_ASSERT(has_value());
         return std::get<T>(std::move(m_data));
      }

   private:
      std::variant<std::error_code, T> m_data;
   };
}