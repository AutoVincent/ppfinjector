#include "app_impl.h"

#include <ppfbase/logging/logging.h>
#include <ppfbase/process/this_module.h>
#include <ppfbase/process/this_process.h>
#include <ppfbase/stdext/string.h>

#include <json/json.h>

#include <fstream>

namespace tdd::tk::config::details {

namespace {
   namespace fs = std::filesystem;

   namespace schema {
      static constexpr auto kConfigFileName = L"ppfinjector.json";
      static const std::set<std::string> kDefaultExts{
         ".bin"
      };

      // fs::path. Absolute path.
      static constexpr auto kEmulator = "emulator";
      // int.
      static constexpr auto kLogLevel = "log_level";
      // array of strings
      static constexpr auto kTargetExts = "target_extensions";
   }

   [[nodiscard]] fs::path ConfigPath()
   {
      // This is equivalent to ThisProcess::ImagePath() when the calling
      // module is the exe.
      const auto dllPath = base::process::ThisModule::ImagePath();

      return dllPath.parent_path() / schema::kConfigFileName;
   }

   [[nodiscard]] bool WriterIsExe()
   {
      const auto exePath = base::process::ThisProcess::ImagePath();
      const auto dllPath = base::process::ThisModule::ImagePath();

      TDD_DCHECK(
         exePath == dllPath,
         "Only the launcher process should writing the config file");

      return exePath == dllPath;
   }

   Json::Value ToJson(const AppImpl& config)
   {
      Json::Value exts(Json::arrayValue);
      for (const auto& e : config.TargetExts()) {
         exts.append(e);
      }

      Json::Value json;
      if (!config.Emulator().empty()) {
         json[schema::kEmulator] = stdext::WideToUtf8(config.Emulator().wstring());
      }

      json[schema::kLogLevel] = static_cast<int>(config.LogLevel());
      json[schema::kTargetExts] = exts;
      return json;
   }

   void WriteConfig(const AppImpl& config)
   {
      if (!WriterIsExe()) {
         return;
      }

      std::ofstream out(ConfigPath(), std::ios::trunc);

      Json::StreamWriterBuilder writer;
      writer["indentation"] = "  ";
      out << Json::writeString(writer, ToJson(config)) << std::endl;
      out.close();
   }

   Json::Value ParseConfig()
   {
      const auto configPath = ConfigPath();
      if (!fs::exists(configPath)) {
         return Json::nullValue;
      }

      std::ifstream in(configPath);

      Json::Value config;
      Json::CharReaderBuilder builder;
      std::string errs;
      if (!Json::parseFromStream(builder, in, &config, &errs)) {
         TDD_LOG_ERROR() << "Unable to parse [" << configPath.wstring() << "]: "
            << errs;
         return Json::nullValue;
      }

      return config;
   }
}

AppImpl::AppImpl()
   : m_emulator()
   , m_targetExts(schema::kDefaultExts)
   , m_logLevel(base::logging::Severity::Info)
{
   try {
      Load();
   }
   catch (const std::exception& e) {
      TDD_LOG_ERROR() << "Unable to load config: " << e.what();
   }
}

const base::logging::Severity AppImpl::LogLevel() const noexcept
{
   return m_logLevel;
}

const std::filesystem::path& AppImpl::Emulator() const noexcept
{
   return m_emulator;
}

void AppImpl::Emulator(const std::filesystem::path& emulator)
{
   TDD_CHECK(emulator.is_absolute(), "Emulator path should be absolute");
   m_emulator = emulator;
   WriteConfig(*this);
}

void AppImpl::ClearEmulator()
{
   m_emulator.clear();
   WriteConfig(*this);
}

const std::set<std::string>& AppImpl::TargetExts() const noexcept
{
   return m_targetExts;
}

void AppImpl::Load()
{
   const auto config = ParseConfig();
   if (config.isNull()) {
      return;
   }

   const auto emulator = config.get(schema::kEmulator, "").asString();
   if (!emulator.empty()) {
      fs::path emuPath(stdext::Utf8ToWide(emulator));
      if (emuPath.is_absolute() && emuPath.extension() == L".exe") {
         m_emulator = std::move(emuPath);
      }
      else {
         TDD_LOG_ERROR() << "Invalid emulator path: [" << emuPath << "]";
      }
   }

   if (config.isMember(schema::kLogLevel)) {
      m_logLevel = static_cast<base::logging::Severity>(
         config[schema::kLogLevel].asInt());
   }

   if (config.isMember(schema::kTargetExts)) {
      const auto extsArray = config[schema::kTargetExts];
      for (const auto& item : extsArray) {
         auto ext = item.asString();
         if (ext.starts_with(".")) {
            m_targetExts.insert(std::move(ext));
         }
      }
   }
}



}