// Copyright Łukasz Drożdż 2018
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)

#include <dyno.hpp>
#include <iostream>
#include <vector>
#include <boost/range/adaptor/sliced.hpp>
#include <boost/range/algorithm/copy.hpp>
#include <boost/container/static_vector.hpp>

DYNO_INTERFACE(Drawable,
  (draw, void (std::ostream&) const)
);

struct Square
{
  void draw(std::ostream& out) const { out << "Square\n"; }
};

struct Circle
{
  void draw(std::ostream& out) const { out << "Circle\n"; }
};

int main()
{
    std::vector<Drawable<>> vec{ Square{}, Circle{}, Square{}, Circle{} };
    for(const auto& obj : vec)
        obj.draw(std::cout);

    vec[0] = vec.back();

    using namespace dyno::macro;
    boost::container::static_vector< Drawable<on_stack<4> >, 10> stack_vec(
         vec.begin(), vec.begin() + std::min(vec.size(), 10ul));

    struct Triangle
    {
      void draw(std::ostream& out) const { out << name << "\n"; }
      int doSomethingElse(){ return 0; }
      const char* name { "Triangle" };
    };

    if(stack_vec.size() < stack_vec.capacity())
        stack_vec.emplace_back( Triangle{} );

    for(const auto& obj : stack_vec)
        obj.draw(std::cout);

    struct Sphere
    {
      void draw(std::ostream& out) const { out << "Sphere" << "\n"; }
      int points[100]{};
    };
//    stack_vec.emplace_back( Sphere{} ); // will not copile!

    std::vector<Drawable<on_stack_or_heap<16>>> sbo_vec(stack_vec.begin(), stack_vec.end());
    sbo_vec.emplace_back( Sphere{} );

    for(const auto& obj : sbo_vec)
        obj.draw(std::cout);

    Drawable someHeapDrawable{ Sphere{} };
    sbo_vec.emplace_back(std::move(someHeapDrawable));

    Drawable<on_heap_shared> shared1{ std::move(vec.back()) };
    vec.pop_back();
    auto shared2{ shared1 };

    auto drawOnCout = [](const Drawable<visitor>& d) // auto& not used just to prove a point ;)
    {
        d.draw(std::cout);
    };
    Circle circle{};
    drawOnCout(circle);
    drawOnCout(vec[0]);
    drawOnCout(stack_vec[1]);
    drawOnCout(sbo_vec[2]);
    drawOnCout(shared1);
//    drawOnCout(Square{}); // will not compile with an rvalue

    struct Cuboid
    {
        Cuboid(const char* name) : name{name} {}
        void draw(std::ostream& out) const { out << name << "\n"; }
        const char* name;
    };

    vec.emplace_back( in_place<Cuboid>, "Cuboid the Type Erased!" ).draw(std::cout);
}
//////////////////////////////////////////////////////////////////////////////
