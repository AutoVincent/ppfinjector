#pragma once

#include <string>

namespace tdd::stdext {

   template <typename CharT>
   void StripTrailingNulls(std::basic_string<CharT>& s)
   {
      while (!s.empty() && s.back() == 0) {
         s.pop_back();
      }
   }

   std::string WideToUtf8(const std::wstring& src);
   std::wstring Utf8ToWide(const std::string& src);

}