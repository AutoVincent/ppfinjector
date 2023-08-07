#pragma once

#include <ppftk/rom_patch/patch_descriptor.h>

#include <iosfwd>
#include <optional>

namespace tdd::tk::rompatch::details::ppf::V3 {
   std::optional<PatchDescriptor> Parse(std::istream& ppf);
}