#pragma once

#include "basic_log.h"

#include <ppfbase/preprocessor_utils.h>
#include <ppfbase/logging/ilog.h>

#include <mutex>

namespace tdd::base::logging::details {

   template <typename LogT>
   class [[nodiscard]] SingleProcessDecorator final : public ILog
   {
   public:
      SingleProcessDecorator(
         const std::filesystem::path& filepath,
         size_t maxBytesPerFile,
         size_t numberOfFilesToKeep)
         : m_lock()
         , m_log(filepath, maxBytesPerFile, numberOfFilesToKeep)
      {}

      ~SingleProcessDecorator() override = default;

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
      std::mutex m_lock;
      LogT m_log;

      TDD_DISABLE_COPY_MOVE(SingleProcessDecorator);
   };

   using SingleProcessLog = SingleProcessDecorator<BasicLog>;

}