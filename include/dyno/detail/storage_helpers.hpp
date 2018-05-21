// Copyright Lukasz Drozdz 2017
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)

#ifndef DYNO_STORAGE_HELPERS_HPP
#define DYNO_STORAGE_HELPERS_HPP

#include <cassert>

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

template< typename T > inline constexpr auto is_a_local_storage = is_a_local_storage_t<T>{};
template< typename T > inline constexpr auto is_a_sbo_storage = is_a_sbo_storage_t<T>{};
template< typename T > inline constexpr auto is_a_remote_storage = std::is_same_v<T, remote_storage>;
template< typename T > inline constexpr auto is_a_shared_remote_storage = std::is_same_v<T, shared_remote_storage>;
template< typename T > inline constexpr auto is_a_non_owning_storage = std::is_same_v<T, non_owning_storage>;

template< typename T >
constexpr void static_assert_storage_is_supported()
{
  static_assert(   is_a_local_storage<T>
                || is_a_sbo_storage<T>
                || is_a_remote_storage<T>
                || is_a_shared_remote_storage<T>
                ,"Trying to create a storage using an unsupported other_storage!");
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

} // namespace detail

} // namespace dyno

#endif
