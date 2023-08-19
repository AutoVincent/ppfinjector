#include <ppftk/config/patch.h>

#include <ppftk/rom_patch/patch_file_exts.h>

#include <ppfbase/logging/logging.h>
#include <ppfbase/stdext/poor_mans_expected.h>

#include <json/json.h>

#include <fstream>

#include <Windows.h>

namespace tdd::tk::config {

namespace {
   namespace fs = std::filesystem;

   namespace schema {
      constexpr auto kPatch = "patch";
      constexpr auto kCalculateEdc = "calculate_edc";
   }

   stdext::pm_expected<Json::Value> ParseConfig(
      const std::filesystem::path& config)
   {
      if (!fs::exists(config)) {
         return stdext::make_win32_ec(ERROR_FILE_NOT_FOUND);
      }

      std::ifstream configStream(config);

      Json::Value confJson;
      Json::CharReaderBuilder builder;
      std::string errs;
      if (!Json::parseFromStream(builder, configStream, &confJson, &errs)) {
         TDD_LOG_WARN() << "Unable to parse: [" << config.wstring() << "]: "
            << errs;
         return stdext::make_win32_ec(ERROR_FILE_NOT_SUPPORTED);
      }

      if (!confJson.isMember(schema::kPatch)) {
         TDD_LOG_WARN() << "Missing [" << schema::kPatch << "]";
         return stdext::make_win32_ec(ERROR_INVALID_PARAMETER);
      }

      return std::move(confJson);
   }

   std::tuple<fs::path, bool> ParseConfig(
      const fs::path& target,
      const Json::Value& json)
   {
      fs::path patchFile(json.get(schema::kPatch, {}).asString());
      if (patchFile.empty()) {
         TDD_LOG_WARN() << "Invalid patch path: [" << patchFile.wstring() << "]";
         return {};
      }

      if (patchFile.is_relative()) {
         patchFile = target.parent_path() / patchFile;
      }

      std::error_code ec;
      patchFile = fs::canonical(patchFile, ec);
      if (ec) {
         TDD_LOG_WARN() << "Error locating patch file [" << patchFile.wstring()
            << "]: " << ec.message();
         return {};
      }

      const auto calculateEdc = json.get(schema::kCalculateEdc, false).asBool();
      return {std::move(patchFile), calculateEdc};
   }

   std::tuple<fs::path, bool> BuildConfig(const fs::path& target)
   {
      static constexpr auto kConfigExt = L".piconf";

      auto configPath = target;
      configPath.replace_extension(kConfigExt);
      const auto config = ParseConfig(configPath);

      if (config.has_value()) {
         return ParseConfig(target, config.value());
      }

      auto ppfPath = target;
      ppfPath.replace_extension(rompatch::exts::kPpf);
      if (!fs::exists(ppfPath)) {
         return {};
      }

      return {std::move(ppfPath), true};
   }
}

Patch::Patch(const std::filesystem::path& target)
   : m_patch()
   , m_calculateEdc(true)
{
   std::tie(m_patch, m_calculateEdc) = BuildConfig(target);
}

const std::filesystem::path& Patch::PatchFile() const noexcept
{
   return m_patch;
}

bool Patch::CalculateEdc() const noexcept
{
   return m_calculateEdc;
}


}