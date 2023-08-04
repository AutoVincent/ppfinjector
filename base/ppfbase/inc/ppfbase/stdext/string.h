#pragma once

#include <string>

namespace tdd::ppf::stdext {

   template <typename CharT>
   void StripTrailingNulls(std::basic_string<CharT>& s)
   {
      while (!s.empty() && s.back() == 0) {
         s.pop_back();
      }
   }

}