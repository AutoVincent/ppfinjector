#pragma once

#include <fstream>
#include <mutex>

#include <Windows.h>

namespace tdd::app::ppfinjector {
   class [[nodiscard]] ReadOpsLog {
   public:
      ReadOpsLog(HANDLE hFile, const std::filesystem::path& targetFile);
      ~ReadOpsLog() = default;

      void Log(const size_t dataLength);
   private:
      HANDLE m_hFile;
      std::ofstream m_log;
      std::mutex m_lock;
   };
}
