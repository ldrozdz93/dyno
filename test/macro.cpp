// Copyright Louis Dionne 2017
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)

#include "testing.hpp"

#include <dyno/macro.hpp>

#include <string>
#include <tuple>
#include <utility>
#include <boost/noncopyable.hpp>

template <bool isCopyConstructible>
struct dyno_concept_for_Concept {
  static auto make_type() {
    if
      constexpr(isCopyConstructible == true) {
        return ::dyno::requires(
            (::dyno::detail::prepare_string([] {
              struct tmp {
                static constexpr std::size_t size() { return sizeof("f1") - 1; }
                static constexpr char const* get() { return "f1"; }
              };
              return tmp{};
            }())) = ::dyno::method<int(int) const>,
            (::dyno::detail::prepare_string([] {
              struct tmp {
                static constexpr std::size_t size() { return sizeof("f2") - 1; }
                static constexpr char const* get() { return "f2"; }
              };
              return tmp{};
            }())) = ::dyno::method<char(std::pair<long, double>) const>,
            (::dyno::detail::prepare_string([] {
              struct tmp {
                static constexpr std::size_t size() { return sizeof("f3") - 1; }
                static constexpr char const* get() { return "f3"; }
              };
              return tmp{};
            }())) = ::dyno::method<std::tuple<int, char>(std::string const&)>,
            dyno::CopyConstructible{});
      }
    else {
      return ::dyno::requires(
          (::dyno::detail::prepare_string([] {
            struct tmp {
              static constexpr std::size_t size() { return sizeof("f1") - 1; }
              static constexpr char const* get() { return "f1"; }
            };
            return tmp{};
          }())) = ::dyno::method<int(int) const>,
          (::dyno::detail::prepare_string([] {
            struct tmp {
              static constexpr std::size_t size() { return sizeof("f2") - 1; }
              static constexpr char const* get() { return "f2"; }
            };
            return tmp{};
          }())) = ::dyno::method<char(std::pair<long, double>) const>,
          (::dyno::detail::prepare_string([] {
            struct tmp {
              static constexpr std::size_t size() { return sizeof("f3") - 1; }
              static constexpr char const* get() { return "f3"; }
            };
            return tmp{};
          }())) = ::dyno::method<std::tuple<int, char>(std::string const&)>);
    }
  }
};
template <typename StorageType = dyno::remote_storage,
          uint32_t properties_bitfield = dyno::detail::Properties::defaults>
class Concept {
  static constexpr dyno::detail::Properties prop{properties_bitfield};
  using concept_t = decltype(
      dyno_concept_for_Concept<prop.is_copy_construcible()>::make_type());
  using this_t = Concept<StorageType, properties_bitfield>;
  using poly_t = ::dyno::poly<concept_t, StorageType>;
  template <typename, uint32_t>
  friend class Concept;
  template <typename>
  struct is_a_Concept : std::false_type {};
  template <typename T>
  struct is_a_Concept<Concept<T>> : std::true_type {};
  template <typename T>
  auto construct_poly(T&& x) {
    using RawT = std::decay_t<T>;
    if
      constexpr(not is_a_Concept<RawT>{} || dyno::is_a_make<RawT>) {
        if constexpr( dyno::is_a_make<RawT> )
          return poly_t{
              ::std::forward<T>(x),
              ::dyno::make_concept_map(
                  (::dyno::detail::prepare_string([] {
                    struct tmp {
                      static constexpr std::size_t size() {
                        return sizeof("f1") - 1;
                      }
                      static constexpr char const* get() { return "f1"; }
                    };
                    return tmp{};
                  }())) = [](auto&& self, auto&&... args) -> decltype(auto) {
                    return static_cast<decltype(self)&&>(self).f1(
                        static_cast<decltype(args)&&>(args)...);
                  },
                  (::dyno::detail::prepare_string([] {
                    struct tmp {
                      static constexpr std::size_t size() {
                        return sizeof("f2") - 1;
                      }
                      static constexpr char const* get() { return "f2"; }
                    };
                    return tmp{};
                  }())) = [](auto&& self, auto&&... args) -> decltype(auto) {
                    return static_cast<decltype(self)&&>(self).f2(
                        static_cast<decltype(args)&&>(args)...);
                  },
                  (::dyno::detail::prepare_string([] {
                    struct tmp {
                      static constexpr std::size_t size() {
                        return sizeof("f3") - 1;
                      }
                      static constexpr char const* get() { return "f3"; }
                    };
                    return tmp{};
                  }())) = [](auto&& self, auto&&... args) -> decltype(auto) {
                    return static_cast<decltype(self)&&>(self).f3(
                        static_cast<decltype(args)&&>(args)...);
                  })};
        else
          return poly_t{
              ::std::forward<T>(x),
              ::dyno::make_concept_map(
                  (::dyno::detail::prepare_string([] {
                    struct tmp {
                      static constexpr std::size_t size() {
                        return sizeof("f1") - 1;
                      }
                      static constexpr char const* get() { return "f1"; }
                    };
                    return tmp{};
                  }())) = [](auto&& self, auto&&... args) -> decltype(auto) {
                    return static_cast<decltype(self)&&>(self).f1(
                        static_cast<decltype(args)&&>(args)...);
                  },
                  (::dyno::detail::prepare_string([] {
                    struct tmp {
                      static constexpr std::size_t size() {
                        return sizeof("f2") - 1;
                      }
                      static constexpr char const* get() { return "f2"; }
                    };
                    return tmp{};
                  }())) = [](auto&& self, auto&&... args) -> decltype(auto) {
                    return static_cast<decltype(self)&&>(self).f2(
                        static_cast<decltype(args)&&>(args)...);
                  },
                  (::dyno::detail::prepare_string([] {
                    struct tmp {
                      static constexpr std::size_t size() {
                        return sizeof("f3") - 1;
                      }
                      static constexpr char const* get() { return "f3"; }
                    };
                    return tmp{};
                  }())) = [](auto&& self, auto&&... args) -> decltype(auto) {
                    return static_cast<decltype(self)&&>(self).f3(
                        static_cast<decltype(args)&&>(args)...);
                  })};
      }
    if
      constexpr(is_a_Concept<RawT>{} ) {
        return poly_t{std::forward<T>(x).poly_};
      }
  }
 public:
  template <typename T>
  Concept(T&& x) : poly_{construct_poly(std::forward<T>(x))} {}
  template <typename T>
  Concept& operator=(T&& x) {
    this->~Concept();
    return *(new (static_cast<void*>(this)) Concept(std::forward<T>(x)));
  }
  template <
      typename... Args,
      typename =
          decltype(::std::declval<::boost::callable_traits::function_type_t<
                       int(int) const>>()(::std::declval<Args&&>()...)),
      typename = ::std::enable_if_t<
          !::boost::callable_traits::is_const_member<int(int) const>::value,
          char[sizeof...(Args) + 1]>>
  ::boost::callable_traits::return_type_t<int(int) const> f1(Args&&... args) {
    return poly_.virtual_((::dyno::detail::prepare_string([] {
      struct tmp {
        static constexpr std::size_t size() { return sizeof("f1") - 1; }
        static constexpr char const* get() { return "f1"; }
      };
      return tmp{};
    }())))(static_cast<Args&&>(args)...);
  }
  template <
      typename... Args,
      typename =
          decltype(::std::declval<::boost::callable_traits::function_type_t<
                       int(int) const>>()(::std::declval<Args&&>()...)),
      typename = ::std::enable_if_t<
          ::boost::callable_traits::is_const_member<int(int) const>::value,
          char[sizeof...(Args) + 1]>>
  ::boost::callable_traits::return_type_t<int(int) const> f1(
      Args&&... args) const {
    return poly_.virtual_((::dyno::detail::prepare_string([] {
      struct tmp {
        static constexpr std::size_t size() { return sizeof("f1") - 1; }
        static constexpr char const* get() { return "f1"; }
      };
      return tmp{};
    }())))(static_cast<Args&&>(args)...);
  }
  template <
      typename... Args,
      typename = decltype(
          ::std::declval<::boost::callable_traits::function_type_t<char(
              std::pair<long, double>) const>>()(::std::declval<Args&&>()...)),
      typename =
          ::std::enable_if_t<!::boost::callable_traits::is_const_member<
                                 char(std::pair<long, double>) const>::value,
                             char[sizeof...(Args) + 1]>>
  ::boost::callable_traits::return_type_t<char(std::pair<long, double>) const>
  f2(Args&&... args) {
    return poly_.virtual_((::dyno::detail::prepare_string([] {
      struct tmp {
        static constexpr std::size_t size() { return sizeof("f2") - 1; }
        static constexpr char const* get() { return "f2"; }
      };
      return tmp{};
    }())))(static_cast<Args&&>(args)...);
  }
  template <
      typename... Args,
      typename = decltype(
          ::std::declval<::boost::callable_traits::function_type_t<char(
              std::pair<long, double>) const>>()(::std::declval<Args&&>()...)),
      typename =
          ::std::enable_if_t<::boost::callable_traits::is_const_member<
                                 char(std::pair<long, double>) const>::value,
                             char[sizeof...(Args) + 1]>>
  ::boost::callable_traits::return_type_t<char(std::pair<long, double>) const>
  f2(Args&&... args) const {
    return poly_.virtual_((::dyno::detail::prepare_string([] {
      struct tmp {
        static constexpr std::size_t size() { return sizeof("f2") - 1; }
        static constexpr char const* get() { return "f2"; }
      };
      return tmp{};
    }())))(static_cast<Args&&>(args)...);
  }
  template <typename... Args,
            typename = decltype(
                ::std::declval<::boost::callable_traits::function_type_t<
                    std::tuple<int, char>(std::string const&)>>()(
                    ::std::declval<Args&&>()...)),
            typename = ::std::enable_if_t<
                !::boost::callable_traits::is_const_member<
                    std::tuple<int, char>(std::string const&)>::value,
                char[sizeof...(Args) + 1]>>
  ::boost::callable_traits::return_type_t<
      std::tuple<int, char>(std::string const&)>
  f3(Args&&... args) {
    return poly_.virtual_((::dyno::detail::prepare_string([] {
      struct tmp {
        static constexpr std::size_t size() { return sizeof("f3") - 1; }
        static constexpr char const* get() { return "f3"; }
      };
      return tmp{};
    }())))(static_cast<Args&&>(args)...);
  }
  template <typename... Args,
            typename = decltype(
                ::std::declval<::boost::callable_traits::function_type_t<
                    std::tuple<int, char>(std::string const&)>>()(
                    ::std::declval<Args&&>()...)),
            typename = ::std::enable_if_t<
                ::boost::callable_traits::is_const_member<
                    std::tuple<int, char>(std::string const&)>::value,
                char[sizeof...(Args) + 1]>>
  ::boost::callable_traits::return_type_t<
      std::tuple<int, char>(std::string const&)>
  f3(Args&&... args) const {
    return poly_.virtual_((::dyno::detail::prepare_string([] {
      struct tmp {
        static constexpr std::size_t size() { return sizeof("f3") - 1; }
        static constexpr char const* get() { return "f3"; }
      };
      return tmp{};
    }())))(static_cast<Args&&>(args)...);
  }
 private:
  ::dyno::poly<concept_t, StorageType> poly_;
}

;

//DYNO_INTERFACE(Concept,
//  (f1, int(int) const),
//  (f2, char(std::pair<long, double>) const),
//  (f3, std::tuple<int, char> (std::string const&))
//);

struct Model1 {
  int f1(int) const  { return 1; }
  char f2(std::pair<long, double>) const { return '2'; }
  std::tuple<int, char> f3(std::string const&) { return {1, '2'}; }
};

struct Model2 {
  int f1(int) const { return 91; }
  char f2(std::pair<long, double>) const { return '3'; }
  std::tuple<int, char> f3(std::string const&) { return {91, '3'}; }
};

enum
{
  ENoConstructorInvocation = 0,
  EDefaultConstructed = 1,
  ECopiedWithVTable = 1 << 4,
  EMovedWithVTable = 1 << 8
};
constexpr auto EOnlyBufferPointerMoved = ENoConstructorInvocation;
constexpr auto ESharedPointerCopied = ENoConstructorInvocation;


struct ConstructionCounter
{
    uint32_t def = 0, copy = 0, move = 0 ;
    void reset(){ *this = ConstructionCounter{}; }
    bool check(const uint32_t expected)
    {
      return expected == (def * EDefaultConstructed |
                          copy * ECopiedWithVTable |
                          move * EMovedWithVTable);
    }
};

struct CountedConstruction
{
  static inline ConstructionCounter counter{};
  CountedConstruction(){ counter.def++; }
  CountedConstruction(const CountedConstruction&){ counter.copy++; }
  CountedConstruction(CountedConstruction&&){ counter.move++; }
  ~CountedConstruction() = default;
};

struct Model3 : CountedConstruction
{
  char additionalSize[40]; // useful for local_storage tests

  int f1(int) const  { return 91; }
  char f2(std::pair<long, double>) const { return '3'; }
  std::tuple<int, char> f3(std::string const&) { return {91, '3'}; }
};

auto expectModel3Constructor = [](const auto expected_result, auto&& test) -> bool
{
  CountedConstruction::counter.reset();
  test();
  return CountedConstruction::counter.check(expected_result);
};

void remote_storage_simple_construction_tests();
void shared_remote_storage_simple_construction_tests();
void local_storage_simple_construction_tests();
void sbo_storage_simple_construction_tests();
void non_owning_storage_simple_construction_tests();
void remote_storage_convertion_tests();
void shared_remote_storage_convertion_tests();
void local_storage_convertion_tests();
void sbo_storage_convertion_tests();
void non_owning_storage_convertion_tests();
void noncopyable_interface_tests();

int main() {
  Model1 m1{};
  Concept c1{m1};
  DYNO_CHECK(c1.f1(int{}) == 1);
  DYNO_CHECK(c1.f2(std::pair<long, double>{}) == '2');
  DYNO_CHECK(c1.f3(std::string{}) == std::make_tuple(1, '2'));

  Model2 m2{};
  Concept c2{m2};
  DYNO_CHECK(c2.f1(int{}) == 91);
  DYNO_CHECK(c2.f2(std::pair<long, double>{}) == '3');
  DYNO_CHECK(c2.f3(std::string{}) == std::make_tuple(91, '3'));

  remote_storage_simple_construction_tests();
  shared_remote_storage_simple_construction_tests();
  local_storage_simple_construction_tests();
  sbo_storage_simple_construction_tests();
  non_owning_storage_simple_construction_tests();
  remote_storage_convertion_tests();
  shared_remote_storage_convertion_tests();
  local_storage_convertion_tests();
  sbo_storage_convertion_tests();
  non_owning_storage_convertion_tests();
  noncopyable_interface_tests();
}

void remote_storage_simple_construction_tests()
{
  Concept<dyno::remote_storage> r1 = Model3{};
  Concept<dyno::remote_storage> r2 = Model3{};
  Model3 m1{};

  DYNO_CHECK(expectModel3Constructor( EDefaultConstructed | EMovedWithVTable, [&]
  {
    r1 = Model3{};
  }));

  DYNO_CHECK(expectModel3Constructor( ECopiedWithVTable, [&]
  {
    r1 = r2;
  }));

  DYNO_CHECK(expectModel3Constructor( EOnlyBufferPointerMoved, [&]
  {
    r1 = std::move(r2);
  }));

  DYNO_CHECK(expectModel3Constructor( ECopiedWithVTable, [&]
  {
    r1 = m1;
  }));
}

void shared_remote_storage_simple_construction_tests()
{
  Concept<dyno::shared_remote_storage> s1 = Model3{};

  DYNO_CHECK(expectModel3Constructor( ESharedPointerCopied, [&]
  {
    Concept<dyno::shared_remote_storage> s2 = s1;
  }));

  DYNO_CHECK(expectModel3Constructor( ESharedPointerCopied, [&]
  {
    Concept<dyno::shared_remote_storage> s2 = std::move(s1);
  }));
}

void local_storage_simple_construction_tests()
{
  Concept<dyno::local_storage<sizeof(Model3)>> l1 = Model3{};

  DYNO_CHECK(expectModel3Constructor( ECopiedWithVTable, [&]
  {
    Concept<dyno::local_storage<sizeof(Model3)>> l2 = l1;
  }));

  DYNO_CHECK(expectModel3Constructor( EMovedWithVTable, [&]
  {
    Concept<dyno::local_storage<sizeof(Model3)>> l2 = std::move(l1);
  }));
}

void sbo_storage_simple_construction_tests()
{
  struct BigModel : Model3
  {
    char additional_size[100];
  };
  Concept<dyno::sbo_storage<sizeof(Model3)>> sb = Model3{};
  Concept<dyno::sbo_storage<sizeof(Model3)>> sb_heap = BigModel{};
  Concept<dyno::sbo_storage<sizeof(Model3)>> sb_stack = Model3{};

  DYNO_CHECK(expectModel3Constructor( ECopiedWithVTable, [&]
  {
    sb = sb_heap;
  }));

  DYNO_CHECK(expectModel3Constructor( EOnlyBufferPointerMoved, [&]
  {
    sb = std::move(sb_heap);
  }));

  DYNO_CHECK(expectModel3Constructor( ECopiedWithVTable, [&]
  {
    sb = sb_stack;
  }));

  DYNO_CHECK(expectModel3Constructor( EMovedWithVTable, [&]
  {
    sb = std::move(sb_stack);
  }));
}

void non_owning_storage_simple_construction_tests()
{
  Model3 m1{};

  DYNO_CHECK(expectModel3Constructor( ENoConstructorInvocation, [&]
  {
    Concept<dyno::non_owning_storage> n1 = m1;
    Concept<dyno::non_owning_storage> n2 = n1;
    n2 = n1;
    n2 = std::move(n1);
    n2 = m1;
  }));
}

void remote_storage_convertion_tests()
{
  Concept<dyno::remote_storage> r1 = Model3{};
  Concept<dyno::local_storage<sizeof(Model3)>> l1 = Model3{};
  Concept<dyno::sbo_storage<sizeof(Model3)>> sb1_stack = Model3{};
  Concept<dyno::sbo_storage<sizeof(Model3) / 2>> sb1_heap = Model3{};
  Concept<dyno::shared_remote_storage> sr1 = Model3{};

  DYNO_CHECK(expectModel3Constructor( ECopiedWithVTable, [&]
  {
    r1 = l1;
  }));

  DYNO_CHECK(expectModel3Constructor( EMovedWithVTable, [&]
  {
    r1 = std::move(l1);
  }));

  DYNO_CHECK(expectModel3Constructor( ECopiedWithVTable, [&]
  {
    r1 = sb1_stack;
  }));

  DYNO_CHECK(expectModel3Constructor( EMovedWithVTable, [&]
  {
    r1 = std::move(sb1_stack);
  }));

  DYNO_CHECK(expectModel3Constructor( EOnlyBufferPointerMoved, [&]
  {
    r1 = std::move(sb1_heap);
  }));

  DYNO_CHECK(expectModel3Constructor( ECopiedWithVTable, [&]
  {
    r1 = sr1;
  }));

// TODO: find a better way to test this...
// r1 = std::move(sr1); // moving shared_remote_storage -> remote_storage should't compile!
}

void shared_remote_storage_convertion_tests()
{
  Concept<dyno::shared_remote_storage> sr1 = Model3{};
  Concept<dyno::local_storage<sizeof(Model3)>> l1 = Model3{};
  Concept<dyno::sbo_storage<sizeof(Model3)>> sb1_stack = Model3{};
  Concept<dyno::sbo_storage<sizeof(Model3) / 2>> sb1_heap = Model3{};
  Concept<dyno::remote_storage> r1 = Model3{};

  DYNO_CHECK(expectModel3Constructor( ECopiedWithVTable, [&]
  {
    sr1 = r1;
  }));

  DYNO_CHECK(expectModel3Constructor( EOnlyBufferPointerMoved, [&]
  {
    sr1 = std::move(r1);
  }));

  DYNO_CHECK(expectModel3Constructor( ECopiedWithVTable, [&]
  {
    sr1 = l1;
  }));

  DYNO_CHECK(expectModel3Constructor( EMovedWithVTable, [&]
  {
    sr1 = std::move(l1);
  }));

  DYNO_CHECK(expectModel3Constructor( ECopiedWithVTable, [&]
  {
    sr1 = sb1_stack;
  }));

  DYNO_CHECK(expectModel3Constructor( EMovedWithVTable, [&]
  {
    sr1 = std::move(sb1_stack);
  }));

  DYNO_CHECK(expectModel3Constructor( EOnlyBufferPointerMoved, [&]
  {
    sr1 = std::move(sb1_heap);
  }));
}

void sbo_storage_convertion_tests()
{
  Concept<dyno::sbo_storage<sizeof(Model3)>> sb1_stack = Model3{};
  Concept<dyno::sbo_storage<sizeof(Model3) / 2>> sb1_heap = Model3{};
  Concept<dyno::shared_remote_storage> sr1 = Model3{};
  Concept<dyno::local_storage<sizeof(Model3)>> l1 = Model3{};
  Concept<dyno::remote_storage> r1 = Model3{};
  Concept<dyno::remote_storage> r2 = Model3{};

  DYNO_CHECK(expectModel3Constructor( ECopiedWithVTable, [&]
  {
    sb1_stack = l1;
  }));

  DYNO_CHECK(expectModel3Constructor( ECopiedWithVTable, [&]
  {
    sb1_heap = l1;
  }));

  DYNO_CHECK(expectModel3Constructor( ECopiedWithVTable, [&]
  {
    sb1_heap = r1;
  }));

  DYNO_CHECK(expectModel3Constructor( EOnlyBufferPointerMoved, [&]
  {
    sb1_heap = std::move(r1);
  }));

  DYNO_CHECK(expectModel3Constructor( EMovedWithVTable, [&]
  {
    sb1_stack = std::move(r2);
  }));

  DYNO_CHECK(expectModel3Constructor( ECopiedWithVTable, [&]
  {
    sb1_heap = sr1;
  }));

  DYNO_CHECK(expectModel3Constructor( EMovedWithVTable, [&]
  {
    sb1_stack = std::move(sb1_heap);
  }));

  DYNO_CHECK(expectModel3Constructor( EMovedWithVTable, [&]
  {
    sb1_heap = std::move(sb1_stack);
  }));

  // TODO: find way to test this...
  // sb1_heap = std::move(sr1); // moving shared_remote_storage -> sbo_storage should't compile!
}

void local_storage_convertion_tests()
{
  constexpr auto model_size = sizeof(Model3);
  Concept<dyno::local_storage<model_size>> l1 = Model3{};
  Concept<dyno::local_storage<model_size * 2>> l_big = Model3{};
  Concept<dyno::remote_storage> r1 = Model3{};

  DYNO_CHECK(expectModel3Constructor( ECopiedWithVTable, [&]
  {
    l_big = l1;
  }));

  DYNO_CHECK(expectModel3Constructor( ECopiedWithVTable, [&]
  {
    l_big = l1;
  }));

  DYNO_CHECK(expectModel3Constructor( EMovedWithVTable, [&]
  {
    l_big = std::move(l1);
  }));

// TODO: find way to test this...
// l1 = l_big; // constructing a local_storage from a bigger local_storage should't compile!

  DYNO_CHECK(expectModel3Constructor( ECopiedWithVTable, [&]
  {
    l1 = r1;
  }));

  DYNO_CHECK(expectModel3Constructor( EMovedWithVTable, [&]
  {
    l1 = std::move(r1);
  }));
}

void non_owning_storage_convertion_tests()
{
  Model3 m1{};
  Concept<dyno::non_owning_storage> n1 = m1;
  Concept<dyno::shared_remote_storage> sr1 = Model3{};
  Concept<dyno::local_storage<sizeof(Model3)>> l1 = Model3{};
  Concept<dyno::sbo_storage<sizeof(Model3)>> sb1 = Model3{};
  Concept<dyno::remote_storage> r1 = Model3{};

  DYNO_CHECK(expectModel3Constructor( ENoConstructorInvocation, [&]
  {
    n1 = sr1;
    n1 = l1;
    n1 = sb1;
    n1 = r1;
  }));
}

DYNO_INTERFACE(SimpleConcept,
  (doNothing, void())
);

struct Noncopyable
{
    void doNothing(){}
};

void noncopyable_interface_tests()
{
    SimpleConcept<dyno::remote_storage > c {Noncopyable{}};

    DYNO_CHECK(expectModel3Constructor( EDefaultConstructed , [&]
    {
        Concept<dyno::remote_storage> r1{ dyno::make<Model3>{} };
        DYNO_CHECK(r1.f3(std::string{}) == std::make_tuple(91, '3'));
    }));
}
