#pragma once

#include <ppftk/rom_patch/patch_descriptor.h>

#include <filesystem>

namespace tdd::tk::rompatch::ppf {

   [[nodiscard]] std::error_code WritePpf3Patch(
      const std::filesystem::path& ppf3Path,
      const PatchDescriptor& patch);

}