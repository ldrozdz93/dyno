// Copyright Lukasz Drozdz 2017
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)

#ifndef DYNO_STORAGE_HELPERS_HPP
#define DYNO_STORAGE_HELPERS_HPP

#include <cassert>
#include <cstdlib>
#include <type_traits>
#include <dyno/detail/dsl.hpp>
#include <memory>

namespace dyno
{

template< std::size_t, std::size_t > class local_storage;
template< std::size_t sz1, std::size_t sz2 > class sbo_storage;
struct remote_storage;
struct shared_remote_storage;
struct non_owning_storage;

namespace detail
{
template< typename > struct is_a_local_storage_t : std::false_type {};
template< std::size_t sz1, std::size_t sz2 > struct is_a_local_storage_t<local_storage<sz1, sz2> > : std::true_type {};

template< typename > struct is_a_sbo_storage_t : std::false_type {};
template< std::size_t sz1, std::size_t sz2 > struct is_a_sbo_storage_t<sbo_storage<sz1, sz2> > : std::true_type {};

template< typename T > inline constexpr auto is_a_local_storage = is_a_local_storage_t<T>::value;
template< typename T > inline constexpr auto is_a_sbo_storage = is_a_sbo_storage_t<T>::value;
template< typename T > inline constexpr auto is_a_remote_storage = std::is_same_v<T, remote_storage>;
template< typename T > inline constexpr auto is_a_shared_remote_storage = std::is_same_v<T, shared_remote_storage>;
template< typename T > inline constexpr auto is_a_non_owning_storage = std::is_same_v<T, non_owning_storage>;

template< typename T, typename RawT = std::decay_t<T> >
constexpr void static_assert_can_construct_from_storage()
{
  static_assert(   is_a_local_storage<RawT>
                || is_a_sbo_storage<RawT>
                || is_a_remote_storage<RawT>
                || is_a_shared_remote_storage<RawT>
                ,"Trying to create a storage using an unsupported other_storage!");

  static_assert(not (detail::is_a_shared_remote_storage<RawT> &&
                     !std::is_lvalue_reference_v<T>),
                "Can't move from a shared_remote_storage into a non-shared storage. "
                "It would violate the shared ownership!");
}

template <typename OtherStorage, typename VTable, typename RawOtherStorage = std::decay_t<OtherStorage>>
void construct_with_vtable(void* ptr, OtherStorage&& other_storage, const VTable& vtable)
{
  if constexpr( std::is_lvalue_reference_v<OtherStorage> )
  {
    vtable["copy-construct"_s](ptr, other_storage.get());
  }
  else // other_storage initialized with an rvalue
  {
    vtable["move-construct"_s](ptr, other_storage.get());
  }
}

template< typename VTable >
void* alloc_with_vtable(const VTable& vtable)
{
  void* ptr_ = std::malloc(vtable["storage_info"_s]().size);
  // TODO: That's not a really nice way to handle this
  assert(ptr_ != nullptr && "std::malloc failed, we're doomed");
  return ptr_;
}

class ScopedVoidPtr
{
  void* ptr;

public:
  explicit ScopedVoidPtr(void* source) noexcept
      : ptr{source}
  {}
  ScopedVoidPtr(const ScopedVoidPtr&) = delete;
  ~ScopedVoidPtr()
  {
    std::free(ptr);
  }

  void* get() noexcept { return ptr; }

  [[nodiscard]]
  void* release() noexcept
  {
    auto res = ptr;
    ptr = nullptr;
    return res;
  }
};

template <typename OtherStorage, typename VTable, typename RawOtherStorage = std::decay_t<OtherStorage>>
void* alloc_and_construct_with_vtable(OtherStorage&& other_storage, const VTable& vtable)
{
  ScopedVoidPtr ptr{ alloc_with_vtable(vtable) };
  construct_with_vtable(ptr.get(), std::forward<OtherStorage>(other_storage), vtable);
  return ptr.release();
}

template <typename ExplicitT = void, typename T, typename RawT = std::decay_t<T>>
void* alloc_and_construct_with_T(T&& t) // TODO: handle variable arguments!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
{
  using TtoBeConstructed = std::conditional_t<std::is_same<ExplicitT, void>::value,
                                                RawT, ExplicitT>;
  ScopedVoidPtr ptr{ std::malloc(sizeof(TtoBeConstructed)) };

  // TODO: That's not a really nice way to handle this
  assert(ptr.get() != nullptr && "std::malloc failed, we're doomed");


  new (ptr.get()) TtoBeConstructed(std::forward<T>(t));
  return ptr.release();
}

/*
      ptr_ = std::malloc(sizeof(RawT));
      // TODO: Allocating and then calling the constructor is not
      //       exception-safe if the constructor throws.
      // TODO: That's not a really nice way to handle this
      assert(ptr_ != nullptr && "std::malloc failed, we're doomed");
      new (ptr_) RawT(std::forward<T>(t));
*/


template< typename OtherStorage >
void* movePtrFrom(OtherStorage& other_storage)
{
  void* ptr = other_storage.ptr_;
  other_storage.ptr_ = nullptr;
  return ptr;
}

namespace can_move_ptr_from_other_storage
{

template <typename OtherStorage, typename RawOtherStorage = std::decay_t<OtherStorage>>
constexpr bool compile_time_check()
{
  constexpr bool should_be_moved_from = not std::is_lvalue_reference_v<OtherStorage>;
  return should_be_moved_from &&
         (is_a_sbo_storage<RawOtherStorage> || is_a_remote_storage<RawOtherStorage>);
}

template <typename OtherStorage, typename RawOtherStorage = std::decay_t<OtherStorage>>
bool runtime_check(const OtherStorage& other_storage)
{
  static_assert(compile_time_check<OtherStorage>(),
                "This func should be preceeded with a compile-time check!");
  if constexpr( is_a_sbo_storage<RawOtherStorage> )
  {
    return other_storage.uses_heap();
  }
  else
    return true;
}


} // namespace canPtrBeMovedFromOtherStorage

// below struct is used as a tag to construct a type in-place in storage with placement new without move construction
template< typename T >
struct make_inplace_t
{
  using type = std::decay_t<T>;
};

template< typename > struct is_a_make_inplace_t : std::false_type {};
template< typename T > struct is_a_make_inplace_t< make_inplace_t<T> > : std::true_type {};
template< typename T > constexpr auto is_a_make_inplace = detail::is_a_make_inplace_t<std::decay_t<T>>::value;
template< typename T > inline constexpr auto make_inplace = detail::make_inplace_t<T>{};

} // namespace detail

template< typename T > inline constexpr auto make_inplace = detail::make_inplace<T>;


} // namespace dyno

#endif
