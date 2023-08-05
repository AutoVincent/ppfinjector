#pragma once

#include <filesystem>
#include <string>

namespace tdd::base::logging {

   class ILog
   {
   public:
      ILog() = default;
      virtual ~ILog() = default;

      virtual const std::filesystem::path& path() const noexcept = 0;
      virtual void Write(const char* msg) = 0;

      inline static constexpr auto kExt = L".log";
   };

   class NullLog final : public ILog
   {
   public:
      NullLog() = default;
      virtual ~NullLog() = default;

      virtual const std::filesystem::path& path() const noexcept override { return m_empty; }
      virtual void Write(const char*) override {}
   private:
      const std::filesystem::path m_empty;
   };
}