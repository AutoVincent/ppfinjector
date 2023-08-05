#pragma once

#include <ppfbase/logging/ilog.h>

#include <filesystem>
#include <fstream>

namespace tdd::base::logging::details {
   class BasicLog final : public ILog
   {
   public:
      BasicLog(
         const std::filesystem::path& filepath,
         size_t maxBytesPerFile,
         size_t numberOfFilesToKeep);
      ~BasicLog();

      // ILog
      const std::filesystem::path& path() const noexcept override { return m_file; }
      void Write(const char* msg) override;
   private:
      void RotateAsRequired();

      const std::filesystem::directory_entry m_file;
      const size_t m_maxBytesPerFile;
      const size_t m_archiveSize;
      std::ofstream m_os;
   };
}