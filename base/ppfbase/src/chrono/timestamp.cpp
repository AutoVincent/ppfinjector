#include <ppfbase/chrono/timestamp.h>

#include <chrono>
#include <iomanip>
#include <sstream>

#include <Windows.h>

namespace tdd::base::chrono::TimeStamp {

namespace {
   void AddProcessUptime(std::ostream& os)
   {
      static const auto kStartTime = ::GetTickCount64();

      static constexpr auto kSecInMs = 1000ull;
      static constexpr auto kMinInMs = 60 * kSecInMs;
      static constexpr auto kHrInMs = 60 * kMinInMs;

      const auto uptime = ::GetTickCount64() - kStartTime;
      const auto ms = uptime % kSecInMs;
      const auto sec = uptime % kMinInMs / kSecInMs;
      const auto min = uptime % kHrInMs / kMinInMs;
      const auto hr = uptime / kHrInMs;

      os << '[' << std::setw(2) << hr
         << ':' << std::setw(2) << min
         << ':' << std::setw(2) << sec
         << '.' << std::setw(3) << ms << ']';
   }

   void AddTimezoneBias(std::ostream& os)
   {
      TIME_ZONE_INFORMATION tz = { 0 };
      const auto res = ::GetTimeZoneInformation(&tz);
      if (TIME_ZONE_ID_INVALID == res) {
         os << "+XX:XX";
         return;
      }

      const auto bias = tz.Bias + tz.DaylightBias;
      const auto absBias = std::abs(bias);
      os << (bias < 0 ? '-' : '+')
         << std::setw(2) << absBias / 60 << ':'
         << std::setw(2) << absBias % 60;
   }
}

std::string Now()
{
   SYSTEMTIME now = { 0 };
   ::GetLocalTime(&now);
   std::ostringstream ss;
   ss << std::setfill('0')
      << now.wYear << '-'
      << std::setw(2) << now.wMonth << '-'
      << std::setw(2) << now.wDay << '@'
      << std::setw(2) << now.wHour << ':'
      << std::setw(2) << now.wMinute << ':'
      << std::setw(2) << now.wSecond << '.'
      << std::setw(2) << now.wMilliseconds;
   AddTimezoneBias(ss);
   AddProcessUptime(ss);
   return ss.str();

}

std::wstring FilenameSuffix()
{
   const auto now = std::chrono::system_clock::to_time_t(
      std::chrono::system_clock::now());

   std::tm tp = { 0 };
   ::gmtime_s(&tp, &now);

   std::wostringstream ss;
   ss << std::setfill(L'0') << '.'
      << (tp.tm_year + 1900)
      << std::setw(2) << tp.tm_mon + 1
      << std::setw(2) << tp.tm_mday << '-'
      << std::setw(2) << tp.tm_hour
      << std::setw(2) << tp.tm_min
      << std::setw(2) << tp.tm_sec;
   return ss.str();
}
}