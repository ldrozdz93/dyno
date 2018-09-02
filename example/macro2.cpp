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

#include <algorithm>
void onStackExample()
{
    using namespace dyno::macro;

    boost::container::static_vector< Drawable<on_stack<16> >, 10>
        vecOnStack{ Square{}, Circle{}, Square{}, Circle{} };

    for(const auto& obj : vecOnStack)
        obj.draw(std::cout);

    std::vector<Drawable<>> vecOnHeap{ Square{}, Circle{}, Square{}, Circle{} };

    std::copy(vecOnHeap.begin(), vecOnHeap.end(), std::back_inserter(vecOnStack));

    struct Triangle
    {
      void draw(std::ostream& out) const { out << name << ","; }
      int doSomethingElse(){ return 0; }
      const char* name { "Triangle" };
    };

    if(vecOnStack.size() < vecOnStack.capacity())
        vecOnStack.emplace_back( Triangle{} );

    struct BigSphere
    {
      void draw(std::ostream& out) const { out << "BigSphere,"; }
      int points[100]{};
    };
//    vecOnStack.emplace_back( BigSphere{} ); // will not copile!


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

void drawOnCout(const Drawable<dyno::macro::visited>& d)
{
    d.draw(std::cout);
}

void visitedExample()
{
    using namespace dyno::macro;
    Circle circle{};
    Drawable<on_heap>             drawableOnHeap{ Circle{} };
    Drawable<on_stack<8>>         drawableOnStack{ Square{} };
    Drawable<on_heap_shared>      drawableShared{ Circle{} };
    Drawable<on_stack_or_heap<8>> drawableSbo{ Square{} };

    drawOnCout(circle);
    drawOnCout(drawableOnHeap);
    drawOnCout(drawableOnStack);
    drawOnCout(drawableShared);
    drawOnCout(drawableSbo);
    // drawOnCout(Square{}); // will not compile with an rvalue!
}

struct CountedConstruction
{
    CountedConstruction() = default;
    CountedConstruction(const CountedConstruction&) { ++copiedCount; }
    CountedConstruction(CountedConstruction&&) { ++movedCount; }

    inline static int copiedCount = 0 ;
    inline static int movedCount = 0 ;
    static int reset(){ copiedCount = movedCount = 0; }
};

struct CountedCircle : Circle, CountedConstruction
{};

void reasonableConstructionExample()
{
    using namespace dyno::macro;

    CountedCircle circle{};
    Drawable<on_heap> drawable{ circle };
    assert(1 == CountedCircle::copiedCount);

    Drawable<on_stack<8>> drawable2{ CountedCircle{} };
    assert(1 == CountedCircle::movedCount);

    CountedConstruction::reset();

    Drawable<on_stack<8>> drawable3{ in_place<CountedCircle> };
    assert(0 == CountedCircle::copiedCount);
    assert(0 == CountedCircle::movedCount);

    Drawable<on_stack<8>> drawable4{ in_place<CountedCircle> };
    Drawable<on_heap> drawable5 = drawable4;
    Drawable<on_heap> drawable6 = std::move(drawable4);
    assert(1 == CountedCircle::copiedCount);
    assert(1 == CountedCircle::movedCount);

    CountedConstruction::reset();

    Drawable<on_heap_shared> drawable7{ in_place<CountedCircle> };
    Drawable<on_heap_shared> drawable8{ drawable7 };
    assert(0 == CountedCircle::copiedCount);

    Drawable<on_heap> drawable9{ drawable7 };
    assert(1 == CountedCircle::copiedCount);

    CountedConstruction::reset();

    Drawable<on_heap> drawable10{ in_place<CountedCircle> };
    Drawable<on_heap> drawable11{ std::move(drawable10) };
    assert(0 == CountedCircle::movedCount);

    CountedConstruction::reset();

    struct BigCountedCircle : CountedCircle
    {
        char additionalSize[100];
    };

    Drawable<on_heap> drawable12{ in_place<BigCountedCircle> };
    Drawable<on_stack_or_heap<8>> drawable13{ std::move(drawable12) };
    Drawable<on_stack_or_heap<8>> drawable14{ std::move(drawable13) };
    assert(0 == CountedCircle::movedCount);

    Drawable<on_heap> drawable15{ in_place<CountedCircle> };
    Drawable<on_stack_or_heap<8>> drawable16{ std::move(drawable15) };
    Drawable<on_stack_or_heap<8>> drawable17{ std::move(drawable16) };
    assert(2 == CountedCircle::movedCount);

    CountedConstruction::reset();
}

void on_stackConstructionExample()
{
    using namespace dyno::macro;

    Drawable<on_stack<8>> drawable1{ Circle{} };
    Drawable<on_stack<64>> drawable2{ Square{} };

//    drawable1 = drawable2; // won't copile!
    drawable2 = drawable1;
}

void assignmentExample()
{
    using namespace dyno::macro;

    Drawable drawable1{ Circle{} };
    // ^ default storage policy, i.e. on_heap
    Drawable<on_stack<8>> drawable2{ Circle{} };

    drawable2 = drawable1;
    drawable2 = in_place<Circle>;

    drawable2.draw(std::cout);
    // prints "Circle,"
}

int main()
{
    basicExample();
    onStackExample();
    sboExample();
    onHeapSharedExample();
    visitedExample();
    reasonableConstructionExample();
    on_stackConstructionExample();
    assignmentExample();
}
