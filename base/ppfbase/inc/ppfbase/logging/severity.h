#pragma once

#include <iosfwd>

namespace tdd::base::logging {

   enum class Severity : int
   {
      Verbose_2 = -2,
      Verbose_1 = -1,
      Debug = 0,
      Info = 1,
      Warning = 2,
      Error = 3,
      Fatal = 4,

      // Not a severity
      TurnOffLogging = 0xff
   };

}

std::ostream& operator<<(
   std::ostream& os,
   const tdd::base::logging::Severity severity);