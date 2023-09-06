#pragma once

#include <ppfbase/preprocessor_utils.h>

#include <tuple>
#include <type_traits>

namespace tdd::stdext {
namespace details {
   // Unconstructable sentinel type to denote no such type.
   struct NoneSuch
   {
      NoneSuch() = delete;
      ~NoneSuch() = delete;
      TDD_DISABLE_COPY_MOVE(NoneSuch);
   };

   // 0-based indexing
   template <std::size_t N, bool HasNthType, typename... Ts>
   struct NthOrNoneSuch;

   template <std::size_t N, typename... Ts>
   struct NthOrNoneSuch<N, false, Ts...>
   {
      using type = NoneSuch;
   };

   template <std::size_t N, typename... Ts>
   struct NthOrNoneSuch<N, true, Ts...>
   {
      using type = std::remove_cvref_t<decltype(std::get<N>(
         std::declval<std::tuple<Ts...>>()))>;
   };
}

// clang-format off
// Variadic template type parameter pack type picker. 0-based indexing.
template <std::size_t N, typename... Ts>
using nth_or_none_such = details::NthOrNoneSuch<N, N < sizeof...(Ts), Ts...>;
// clang-format on

template <std::size_t N, typename... Ts>
using nth_or_none_such_t = typename nth_or_none_such<N, Ts...>::type;

template <typename... Ts>
using zeroth_or_none_such = nth_or_none_such<0, Ts...>;

template <typename... Ts>
using zeroth_or_none_such_t = typename zeroth_or_none_such<Ts...>::type;

template <std::size_t N, typename TypeToCheck, typename... Ts>
using nth_is = std::is_same<TypeToCheck, nth_or_none_such_t<N, Ts...>>;

template <std::size_t N, typename TypeToCheck, typename... Ts>
inline constexpr bool nth_is_v = nth_is<N, TypeToCheck, Ts...>::value;

template <typename TypeToCheck, typename... Ts>
using zeroth_is = nth_is<0, TypeToCheck, Ts...>;

template <typename TypeToCheck, typename... Ts>
inline constexpr bool zeroth_is_v = zeroth_is<TypeToCheck, Ts...>::value;

template <typename TypeToCheck, typename... Ts>
using is_any_of = std::disjunction<std::is_same<TypeToCheck, Ts>...>;

template <typename TypeToCheck, typename... Ts>
inline constexpr bool is_any_of_v = is_any_of<TypeToCheck, Ts...>::value;

template <typename TypeToCheck, typename... Ts>
inline constexpr bool is_none_of_v = !is_any_of_v<TypeToCheck, Ts...>;

// T is a value type
template <typename T>
inline constexpr bool is_value_v =
   !std::is_pointer_v<T> && !std::is_reference_v<T>;

// clang-format off
template <typename T>
struct is_value : std::bool_constant<is_value_v<T>> {};
// clang-format on

template <
   typename T,
   typename Tag,
   std::enable_if_t<!std::is_reference_v<T>, void*> = nullptr>
class [[nodiscard]] strong_type
{
public:
   using value_type = T;
   using reference_type = value_type&;
   using const_reference_type = const value_type&;

   template <
      typename... Args,
      std::enable_if_t<!zeroth_is_v<strong_type, Args...>, void*> = nullptr>
   explicit constexpr strong_type(Args... args)
      : m_val(std::forward<Args>(args)...)
   {}

   ~strong_type() = default;

   TDD_DEFAULT_COPY_MOVE(strong_type);

   [[nodiscard]] constexpr reference_type get() &
   {
      return m_val;
   }

   [[nodiscard]] constexpr value_type&& get() &&
   {
      return std::move(m_val);
   }

   [[nodiscard]] constexpr const_reference_type get() const&
   {
      return m_val;
   }

   [[nodiscard]] constexpr bool operator==(const strong_type& other) const =
      default;
   [[nodiscard]] constexpr auto operator<=>(const strong_type& other) const =
      default;

private:
   T m_val;
};

}
