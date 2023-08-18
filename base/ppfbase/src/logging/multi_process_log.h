#pragma once

#include "basic_log.h"

#include <ppfbase/preprocessor_utils.h>
#include <ppfbase/logging/ilog.h>
#include <ppfbase/stdext/mutex.h>

#include <mutex>

namespace tdd::base::logging::details {

   inline std::wstring GenerateLockName(const std::filesystem::path& logFile)
   {
      static constexpr auto kLockSuffix = L"-TddLogLock";
      auto lockStem = logFile.stem().wstring();
      lockStem.append(kLockSuffix);
      return lockStem;
   }

   template <typename LogT>
   class [[nodiscard]] MultiProcessDecorator final : public ILog
   {
   public:
      MultiProcessDecorator(
         const std::filesystem::path& filepath,
         size_t maxBytesPerFile,
         size_t numberOfFilesToKeep)
         : m_lock(GenerateLockName(filepath))
         , m_log(filepath, maxBytesPerFile, numberOfFilesToKeep)
      {}

      ~MultiProcessDecorator() override = default;

      const std::filesystem::path& path() const noexcept override
      {
         return m_log.path();
      }

      void Write(const char* msg) override
      {
         std::lock_guard l(m_lock);
         m_log.Write(msg);
      }

   private:
      stdext::mp_mutex m_lock;
      LogT m_log;

      TDD_DISABLE_COPY_MOVE(MultiProcessDecorator);
   };

   using MultiProcessLog = MultiProcessDecorator<BasicLog>;

}