#pragma once

#include <cli11/CLI11.hpp>

#include <memory>

namespace tdd::app::emulauncher::cmd {

   class [[nodiscard]] ISubCmd
   {
   public:
      virtual ~ISubCmd() = default;
      [[nodiscard]] virtual bool Execute() = 0;
   };

   using SubCmdPtr = std::unique_ptr<ISubCmd>;

}