# DISCLAIMER
I forked this library in order to enchance the original macro-based interface
creation mechanism, provided by Louis Dionne in his great library.

## Overview
__Dyno__ is a type-erasure library, which lets you use runtime polymorphism without the need to explicitly define an inheritance hierarchy. The following __Dyno__ fork is an attempt to remove some of the boilerplate and 
customizability problems of the root __Dyno__ provided by Louis Dionne.
The target was to add new possibilities with the __DYNO_INTERFACE__ macro,
such as:
1. to choose different storage policies than just (default from Louis Dionne) remote storage
2. to construct and copy interface instances of different storage policies
3. to create a given instance in-place, without the need to move it in
4. to provide additional interface properties, ex. noncopyable
5. to optimize-out the amount of unnecessary copies and moves

The target was to provide changes to __DYNO_INTERFACE__ macro, leaving all the not macro-based functionalities as they were implemented by Louis Dionne. Usage of the  macro does not require an in-depth knowledge of __Dyno__. However, if you want to know how __Dyno__ works under the hood, then I recommend reading the [README][] file by Louis Dionne.

In the following readme, I will refer to original __Dyno__ written by Louis Dionne as "legacy __DYNO__".

All the customizability keywords are in namespace `dyno::macro`

# Basic library usage
Let's assume we want to work on objects capable of drawing themselves on the given 'std::ostream'. First, we define a drawable interface and some structs to fulfill it:
```c++
DYNO_INTERFACE(Drawable,
  (draw, void(std::ostream&) const)
);

struct Square 
{
  void draw(std::ostream& out) const { out << "Square,"; }
};

struct Circle 
{
  void draw(std::ostream& out) const { out << "Circle,"; }
};
```

Then, we could store some drawable objects in a container:
```c++
std::vector<Drawable<>> vec{ Square{}, Circle{}, Square{}, Circle{} };
// ^ 'Drawable' is a template with default parameters, so in above context it must have <>
```
By default, the instances are stored on the heap, so the vector will in fact contain pointers to polymorphic objects managed on the heap (refer to `dyno::remote_storage` for details) and pointers to appropriate static vtables (refer to `dyno::remote_vtable` for details).

The type-erased objects can now be accessed in a natural syntax, ex:
```c++
for(const auto& obj : vec)
    obj.draw(std::cout);
```
The above prints:
> Square,Circle,Square,Circle,

The Drawable objects can be naturally copied, assigned or moved, ex:
```c++
vec[0] = vec.back();
```
Now, the first element is a copy of the last element in the vector, ie. a `Circle`.

# Custom storage policy
## Polymorphism without heap - on_stack<> storage
##### Non-boilerplate stack-based polymorphism can be easily achived using this fork of Dyno.
This __Dyno__ fork lets you use __runtime polymorphism with no dynamic memory allocation__. In the following example, using a `boost::container::static_vector`, all type-erased objects are kept directly on stack:
```c++
using namespace dyno::macro;

boost::container::static_vector< Drawable<on_stack<16> >, 10>
    vecOnStack{ Square{}, Circle{}, Square{}, Circle{} };

for(const auto& obj : vecOnStack)
    obj.draw(std::cout);
```
The above prints as expected:
> Square,Circle,Square,Circle,

We could also copy objects from heap storage to stack storage with ease:
```c++
std::vector<Drawable<>> vecOnHeap{ Square{}, Circle{}, Square{}, Circle{} };
std::copy(vecOnHeap.begin(), vecOnHeap.end(), std::back_inserter(vecOnStack));
```
`on_stack<16>` is a storage policy. It means that the size of the storage buffer in the Drawable object is 16 bytes. In this case, the size of the type-erased-object to be constructed can be no more than 16 bytes.

We could define a new class fulfilling the Drawable interface and add its object to our stack vector:
```c++
struct Triangle
{
  void draw(std::ostream& out) const { out << name << ","; }
  int doSomethingElse(){ return 0; }
  const char* name { "Triangle" };
};

if(vecOnStack.size() < vecOnStack.capacity())
    vecOnStack.emplace_back( Triangle{} );
```
Notice the Triangle can have excessive methods and fields. It can be treated like a Drawable as long as if fulfills the Drawable interface and fits in the given on_stack<> storage.

If the compiler detects an attemp to construct a `Drawable<on_stack<16>>` from a struct of size bigger than 16 bytes, a static assert is launched, ex:
```c++
struct BigSphere
{
  void draw(std::ostream& out) const { out << "BigSphere,"; }
  int points[100]{};
};
//    vecOnStack.emplace_back( BigSphere{} ); // will not copile!
```
gives a static assertion error:
> error: static assertion failed: dyno::local_storage: Trying to construct from an object that won't fit in the local storage.

Does this example ring a bell of a more general use case? We could use runtime on-stack polimorphism with value sematics even on memory-constrained bare-metal systems with no dynamic memory. 

## Small Buffer Optimisation - on_stack_or_heap<> storage
We might want to benefit from storing the object payload in a buffer inside the Drawable, but also not be constrained with a size limit if the situation demands it:
```c++
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
```
`on_stack_or_heap<16>` storage policy stores the object on stack if it fits in the buffer, which is 16 bytes in this case. If it's too large, the object is allocated on the heap. A BigSphere{} is much larger than 16 bytes, so it will be stored with operator new. This approach is often considered an optimisation, as it is likely to reduce the ammount of cache misses, but it should always be measured first.

## Shared objects - on_heap_shared storage
```c++
using namespace dyno::macro;
Drawable<on_heap_shared> shared1{ Circle{} };
auto shared2{ shared1 };
// ^ shared1 and shared2 refer to the same Circle;
```
Shared storage uses a `std::shared_ptr` in its implementation, so `on_heap_shared` objects are reference counted. A shared object can be created, apart from normal construction, by moving a 'standard' `on_heap` object into a `on_heap_shared` interface, just like creating a `std::shared_ptr` from a `std::unique_ptr`:
```c++
Drawable someHeapDrawable{ Circle{} };
Drawable<on_heap_shared> shared3{ std::move(someHeapDrawable) };
// ^ 'on_heap_shared' can be created from a 'on_heap' object
```

Of course it's not possible bo create a `on_heap` object by moving a `on_heap_shared` one, because it would violate shared ownership. 
```c++
Drawable someOtherHeapDrawable{ std::move(shared1) };
// ^ won't compile! 
```

## I'm just visiting - visited storage
Sometimes we might want to just use an object with no regard to it's ownership rules. The `visited` storage policy is our way to go, for ex.:
```c++
void drawOnCout(const Drawable<dyno::macro::visited>& d)
{
    d.draw(std::cout);
}
```
The function can be invoked with any object fulfilling the `Drawable` interface, ex:
```c++
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
```

You could say: "Just use a template function!" A template definition in general must be placed in a header file, whereas an implementation of a function with `visited` parameters can be moved to a different translation unit.

It's important to mention, that a `visited` cannot be used to visit an rvalue. It's completely reasonable. In the above example, in `drawOnCout(Square{})` a visitor
would try to use an object, which was allready destructed on the calling stack. This cannot be allowed.

## Performance matters - in_place<> construction
__Don't pay for what you don't use.__ Don't construct an object in one place just to move it to another.
Just like [`std::in_place`](https://en.cppreference.com/w/cpp/utility/in_place), we could use a template tag to construct the object directly in provided memory. In our Drawable example it would be:
```c++
struct Cuboid
{
    Cuboid(const char* name) : name{name} {}
    void draw(std::ostream& out) const { out << name << ","; }
    const char* name;
};
vec.emplace_back( in_place<Cuboid>, "Cuboid the Type Erased!" ).draw(std::cout);
```
The above code prints:
> Cuboid the Type Erased!

`in_place<>` is the optimal way to create __DYNO_INTERFACE__ objects.

## Reasonable construction
With this __Dyno__ fork, in comparrison to legacy __Dyno__, when using __DYNO_INTERFACE__ macro copy and move ctors are invoked only when necessary. 

To present this, we will define a `CountedCircle` class, which counts the number of move-constructor and copy-constructor invocatons:
```c++
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
```
When using the __DYNO_INTERFACE__ macro, the copy and move constructors are generally invoked only when you'd expect them to. If that is not enough for you and you want more details, the rules for the type-erased objects construction are as follows:

1. Creating a type-erased object from a concrete-type __lvalue__ always invokes a __copy__ ctor exactly once, regardless of the storage policy:
```c++
CountedCircle circle{};
Drawable<on_heap> drawable{ circle };
assert(1 == CountedCircle::copiedCount);
```
2. Creating a type-erased object from a concrete-type __rvalue__ always invokes a __move__ ctor exactly once, regardless of the storage policy:
```c++
Drawable<on_stack<8>> drawable2{ CountedCircle{} };
assert(1 == CountedCircle::movedCount);
```
3. Creating a type-erased object using an `in_place<>` tag __does not require any copy/move construction__. The constructor is invoked directly in memory:
```c++
Drawable<on_stack<8>> drawable3{ in_place<CountedCircle> };
assert(0 == CountedCircle::copiedCount);
assert(0 == CountedCircle::movedCount);
```
4. Copying/moving type-erased objects from/to an `on_stack<>` policy based object always invokes the copy/move ctor:
```c++
Drawable<on_stack<8>> drawable4{ in_place<CountedCircle> };
auto drawable4 = drawable5
auto drawable5 = std::move(drawable6);
assert(1 == CountedCircle::copiedCount);
assert(1 == CountedCircle::movedCount);
```
5. Copying a type-erased object always invokes the copy ctor regardless of the storage policy, apart from `on_heap_shared` copy, which just increases the reference count:
```c++
Drawable<on_heap_shared> drawable7{ in_place<CountedCircle> };
Drawable<on_heap_shared> drawable8{ drawable7 };
assert(0 == CountedCircle::copiedCount);

Drawable<on_heap> drawable9{ drawable7 };
assert(1 == CountedCircle::copiedCount);
```
6. Constructing an `on_heap` policy based object by moving another `on_heap` into it does not invoke the move ctor. It's just a pointer move:
```c++
Drawable<on_heap> drawable10{ in_place<CountedCircle> };
Drawable<on_heap> drawable11{ std::move(drawable10) };
assert(0 == CountedCircle::movedCount);
```
7. Moving a type-erased object into a `on_stack_or_heap<>` storage is dependent on the size of the moved object. 

If the type-erased object in-move does not fit in the predefined bufer __and__ is allready on the heap (i.e. is stored with a `on_heap` or `on_stack_or_heap<>` storage policy with size large enough), then it is moved with just a pointer move (just like 6.):
```c++
struct BigCountedCircle : CountedCircle
{
    char additionalSize[100];
};

Drawable<on_heap> drawable12{ in_place<BigCountedCircle> };
Drawable<on_stack_or_heap<8>> drawable13{ std::move(drawable12) };
Drawable<on_stack_or_heap<8>> drawable14{ std::move(drawable13) };
assert(0 == CountedCircle::movedCount);
```
Otherwise, the type-erased object in-move is moved with the move ctor:
```c++
Drawable<on_heap> drawable15{ in_place<CountedCircle> };
Drawable<on_stack_or_heap<8>> drawable16{ std::move(drawable15) };
Drawable<on_stack_or_heap<8>> drawable17{ std::move(drawable16) };
assert(2 == CountedCircle::movedCount);
```
8. `on_stack<N>` object with size N can be constructed from a type-erased object of policy `on_stack<M>` with size M only if `sizeof(aligned_storage_t<M>)<=sizeof(aligned_storage_t<N>)`:
```c++
Drawable<on_stack<8>> drawable1{ Circle{} };
Drawable<on_stack<64>> drawable2{ Square{} };

// drawable1 = drawable2; // won't copile!
drawable2 = drawable1;
```

## Construction-like assignment
__Assignment rules are the same as construction__, apart from the fact, that the storage is first destructed. After destruction, it is constructed with above listed construction rules. Example:
```c++
using namespace dyno::macro;

Drawable drawable1{ Circle{} };
// ^ default storage policy, i.e. on_heap
Drawable<on_stack<8>> drawable2{ Circle{} };

drawable2 = drawable1;
drawable2 = in_place<Circle>;

drawable2.draw(std::cout);
// prints "Circle,"
```

NOTE: A custom destruction policy makes sure, that in a situation of an exception thrown, the storage is always destructed only once. It's just in case that after a succesfull storage destruction, if the constructor throws an exception, the storage is not destructed again during stack unwinding due to a boolean check. This does not guarantee any level of exception safety.

## Exception safety
Currently, only `on_stack<>` objects construction and assignment provide basic exception safety (no resource leaks). It is planned to add strong exception safety for both stack- and heap-stored objects. 

## Advanced properties
#### non_copy_constructible
By default, type-erased objects must be copy and move-constructible. But what if you want your interface to be noncopyable? No problem! Just define it as an additional property of an interface instance:
```c++
struct NoncopyableLine
{
    NoncopyableLine(const NoncopyableLine&) = delete;
    NoncopyableLine(NoncopyableLine&&) = default;
    void draw(std::ostream& out) const { out << "NoncopyableLine,"; }
};
//  Drawable<on_heap> noncopyableDrawable{ NoncopyableLine{} }; // won't compile!
Drawable<on_heap, non_copy_constructible> noncopyableDrawable{ NoncopyableLine{} };
```
Our `noncopyableDrawable` can only be moved, not copied. By "copy" we mean a copy performed on the stored object payload with the copy ctor stored in the vtable. It means we still can copy a shared interface, which would just increse the reference count:

```c++
Drawable<on_heap_shared, non_copy_constructible> noncopyableShared1{ std::move(noncopyableDrawable) };
auto noncopyableShared2{ noncopyableShared1 };
```
#### non_move_constructible
By analogy to copy ctor, we can loosen the requirement on move ctor of our type_erased object:
```c++
struct NonmovableLine
{
    NonmovableLine() = default;
    NonmovableLine(const NonmovableLine&) = delete;
    NonmovableLine(NonmovableLine&&) = delete;
    void draw(std::ostream& out) const { out << "NonmovableLine,"; }
};
Drawable<on_heap, non_move_constructible> nonmovableDrawable{ in_place<NonmovableLine> }; // must use in_place<>
Drawable<on_heap, non_move_constructible> nonmovableDrawable2{ std::move(nonmovableDrawable) }; // move a pointer - legal
//  Drawable<on_stack<16>, non_move_constructible> nonmovableDrawable3{ std::move(nonmovableDrawable2) }; // move to stack with a vtable - illegal!
```
The `NonmovableLine`, due to a lack of a move constructor, must be created with `in_place<>`, because it can't be moved inside the object.

Again, `non_move_constructible` refers to the type-erased payload, not the interface object itself. Moving a `on_heap` object is just a pointer move, where a "move-construct" vtable entry is not needed. Moving a `on_heap` object into a `on_stack<>` one would requires an invocation of a move constructor, so it won't compile.

## Full example
The full example of provided __DYNO_INTERFACE__ macro functionalities is in [example/macro2.cpp](https://github.com/ldrozdz93/dyno/blob/master/example/macro2.cpp)

This also shows we could use runtime on-stack polimorphism with value sematics even on memory-constrained
bare-metal systems with no dynamic memory.

## TODO list:
#### Things ready
My main priority was to implement a fully __type-safe stack-based runtime polimorphism__ support. __This task is 'almost' finished__ from safety point of view. 'Almost', because only basic exception safety is provided for `on_stack<>`. If there is no dynamic memory management, there is no risk of allocation exceptions.  The size of objects to be crated or assigned is verified in compile time to fit in the predefined stack buffer.

This lets __DYNO_INTERFACE__ stack-based polimorphism to be safely used on virtually any hardware. No matter if you are building for x86-64 or Cortex-M0. A virtual call cost is just a one pointer indirection on the vtable, compared with a statically typed object. On the other hand you get an opportunity to write efficient object-oriented, unit-testable (mockable) code, even on a bare-metal hardware.
#### Things waiting to be done
There are still a few functionalities missing in forked __Dyno__, mostly considering memory and excepton safety. They are planned to be added in the future:
1. Custom allocators support. At the moment, only `std::malloc` is supported.
2. Custom alocation error handlers. At the moment, a simple assertion is used:
```c++
assert(ptr_ != nullptr && "std::malloc failed, we're doomed");
```
3. Fix exception safety of construction after allocation. Allocating and then calling the constructor is not exception-safe if the constructor throws. Destructing before construction violates strong exception safety, because the constructor could throw, leaving the object in an invalid (destructed) state.
4. Support for `noexcept` as a part of a method signature (C++17) in a __DYNO_INTERFACE__.
5. Support for method names overloading. At the moment, every method in a __DYNO_INTERFACE__ must have a different name.
5. Support for constructing type-erased objects from other type-erased objects which happen to be modelled by different __DYNO_INTERFACE__ macros, but with similar methods.
6. Refactor of unit tests. Testing static_asserts.



<!-- Links -->
[README]: https://github.com/ldionne/dyno/blob/master/README.md

