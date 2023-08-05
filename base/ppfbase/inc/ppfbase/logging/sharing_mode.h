#pragma once

namespace tdd::base::logging {

   enum class [[nodiscard]] SharingMode
   {
      SingleProcess,
      MultiProcess // TODO add support
   };

}