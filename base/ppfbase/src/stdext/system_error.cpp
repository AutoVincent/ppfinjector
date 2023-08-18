#include <ppfbase/stdext/system_error.h>

#include <Windows.h>

namespace tdd::stdext {

std::error_code make_last_error() noexcept
{
   return make_win32_ec(::GetLastError());
}

}