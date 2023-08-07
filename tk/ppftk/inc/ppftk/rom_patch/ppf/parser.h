#pragma once

#include <ppftk/rom_patch/patch_descriptor.h>

#include <filesystem>
#include <optional>

namespace tdd::tk::rompatch::ppf {

   std::optional<PatchDescriptor> Parse(const std::filesystem::path& ppf);

}