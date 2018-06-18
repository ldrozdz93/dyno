// Copyright Lukasz Drozdz 2018
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)

#ifndef DYNO_DETAIL_MACRO_DETAILS_HPP
#define DYNO_DETAIL_MACRO_DETAILS_HPP

#include <type_traits>

namespace dyno
{

inline namespace properties
{
using PropertiesBitfield = uint32_t;

enum
{
  all_default = 0,
  copy_constructible = 1,
  non_copy_constructible = 1 << 1,
  exception_safe_constructible = 1 << 2,
  exception_unsafe_constructible = 1 << 3,
};

} // namespace properties

namespace detail
{

struct macro_config_raw
{
  bool is_copy_constructible;
  bool is_exception_safe_constructible;
};

constexpr macro_config_raw default_config{
    /*.is_copy_constructible =*/ true
   ,/*.is_exception_safe_constructible =*/ true
};

struct macro_config : macro_config_raw
{
  using PropertiesBitfield = properties::PropertiesBitfield;

  constexpr macro_config(const PropertiesBitfield prop)
  {
      is_copy_constructible = copy_constructible & prop ? true :
                              non_copy_constructible & prop ? false :
                              default_config.is_copy_constructible;

      is_exception_safe_constructible = exception_safe_constructible & prop ? true :
                                        exception_unsafe_constructible & prop ? false :
                                        default_config.is_exception_safe_constructible;
  }
};

}} // namespace dyno namespace detail

#endif //DYNO_DETAIL_MACRO_DETAILS_HPP
