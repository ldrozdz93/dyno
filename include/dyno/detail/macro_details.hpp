// Copyright Lukasz Drozdz 2018
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)

#ifndef DYNO_DETAIL_MACRO_DETAILS_HPP
#define DYNO_DETAIL_MACRO_DETAILS_HPP

#include <type_traits>

namespace dyno
{

template< typename T >
struct make
{
  using type = std::decay_t<T>;
};

namespace detail
{
class Properties
{
  using Bitfield = uint32_t;
  Bitfield prop;

public:
  enum Property
  {
    defaults = 0,
    non_copy_constructible = 1
  };

  constexpr Properties(const Bitfield p_prop) :
    prop(p_prop)
  {}

  constexpr bool is_copy_construcible() const { return not( non_copy_constructible & prop ); }
};

}} // namespace dyno namespace detail

#endif //DYNO_DETAIL_MACRO_DETAILS_HPP
