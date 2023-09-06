#include <ppfbase/stdext/type_traits.h>

#include <doctest/doctest.h>

namespace tdd::stdext {

TEST_CASE("strong_type<T>: simple construction")
{
   static constexpr int kStartingValue = 5;
   static constexpr int kUpdatedValue = 43;

   struct TestTag {};
   using StrongInt = strong_type<int, TestTag>;

   StrongInt val(kStartingValue);

   CHECK(val.get() == kStartingValue);
   CHECK(val == StrongInt(kStartingValue));

   val.get() = kUpdatedValue;
   CHECK(val.get() == kUpdatedValue);
   CHECK(val != StrongInt(kStartingValue));

   val = StrongInt(kStartingValue);
   CHECK(val.get() == kStartingValue);
   CHECK(val == StrongInt(kStartingValue));
   CHECK(val < StrongInt(kUpdatedValue));
}

namespace CompileTimeTests {

   static_assert(std::is_same_v<int, zeroth_or_none_such_t<int>>);
   static_assert(std::is_same_v<char[5], zeroth_or_none_such_t<char[5]>>);

   static_assert(std::is_same_v<
      int,
      nth_or_none_such_t<2, long, bool, int, long, char[5]>>);
   static_assert(std::is_same_v<
      char[5],
      nth_or_none_such_t<4, long, bool, int, long, char[5]>>);

   static_assert(nth_is_v<2, int, long, bool, int, long, char[5]>);
   static_assert(nth_is_v<4, char[5], long, bool, int, long, char[5]>);

   static_assert(is_any_of_v<int, bool, strong_type<bool, bool>, int>);
   static_assert(is_none_of_v<char, bool, strong_type<bool, bool>, int>);

}

}