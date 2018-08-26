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
  void draw(std::ostream& out) const { out << "Square,"; }
};

struct Circle
{
  void draw(std::ostream& out) const { out << "Circle,"; }
};

void basicExample()
{
    std::vector<Drawable<>> vec{ Square{}, Circle{}, Square{}, Circle{} };
    // ^ 'Drawable' is a template with default parameters, so in above context it must have <>

    for(const auto& obj : vec)
        obj.draw(std::cout);
    //  ^ prints Square,Circle,Square,Circle,

    vec[0] = vec.back();

    for(const auto& obj : vec)
        obj.draw(std::cout);
    //  ^ prints Circle,Circle,Square,Circle,
}

void onStackExample()
{
    using namespace dyno::macro;
    std::vector<Drawable<>> vec{ Square{}, Circle{}, Square{}, Circle{} };

    boost::container::static_vector< Drawable<on_stack<4> >, 10> stack_vec(
         vec.begin(), vec.begin() + std::min(vec.size(), 10ul));

    for(const auto& obj : stack_vec)
        obj.draw(std::cout);

    struct Triangle
    {
      void draw(std::ostream& out) const { out << name << ","; }
      int doSomethingElse(){ return 0; }
      const char* name { "Triangle" };
    };

    if(stack_vec.size() < stack_vec.capacity())
        stack_vec.emplace_back( Triangle{} );

    struct BigSphere
    {
      void draw(std::ostream& out) const { out << "BigSphere,"; }
      int points[100]{};
    };
//    stack_vec.emplace_back( BigSphere{} ); // will not copile!
}

void sboExample()
{
    using namespace dyno::macro;
    std::vector<Drawable<on_stack_or_heap<16>>>
            sbo_vec{ Square{}, Circle{}, Square{}, Circle{} };

    struct BigSphere
    {
      void draw(std::ostream& out) const { out << "BigSphere,"; }
      int points[100]{};
    };

    sbo_vec.emplace_back( BigSphere{} );
    // ^ BigSphere does not fit inside the object buffer, so it's stored on the heap

    for(const auto& obj : sbo_vec)
        obj.draw(std::cout);
    //  ^ prints Circle,Circle,Square,Circle,BigSphere,
}

void onHeapSharedExample()
{
    using namespace dyno::macro;
    Drawable<on_heap_shared> shared1{ Circle{} };
    auto shared2{ shared1 };
    // ^ shared1 and shared2 refer to the same Circle;

    Drawable someHeapDrawable{ Circle{} };
    Drawable<on_heap_shared> shared3{ std::move(someHeapDrawable) };
    // ^ 'on_heap_shared' can be created from a 'on_heap' object

    //Drawable someOtherHeapDrawable{ std::move(shared1) };
    // ^ won't compile!
}

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
      void draw(std::ostream& out) const { out << name << ","; }
      int doSomethingElse(){ return 0; }
      const char* name { "Triangle" };
    };

    if(stack_vec.size() < stack_vec.capacity())
        stack_vec.emplace_back( Triangle{} );

    for(const auto& obj : stack_vec)
        obj.draw(std::cout);

    struct Sphere
    {
      void draw(std::ostream& out) const { out << "Sphere,"; }
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

    auto drawOnCout = [](const Drawable<visited>& d) // auto& not used just to prove a point ;)
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
        void draw(std::ostream& out) const { out << name << ","; }
        const char* name;
    };

    vec.emplace_back( in_place<Cuboid>, "Cuboid the Type Erased!" ).draw(std::cout);

    struct NoncopyableLine
    {
        NoncopyableLine(const NoncopyableLine&) = delete;
        NoncopyableLine(NoncopyableLine&&) = default;
        void draw(std::ostream& out) const { out << "NoncopyableLine,"; }
    };
//    Drawable<on_heap> noncopyableDrawable{ NoncopyableLine{} }; // won't compile!
    Drawable<on_heap, non_copy_constructible> noncopyableDrawable{ NoncopyableLine{} };

    Drawable<on_heap_shared, non_copy_constructible> noncopyableShared1{ std::move(noncopyableDrawable) };
    auto noncopyableShared2{ noncopyableShared1 };

    struct NonmovableLine
    {
        NonmovableLine() = default;
        NonmovableLine(const NonmovableLine&) = delete;
        NonmovableLine(NonmovableLine&&) = delete;
        void draw(std::ostream& out) const { out << "NonmovableLine,"; }
    };
    Drawable<on_heap, non_move_constructible> nonmovableDrawable{ in_place<NonmovableLine> }; // must use in_place<>
    Drawable<on_heap, non_move_constructible> nonmovableDrawable2{ std::move(nonmovableDrawable) }; // move a pointer - legal
//    Drawable<on_stack<16>, non_move_constructible> nonmovableDrawable3{ std::move(nonmovableDrawable2) }; // move to stack with a vtable - illegal!

    basicExample();
    onStackExample();
    sboExample();
}
