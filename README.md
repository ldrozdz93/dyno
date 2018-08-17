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

struct Square {
  void draw(std::ostream& out) const { out << "Square\n"; }
};

struct Circle {
  void draw(std::ostream& out) const { out << "Circle\n"; }
};
```

Then, we could store some drawable objects in a container:
```c++
std::vector<Drawable<>> vec{ Square{}, Circle{}, Square{}, Circle{} };
```
By default, the instances are stored on the heap, so the vector will in fact contain pointers to polymorphic objects managed on the heap (refer to `dyno::remote_storage` for details).

The type-erased objects can now be accessed in a natural syntax, ex:
```c++
for(const auto& obj : vec)
    obj.draw(std::cout);
```

The Drawable objects can be naturally copied, assigned or moved, ex:
```c++
vec[0] = vec.back();
```
Now, the first and the last element in the vector are the same.

## Polymorphism without heap

Lets say we performed some performance benchmarks in our application and came to a conclusion, that our `vec` is the bottleneck. Iterating over stored drawable objects turnes out to cause a lots of cache misses due to actual objects being held on the heap in different places, because `vec` stores only the pointers to storage and vtable. What's more, we could also want to neglect the effect of vector heap indirection and use a boost::container::static_vector, which stores its data directly on the stack.

Assume we want to store at most 10 drawable objects. We could copy them from heap to stack, i.e. from `vec` to `stack_vec`:
```c++
using namespace dyno::macro_storage;
boost::container::static_vector< Drawable<on_stack<4> >, 10> stack_vec{};
boost::range::copy( vec | boost::adaptors::sliced(0, std::min(vec.size(), stack_vec.capacity())),
                    std::back_inserter(stack_vec));
```
`on_stack<4>` is a storage policy. It means that the size of the storage buffer in the Drawable object is 4 bytes (it's a lie, because the buffer is properly alligned anyway). In this case, size of type-erased-object to be constructed can be no more than 4 bytes, or a compile-time or runtime error will occur, whichever is detected.

Our stack_vec can now be used as usual. We could define a new Drawable class and add its object to our stack vector:
```c++
struct Triangle {
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

struct Square {
  void draw(std::ostream& out) const { out << "Square\n"; }
};

struct Circle {
  void draw(std::ostream& out) const { out << "Circle\n"; }
};

int main()
{
        std::vector<Drawable<>> vec{ Square{}, Circle{}, Square{}, Circle{} };
    for(const auto& obj : vec)
        obj.draw(std::cout);

    vec[0] = vec.back();

    using namespace dyno::macro_storage;
    boost::container::static_vector< Drawable<on_stack<4> >, 10> stack_vec{};
    boost::range::copy( vec | boost::adaptors::sliced(0, std::min(vec.size(), stack_vec.capacity())),
                        std::back_inserter(stack_vec));

    struct Triangle {
      void draw(std::ostream& out) const { out << name << "\n"; }
      int doSomethingElse(){ return 0; }
      const char* name {"Triangle"};
    };

    if(stack_vec.size() < stack_vec.capacity())
        stack_vec.emplace_back( Triangle{} );

    for(const auto& obj : stack_vec)
        obj.draw(std::cout);
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
