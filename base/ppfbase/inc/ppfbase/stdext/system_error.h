#pragma once

#include <system_error>

namespace tdd::stdext {
   inline [[nodiscard]] std::error_code make_win32_ec(uint32_t err) noexcept
   {
      return std::error_code(err, std::system_category());
   }
}