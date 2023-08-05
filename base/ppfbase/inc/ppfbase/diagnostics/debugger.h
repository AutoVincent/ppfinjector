#pragma once

#include <chrono>

namespace tdd::base::diagnostics::Debugger {
   void Break();
   void DbgPrint(const char* dbgMsg);
   void WaitForDebugger(const std::chrono::seconds waitFor);
}