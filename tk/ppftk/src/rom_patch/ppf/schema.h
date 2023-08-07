#pragma once

namespace tdd::tk::rompatch::details::ppf::schema {

   enum class [[nodiscard]] Encoding : char
   {
      PPF1 = 0,
      PPF2 = 1,
      PPF3 = 2
   };

   enum class [[nodiscard]] TargetImageType : char
   {
      Any = 0,
      PrimoDvd = 1
   };
}