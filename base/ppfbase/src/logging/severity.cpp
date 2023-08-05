#include <ppfbase/logging/severity.h>

#include <iomanip>
#include <iostream>
#include <sstream>

#define TDD_SEVERITY_CASE(x, txt) \
   case tdd::base::logging::Severity::x: \
      label = txt; \
      break

std::ostream& operator<<(
   std::ostream& os,
   const tdd::base::logging::Severity severity)
{
   const char* label = " UNK";
   std::string vlog;
   
   const auto intLevel = static_cast<int>(severity);
   if (intLevel < 0) {
      std::ostringstream ss;
      ss << "V_"<< -intLevel;
      vlog = ss.str();
      label = vlog.c_str();
   }
   else 
   {
      switch (severity) {
         TDD_SEVERITY_CASE(Debug, "DBG");
         TDD_SEVERITY_CASE(Info, "INFO");
         TDD_SEVERITY_CASE(Warning, "WARN");
         TDD_SEVERITY_CASE(Error, "ERR");
         TDD_SEVERITY_CASE(Fatal, "FATL");
      default:
         break;
      }
   }

   return os << label;
}
