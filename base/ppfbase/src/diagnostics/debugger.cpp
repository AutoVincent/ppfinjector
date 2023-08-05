#include <ppfbase/diagnostics/debugger.h>

#include <thread>

#include <Windows.h>

namespace tdd::base::diagnostics::Debugger {

void Break()
{
   ::DebugBreak();
}

void DbgPrint(const char* dbgMsg)
{
   if (::IsDebuggerPresent()) {
      ::OutputDebugStringA(dbgMsg);
   }
}

void WaitForDebugger(const std::chrono::seconds waitFor)
{
   static constexpr std::chrono::milliseconds kInterval(50);

   const auto startTime = std::chrono::steady_clock::now();
   while (std::chrono::steady_clock::now() - startTime < waitFor) {
      if (::IsDebuggerPresent()) {
         Break();
         break;
      }

      std::this_thread::sleep_for(kInterval);
   }
}

}