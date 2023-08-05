#pragma once

#include <iosfwd>
#include <string>

std::ostream& operator<<(std::ostream& os, const std::wstring& ws);

inline std::ostream& operator<<(std::ostream& os, const wchar_t* ws)
{
   return os << std::wstring(ws);
}

inline std::ostream& operator<<(std::ostream& os, std::wstring_view ws)
{
   return os << std::wstring(ws);
}