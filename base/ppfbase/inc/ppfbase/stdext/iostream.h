#pragma once

#include <iostream>

namespace tdd::stdext {
   
   template <
      typename T,
      std::enable_if_t<std::is_trivial_v<std::remove_cvref_t<T>>, void*> = nullptr>
   void Read(std::istream& is, T& value)
   {
      is.read(reinterpret_cast<char*>(&value), sizeof(T));
   }

   template <
      typename ContainerT,
      std::enable_if_t<!std::is_trivial_v<std::remove_cvref_t<ContainerT>>, void*> = nullptr>
   void Read(std::istream& is, ContainerT& value)
   {
      is.read(
         reinterpret_cast<char*>(value.data()),
         sizeof(typename ContainerT::value_type) * value.size());
   }

   template <
      typename T,
      std::enable_if_t<std::is_trivial_v<std::remove_cvref_t<T>>, void*> = nullptr>
   void Write(std::ostream& os, const T& value)
   {
      os.write(reinterpret_cast<const char*>(&value), sizeof(T));
   }

   template <
      typename ContainerT,
      std::enable_if_t<!std::is_trivial_v<std::remove_cvref_t<ContainerT>>, void*> = nullptr>
   void Write(std::ostream& os, const ContainerT& value)
   {
      os.write(
         reinterpret_cast<const char*>(value.data()),
         sizeof(typename ContainerT::value_type) * value.size());
   }

}