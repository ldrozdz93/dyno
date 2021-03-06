// Copyright Louis Dionne 2017
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)

#include "testing.hpp"

#include <dyno/macro.hpp>

#include <string>
#include <tuple>
#include <utility>
#include <boost/noncopyable.hpp>

DYNO_INTERFACE(Concept,
  (f1, int(int) const),
  (f2, char(std::pair<long, double>) const),
  (f3, std::tuple<int, char> (std::string const&))
);

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
  CountedConstruction() noexcept { counter.def++; }
  CountedConstruction(const CountedConstruction&) noexcept { counter.copy++; }
  CountedConstruction(CountedConstruction&&) noexcept { counter.move++; }
  ~CountedConstruction() = default;
};

struct Model3 : CountedConstruction
{
  Model3() = default;
  Model3(int, int) noexcept {}

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
void make_inplace_tests();
void constructing_noncopyable_tests();
void constructing_nonmovable_tests();
void exceptionSafetyTests();


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
  make_inplace_tests();
  constructing_noncopyable_tests();
  constructing_nonmovable_tests();
  exceptionSafetyTests();
}

void remote_storage_simple_construction_tests()
{
  Concept<dyno::on_heap> r1 = Model3{};
  Concept<dyno::on_heap> r2 = Model3{};
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
  Concept<dyno::on_heap_shared> s1 = Model3{};

  DYNO_CHECK(expectModel3Constructor( ESharedPointerCopied, [&]
  {
    Concept<dyno::on_heap_shared> s2 = s1;
  }));

  DYNO_CHECK(expectModel3Constructor( ESharedPointerCopied, [&]
  {
    Concept<dyno::on_heap_shared> s2 = std::move(s1);
  }));
}

void local_storage_simple_construction_tests()
{
  Concept<dyno::on_stack<sizeof(Model3)>> l1 = Model3{};
  Concept<dyno::on_stack<sizeof(Model3)>> l3 = Model3{};
  const Concept<dyno::on_stack<sizeof(Model3)>> l4 = Model3{};

  DYNO_CHECK(expectModel3Constructor( ECopiedWithVTable, [&]
  {
    Concept<dyno::on_stack<sizeof(Model3)>> l2 { l4 };
  }));

  DYNO_CHECK(expectModel3Constructor( EMovedWithVTable, [&]
  {
    Concept<dyno::on_stack<sizeof(Model3)>> l2 = std::move(l1);
  }));

  DYNO_CHECK(expectModel3Constructor( ECopiedWithVTable, [&]
  {
    l3 = l1;
  }));

  DYNO_CHECK(expectModel3Constructor( EMovedWithVTable, [&]
  {
    l3 = std::move(l1);
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

//  DYNO_CHECK(expectModel3Constructor( EMovedWithVTable, [&]
//  {
//    sb = std::move(sb_stack);
//  }));
}

void non_owning_storage_simple_construction_tests()
{
  Model3 m1{};

  DYNO_CHECK(expectModel3Constructor( ENoConstructorInvocation, [&]
  {
    Concept<dyno::visited> n1 = m1;
    Concept<dyno::visited> n2 = n1;
    n2 = n1;
    n2 = std::move(n1);
    n2 = m1;
  }));
}

void remote_storage_convertion_tests()
{
  Concept<dyno::on_heap> r1 = Model3{};
  Concept<dyno::on_stack<sizeof(Model3)>> l1 = Model3{};
  Concept<dyno::sbo_storage<sizeof(Model3)>> sb1_stack = Model3{};
  Concept<dyno::sbo_storage<sizeof(Model3) / 2>> sb1_heap = Model3{};
  Concept<dyno::on_heap_shared> sr1 = Model3{};

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
  Concept<dyno::on_heap_shared> sr1 = Model3{};
  Concept<dyno::on_stack<sizeof(Model3)>> l1 = Model3{};
  Concept<dyno::sbo_storage<sizeof(Model3)>> sb1_stack = Model3{};
  Concept<dyno::sbo_storage<sizeof(Model3) / 2>> sb1_heap = Model3{};
  Concept<dyno::on_heap> r1 = Model3{};

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
  Concept<dyno::on_heap_shared> sr1 = Model3{};
  Concept<dyno::on_stack<sizeof(Model3)>> l1 = Model3{};
  Concept<dyno::on_heap> r1 = Model3{};
  Concept<dyno::on_heap> r2 = Model3{};

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

  // TODO: find a way to test this...
  // sb1_heap = std::move(sr1); // moving shared_remote_storage -> sbo_storage should't compile!
}

void local_storage_convertion_tests()
{
  constexpr auto model_size = sizeof(Model3);
  Concept<dyno::on_stack<model_size>> l1 = Model3{};
  Concept<dyno::on_stack<model_size * 2>> l_big = Model3{};
  Concept<dyno::on_heap> r1 = Model3{};

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
  Concept<dyno::visited> n1 = m1;
  Concept<dyno::on_heap_shared> sr1 = Model3{};
  Concept<dyno::on_stack<sizeof(Model3)>> l1 = Model3{};
  Concept<dyno::sbo_storage<sizeof(Model3)>> sb1 = Model3{};
  Concept<dyno::on_heap> r1 = Model3{};

  DYNO_CHECK(expectModel3Constructor( ENoConstructorInvocation, [&]
  {
    n1 = sr1;
    n1 = l1;
    n1 = sb1;
    n1 = r1;
  }));
}

void make_inplace_tests()
{
  Concept<dyno::on_heap> r1 = Model3{};

  DYNO_CHECK(expectModel3Constructor( EDefaultConstructed , [&]
  {
      r1 = { dyno::make_inplace<Model3> };
      DYNO_CHECK(r1.f3(std::string{}) == std::make_tuple(91, '3'));
  }));

  DYNO_CHECK(expectModel3Constructor( EDefaultConstructed , [&]
  {
      r1 = { dyno::make_inplace<Model3>, int{}, int{} };
      DYNO_CHECK(r1.f3(std::string{}) == std::make_tuple(91, '3'));
  }));

  DYNO_CHECK(expectModel3Constructor( EDefaultConstructed , [&]
  {
      Concept<dyno::on_stack<sizeof(Model3)>> r2{ dyno::make_inplace<Model3> };
      DYNO_CHECK(r2.f3(std::string{}) == std::make_tuple(91, '3'));
  }));

  DYNO_CHECK(expectModel3Constructor( EDefaultConstructed , [&]
  {
      Concept<dyno::sbo_storage<sizeof(Model3) / 2>> r2{ dyno::make_inplace<Model3> };
      DYNO_CHECK(r2.f3(std::string{}) == std::make_tuple(91, '3'));
  }));
}

void constructing_noncopyable_tests()
{
  struct Model3Noncopyable : public Model3
  {
    Model3Noncopyable() = default;
    Model3Noncopyable(const Model3Noncopyable&) = delete;
    Model3Noncopyable(Model3Noncopyable&&) = default;

    void doNothing(){}
  };

  using namespace dyno;

  // test noncopyable can be constructed inplace
  DYNO_CHECK(expectModel3Constructor( EDefaultConstructed , [&]
  {
      Concept<sbo_storage<sizeof(Model3Noncopyable)>, non_copy_constructible>
        c1{ make_inplace<Model3Noncopyable> };
  }));

  //  test can still move from other concept if copy construction was deleted.
  DYNO_CHECK(expectModel3Constructor( EDefaultConstructed | EMovedWithVTable, [&]
  {
      Concept<sbo_storage<sizeof(Model3Noncopyable)>, non_copy_constructible>
        c1{ make_inplace<Model3Noncopyable> };
      Concept<on_heap, non_copy_constructible> c2{ std::move(c1) };
  }));

  //  test can still move object if copy construction was deleted.
  DYNO_CHECK(expectModel3Constructor( EDefaultConstructed | EMovedWithVTable, [&]
  {
      auto model = Model3Noncopyable{};
      Concept<on_heap, non_copy_constructible> c1{ std::move(model) };
  }));

// TODO: Test below static_assert
//  Concept<remote_storage, non_copy_constructible> c2{ c1 }; // should fail to compile
}

void constructing_nonmovable_tests()
{
  struct Model3Nonmovable : public Model3
  {
    Model3Nonmovable() = default;
    Model3Nonmovable(Model3Nonmovable&&) = delete;
  };

  using namespace dyno;

  // test nonmovable can be constructed inplace
  DYNO_CHECK(expectModel3Constructor( EDefaultConstructed , [&]
  {
      Concept<sbo_storage<sizeof(Model3Nonmovable)>, non_move_constructible>
        c1{ make_inplace<Model3Nonmovable> };
  }));

  // test the pointer still can be moved
  DYNO_CHECK(expectModel3Constructor( EDefaultConstructed | EOnlyBufferPointerMoved , [&]
  {
      Concept<on_heap, non_move_constructible>
        c1{ make_inplace<Model3Nonmovable> };
      auto c2 = std::move(c1);
  }));

  // test the shared pointer still can still be copied (increase reference count)
  DYNO_CHECK(expectModel3Constructor( EDefaultConstructed | EOnlyBufferPointerMoved , [&]
  {
      Concept<on_heap_shared, non_move_constructible>
        c1{ make_inplace<Model3Nonmovable> };
      auto c2 = c1;
  }));
}

void exceptionSafetyTests()
{
    using namespace dyno::macro;
    auto basicExceptionSafetyDuringMoveConstrucion = []()
    {
        struct ThrowingModel3 : Model3
        {
            ThrowingModel3() = default;
            ThrowingModel3(const ThrowingModel3&)
            {
                throw 0;
            }
        };

        try
        {
            Concept<on_heap> drawable10{ ThrowingModel3{} };
        }
        catch (...){ }
        //no leaks detected with valgrind;
    };

    auto basicExceptionSafetyDuringInPlaceConstrucion = []()
    {
        struct ThrowingModel3 : Model3
        {
            ThrowingModel3()
            {
                throw 0;
            }
        };

        try
        {
            Concept<on_heap> drawable10{ in_place<ThrowingModel3> };
        }
        catch (...){ }
    };

    basicExceptionSafetyDuringMoveConstrucion();
    basicExceptionSafetyDuringInPlaceConstrucion();
}
