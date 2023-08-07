#include <ppftk/rom_patch/ppf/parser.h>

#include "schema.h"
#include "v3.h"

#include <ppfbase/logging/logging.h>

#include <fstream>
#include <map>
#include <set>

namespace tdd::tk::rompatch::ppf {

namespace {
   namespace schema = details::ppf::schema;

   std::optional<PatchDescriptor> DoParse(const std::filesystem::path& ppf)
   {
      static constexpr auto kMagicSize = 5;
      static constexpr auto kVersionSize = 1;

      static const std::set<std::string> kMagic = {
         "PPF10",
         "PPF20",
         "PPF30"
      };

      static const std::map<schema::Encoding, decltype(&details::ppf::V3::Parse)>
         kParsers{
            { schema::Encoding::PPF3, &details::ppf::V3::Parse }
         };

      TDD_LOG_INFO() << "Parsing [" << ppf.wstring() << "]";

      std::ifstream ppfStream(ppf, std::ifstream::binary);

      std::string magic(kMagicSize, '0');
      ppfStream.read(magic.data(), magic.size());

      if (kMagic.count(magic) == 0) {
         TDD_LOG_INFO() << "Not a PPF file";
         return std::nullopt;
      }
      
      TDD_LOG_INFO() << "PPF-Magic: " << magic;

      schema::Encoding encoding = schema::Encoding::PPF1;
      ppfStream.read(reinterpret_cast<char*>(&encoding), sizeof(encoding));

      TDD_LOG_INFO() << "PPF encoding: " << static_cast<int>(encoding);

      const auto parser = kParsers.find(encoding);
      if (kParsers.end() != parser) {
         return parser->second(ppfStream);
      }

      TDD_LOG_INFO() << "Unsupport encoding";
      return std::nullopt;
   }
}

std::optional<PatchDescriptor> Parse(const std::filesystem::path& ppf)
{
   TDD_ASSERT(ppf.is_absolute());

   try {
      return DoParse(ppf);
   }
   catch (const std::exception& e) {
      TDD_LOG_ERROR() << "Unable to parse [" << ppf.wstring() << "]: "
         << e.what();
   }

   return std::nullopt;

}

}