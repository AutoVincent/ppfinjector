#pragma once

#include <ppftk/rom_patch/patch_item.h>

#include <ppfbase/preprocessor_utils.h>

#include <vector>

namespace tdd::tk::rompatch {

   class [[nodiscard]] FlatPatch
   {
   public:
      using ValidationData = PatchItem;

      TDD_DEFAULT_ALL_SPECIAL_MEMBERS(FlatPatch);


   private:
      ValidationData m_validationData;
      DataBuffer m_patch;
   };

}