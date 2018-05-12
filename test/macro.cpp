// Copyright Louis Dionne 2017
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)

#include "testing.hpp"

#include <dyno/macro.hpp>

#include <string>
#include <tuple>
#include <utility>


DYNO_INTERFACE(Concept,
  (f1, int(int) const),
  (f2, char(std::pair<long, double>) const),
  (f3, std::tuple<int, char> (std::string const&))
);

struct Model1 {
  int f1(int) const { return 1; }
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
  ECopied = 1 << 4,
  EMoved = 1 << 8
};
constexpr auto EOnlyBufferMoved = ENoConstructorInvocation;
constexpr auto ESharedPointerCopied = ENoConstructorInvocation;

struct ConstructorCounter
{
    uint32_t def = 0, copy = 0, move = 0 ;
    void reset(){ *this = ConstructorCounter{}; }
    bool check(const uint32_t expected)
    {
      return expected == (def * EDefaultConstructed |
                          copy * ECopied |
                          move * EMoved);
    }
} counter;

struct CountedConstruction
{
    CountedConstruction(){ counter.def++; }
    CountedConstruction(const CountedConstruction&){ counter.copy++; }
    CountedConstruction(CountedConstruction&&){ counter.move++; }
    ~CountedConstruction() = default;
};

struct Model3 : CountedConstruction
{
  int f1(int) const { return 91; }
  char f2(std::pair<long, double>) const { return '3'; }
  std::tuple<int, char> f3(std::string const&) { return {91, '3'}; }
};

void remote_storage_tests();
void shared_remote_storage_tests();
void local_storage_tests();
void sbo_storage_tests();
void non_owning_storage_tests();

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

  remote_storage_tests();
  shared_remote_storage_tests();
  local_storage_tests();
  sbo_storage_tests();
  non_owning_storage_tests();
}

void remote_storage_tests()
{
  counter.reset();
  Concept<dyno::remote_storage> r1 = Model3{};
  DYNO_CHECK(counter.check( EDefaultConstructed | EMoved ));

  counter.reset();
  Concept<dyno::remote_storage> r2 = r1;
  DYNO_CHECK(counter.check( ECopied ));

  counter.reset();
  Concept<dyno::remote_storage> r3 = std::move(r2);
  DYNO_CHECK(counter.check( EOnlyBufferMoved ));

  Model3 m3{};
  counter.reset();
  r3 = m3;
  DYNO_CHECK(counter.check( ECopied ));

  counter.reset();
  r3 = Model3{};
  DYNO_CHECK(counter.check( EDefaultConstructed | EMoved ));

  counter.reset();
  r3 = r2;
  DYNO_CHECK(counter.check( ECopied ));

  counter.reset();
  r3 = std::move(r2);
  DYNO_CHECK(counter.check( EOnlyBufferMoved ));
}

void shared_remote_storage_tests()
{
  Concept<dyno::shared_remote_storage> s1 = Model3{};
  counter.reset();
  Concept<dyno::shared_remote_storage> s2 = s1;
  DYNO_CHECK(counter.check( ESharedPointerCopied ));

  counter.reset();
  Concept<dyno::shared_remote_storage> s3 = std::move(s2);
  DYNO_CHECK(counter.check( ESharedPointerCopied ));
}

void local_storage_tests()
{
  Concept<dyno::local_storage<sizeof(Model3)>> l1 = Model3{};
  counter.reset();
  Concept<dyno::local_storage<sizeof(Model3)>> l2 = std::move(l1);
  DYNO_CHECK(counter.check( EMoved ));
}

void sbo_storage_tests()
{
  struct BigModel : Model3
  {
    char size[100];
  };
  Concept<dyno::sbo_storage<sizeof(Model3)>> sb1 = BigModel{};
  counter.reset();
  Concept<dyno::sbo_storage<sizeof(Model3)>> sb2 = std::move(sb1);
  DYNO_CHECK(counter.check( EOnlyBufferMoved ));


  Concept<dyno::sbo_storage<sizeof(Model3)>> sb3 = Model3{};
  counter.reset();
  Concept<dyno::sbo_storage<sizeof(Model3)>> sb4 = std::move(sb3);
  DYNO_CHECK(counter.check( EMoved ));

  Concept<dyno::sbo_storage<sizeof(Model3)>> sb5 = BigModel{};
  auto bm1 = BigModel{};
  counter.reset();
  sb5 = bm1;
  DYNO_CHECK(counter.check( ECopied ));

  counter.reset();
  sb5 = std::move(sb4);
  DYNO_CHECK(counter.check( 2*EMoved )); // TODO: fix generic assignment operator in macro
}

DYNO_INTERFACE(SimpleConcept,
  (fun, void())
);

void non_owning_storage_tests()
{
  struct SimpleModel
  {
    void fun(){}
  };

  SimpleModel m1{};
  counter.reset();
  SimpleConcept<dyno::non_owning_storage> n1 = m1;
  SimpleConcept<dyno::non_owning_storage> n2 = n1;
  n2 = n1;
  n2 = m1;
  DYNO_CHECK(counter.check( ENoConstructorInvocation ));
}
