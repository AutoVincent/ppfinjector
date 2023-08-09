#pragma once

#include <ppftk/rom_patch/ppf/ppf3.h>

#include <filesystem>
#include <optional>

namespace tdd::tk::rompatch::ppf {

   std::optional<Ppf3> Parse(const std::filesystem::path& ppf);

}