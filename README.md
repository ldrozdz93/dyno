## DISCLAIMER
I forked this library in order to enchance the original macro-based interface
creation mechanism, provided by Louis Dionne in his great library.

## Overview
__Dyno__ is a type-erasure library, which lets you use runtime polomorphism without the need to explicitly define an inheritance hierarchy. The following __Dyno__ fork is an attempt to remove some of the boilerplate and 
customizability problems of the root __DYNO__ provided by Louis Dionne.
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

## Sample library usage
Let's assume we want to work on objects capable of drawing themselves on the given 'std::ostream'. First, we define a drawable interface:

```c++
DYNO_INTERFACE(Drawable,
  (draw, void(std::ostream&) const)
);
```

Next, we define a container to hold the... WIP


I'll jump straight to the most interesting __Dyno__ use case for me - 
using stack-based polimorphism without any dynamic allocations:

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

This allows us to use runtime on-stack polimorphism, for example on memory-constrained
embedded systems. Of course the above example isn't fully optimal from performance pov, 
(a forwarding reference could be used, of even make_inplace<> idiom -
more below), but proves a general point: non-boilerplate stack-based
polimorphism can be achived using this fork of __Dyno__.


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
