## DISCLAIMER
I forked this library in order to enchance the original macro-based interface
creation mechanism, provided by Louis Dionne in his great library.

## Overview
__Dyno__ is a type-erasure library, which lets you use runtime polomorphism without the need to explicitly define an inheritance hierarchy. The following __Dyno__ fork is an attempt to remove some of the boilerplate and 
customizability problems of the root __Dyno__ provided by Louis Dionne.
The target was to add new possibilities with the __DYNO_INTERFACE__ macro,
such as:
1. to choose different storage policies than just (ldionne default) remote storage
2. to construct and copy interface instances of different storage policies
3. to create a given instance in-place, without the need to move it in
4. to provide additional interface properties, ex. noncopyable
5. to optimize-out the amount of unnecessary copies and moves

The target was to provide changes to __DYNO_INTERFACE__ macro, leaving all the not macro-based functionalities as they were implemented by Louis Dionne.

I assume you are familiar with how __Dyno__ works. If not, then you could read the 
[README][] file provided by ldionne, but a simple usage does not require an in-depth 
knowledge of __Dyno__.

In the following readme, I will refer to original __Dyno__ written by Louis Dionne as "legacy __DYNO__".

## Basic library usage
Let's assume we want to work on objects capable of drawing themselves on the given 'std::ostream'. First, we define a drawable interface and some structs to fulfill it:
```c++
DYNO_INTERFACE(Drawable,
  (draw, void(std::ostream&) const)
);

struct Square 
{
  void draw(std::ostream& out) const { out << "Square\n"; }
};

struct Circle 
{
  void draw(std::ostream& out) const { out << "Circle\n"; }
};
```

Then, we could store some drawable objects in a container:
```c++
std::vector<Drawable<>> vec{ Square{}, Circle{}, Square{}, Circle{} };
```
By default, the instances are stored on the heap, so the vector will in fact contain pointers to polymorphic objects managed on the heap (refer to `dyno::remote_storage` for details) and pointers to appropriate static vtables (refer to `dyno::remote_vtable` for details).

The type-erased objects can now be accessed in a natural syntax, ex:
```c++
for(const auto& obj : vec)
    obj.draw(std::cout);
```

The Drawable objects can be naturally copied, assigned or moved, ex:
```c++
vec[0] = vec.back();
```
Now, the first element is a copy of the last element in the vector, ie. a `Circle`.

## Polymorphism without heap - on_stack<> storage

Lets say we performed some performance benchmarks in our application and came to a conclusion, that our `vec` is the bottleneck. Iterating over stored drawable objects turnes out to cause a lots of cache misses due to actual objects being held on the heap in different places, because `vec` stores only the pointers to storage and vtable. What's more, we could also want to neglect the effect of vector heap indirection and use a boost::container::static_vector, which stores its data directly on the stack.

Assume we want to store at most 10 drawable objects. We could copy them from heap to stack, i.e. from `vec` to `stack_vec`:
```c++
using namespace dyno::macro_storage;
boost::container::static_vector< Drawable<on_stack<16> >, 10> stack_vec(
     vec.begin(), vec.begin() + std::min(vec.size(), 10ul));
```
`on_stack<16>` is a storage policy. It means that the size of the storage buffer in the Drawable object is 16 bytes. In this case, the size of the type-erased-object to be constructed can be no more than 16 bytes.

Our stack_vec can now be used as usual. We could define a new Drawable class and add its object to our stack vector:
```c++
struct Triangle 
{
  void draw(std::ostream& out) const { out << name << "\n"; }
  int doSomethingElse(){ return 0; }
  const char* name {"Triangle"};
};

if(stack_vec.size() < stack_vec.capacity())
    stack_vec.emplace_back( Triangle{} );

for(const auto& obj : stack_vec)
    obj.draw(std::cout);
```
Notice the Triangle can have excessive methods and fields. It can be treated like a Drawable as long as if fulfills the Drawable interface and fits in the given on_stack<> storage.

If the compiler detects an attemp to construct a `Drawable<on_stack<16>>` from a struct of size bigger than 16 bytes, a static assert is launched, ex:
```c++
struct Sphere
{
  void draw(std::ostream& out) const { out << "Sphere" << "\n"; }
  int points[100]{};
};
stack_vec.emplace_back( Sphere{} ); // will not copile!
```
gives a static assertion error:
> error: static assertion failed: dyno::local_storage: Trying to construct from an object that won't fit in the local storage.

Does this example ring a bell of a more general use case? We could use runtime on-stack polimorphism with value sematics even on memory-constrained bare-metal systems with no dynamic memory. __Non-boilerplate stack-based polimorphism can be easily achived using this fork of Dyno__.

## Small Buffer Optimisation - on_stack_or_heap<> storage
We might want to benefit from storing the object payload in a buffer inside the Drawable, but also not be constrained with a size limit if the situation demands it:
```c++
std::vector<Drawable<on_stack_or_heap<16>>> sbo_vec(stack_vec.begin(), stack_vec.end());
sbo_vec.emplace_back( Sphere{} );
```
`on_stack_or_heap<16>` storage policy stores the object on stack if it fits in the buffer, which is 16 bytes in this case. If it's too large, the object is allocated on the heap. A Sphere{} is much larger than 16 bytes, so it will be stored with operator new. This approach is often considered an optimisation, as it is likely to reduce the ammount of cache misses.

## Shared objects - on_heap_shared storage
Shared storage uses a `std::shared_ptr` in its implementation, so `on_heap_shared` objects are reference counted. A shared object can be created, apart from normal construction, by moving a 'standard' `on_heap` object into a `on_heap_shared` interface, just like creating a `std::shared_ptr` from a `std::unique_ptr`:
```c++
Drawable<on_heap_shared> shared1{ std::move(vec.back()) };
  vec.pop_back();
  auto shared2{ shared1 };
```
Of course it's not possible bo create a `on_heap` object by moving a `on_heap_shared` one, because it would violate shared ownership. 

## I'm just visiting - visitor non_owning storage
Sometimes we might want to just use an object with no regard to it's ownership rules. The `visitor` storage policy is our way to go, for ex.:
```c++
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
// drawOnCout(Square{}); // will not compile with an rvalue
```
It's important to mention, that a visitor cannot be used to visit an rvalue. It's completely reasonable. In the above example, in `drawOnCout(Square{})` a visitor would try to use an object, which was allready destructed on the calling stack. This cannot be allowed.

## Reasonable construction
On object creation only necessary constructors are invoked. The behavior can by deduced from common sense, ex.:
1. moving an `on_heap` object to another `on_heap` object does not invoke type-erased move ctor.
2. moving anything to a `on_stack<>` object always invokes the type-erased move ctor.
3. copying objects always invokes the type_erased copy ctor, with the exception of `on_heap_shared`, which can just increment it's reference count.
4. etc.

Common sense for `on_stack_or_heap<>` storage means that the following code:
```c++
Drawable<on_heap> someHeapDrawable{ Sphere{} }; // Sphere default ctor + Sphere move ctor
sbo_vec.emplace_back(std::move(someHeapDrawable)); // just a pointer moved
```
will invoke the Sphere constructor only twice:
1. Default construction in constructor argument list
2. Move construction of the created Sphere into the heap-allocated buffer

The `sbo_vec` will want to keep a Sphere on the heap, due to its size. `someHeapDrawable` allready allocated the Sphere on the heap, so the object will not be moved with Sphere's move constructor, but just with a pointer move. It's just like moving a `std::unique_ptr`.

If `someHeapDrawable` was initialized with an object of size less than 16 bytes, ex. Triangle{}, than moving a `on_heap` stored Triangle to a `on_stack_or_heap<16>` stored object would invoke a Triangle move constructor, as expected, because the Triangle is of size less than 16 bytes.

## Construction-like assignment
Assignment is the same as construction, apart from the fact, that the storage is first destructed. After destruction, it is constructed in a new buffer, or with placement-new whenever possible.

Exception safety of such a destruction-construction is achived using a custom destruction policy, which makes sure, that the storage is always destructed only once. It's just in case that after a succesfull storage destruction, if the constructor threw an exception, the storage would not be destructed again due to stack unwinding.

In other words, this __DYNO_INTERFACE__ macro does not require the objects's constructors or destructor to be noexcept to perform a safe assignment, although only an amateur or a lunatic would allow his destructor to throw ;)

## Performance matters - in_place<> 
__Don't pay for what you don't use.__ Don't construct an object in one place just to move it to another.
Just like [`std::in_place`](https://en.cppreference.com/w/cpp/utility/in_place), we could use tag dispatch to construct the object directly in provided memory. In our Drawable example it would be:
```c++
struct Cuboid
{
    Cuboid(const char* name) : name{name} {}
    void draw(std::ostream& out) const { out << name << "\n"; }
    const char* name;
};
vec.emplace_back( in_place<Cuboid>, "Cuboid the Type Erased!" );
vec.back().draw(std::cout);
```
The above code prints:
> Cuboid the Type Erased!

`in_place<>` is the optimal way to create objects __DYNO_INTERFACE__ objects.

## Full example

The full example in one place is given below:

```c++
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

    using namespace dyno::macro_storage;
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
    vec.emplace_back( in_place<Cuboid>, "Cuboid the Type Erased!" );
    vec.back().draw(std::cout);
}
```

This shows we could use runtime on-stack polimorphism with value sematics even on memory-constrained
bare-metal systems with no dynamic memory. Of course the above example isn't fully optimal from performance pov (a make_inplace<> idiom could be used -
more below), but proves a general point: non-boilerplate stack-based
polimorphism can be achived using this fork of __Dyno__.



Another example of stack-based polimorphism without any dynamic allocations:

```c++
#include <dyno.hpp>
#include <iostream>

// Define the interface of something that can be drawn
DYNO_INTERFACE(Drawable,
  (draw, void (std::ostream&) const)
);

// Define a class which will be initialised with any Drawable object
class UserOfDrawable
{
  using MyDrawable = Drawable<dyno::on_stack<16>>;

public:
  UserOfDrawable(MyDrawable p_drawable)
    : drawable(std::move(p_drawable))
  {}

  void doIt()
  {
    drawable.draw(std::cout);
  }

private:
  MyDrawable drawable;
};

struct Square {
  void draw(std::ostream& out) const { out << "Square"; }
};

struct Circle {
  void draw(std::ostream& out) const { out << "Circle"; }
};

int main() {
  UserOfDrawable user1{Square{}}; // polimorphism with no allocations
  UserOfDrawable user2{Circle{}}; // polimorphism with no allocations

  user1.doIt();
  user2.doIt();
}
```

The main difference is that __DYNO_INTERFACE__ now defines not just a class,
but a class template. The template default parameters are compatibile with
vanilla __Dyno__, so a fully copyable object of [`dyno::remote_storage`] 
policy is used by default.

<!-- Links -->
[README]: https://github.com/ldionne/dyno/blob/master/README.md
[`std::any`]: http://en.cppreference.com/w/cpp/utility/any
[`std::function`]: http://en.cppreference.com/w/cpp/utility/functional/function
[badge.Travis]: https://travis-ci.org/ldionne/dyno.svg?branch=master
[Boost.CallableTraits]: https://github.com/badair/callable_traits
[Boost.Hana]: https://github.com/boostorg/hana
[Boost.TypeErasure]: http://www.boost.org/doc/libs/release/doc/html/boost_typeerasure.html
[C++0x Concept Maps]: https://isocpp.org/wiki/faq/cpp0x-concepts-history#cpp0x-concept-maps
[Go interfaces]: https://gobyexample.com/interfaces
[Google Benchmark]: https://github.com/google/benchmark
[Haskell type classes]: http://learnyouahaskell.com/types-and-typeclasses
[libawful]: https://github.com/ldionne/libawful
[Mpark.Variant]: https://github.com/mpark/variant
[Rust trait objects]: https://doc.rust-lang.org/book/first-edition/trait-objects.html
[type erasure]: https://en.wikibooks.org/wiki/More_C%2B%2B_Idioms/Type_Erasure
[virtual concepts]: https://github.com/andyprowl/virtual-concepts
