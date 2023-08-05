#include <ppfbase/stdext/stream_operator.h>

#include <ppfbase/stdext/string.h>

#include <iostream>

std::ostream& operator<<(std::ostream& os, const std::wstring& ws)
{
   return os << tdd::stdext::WideToUtf8(ws);
}
