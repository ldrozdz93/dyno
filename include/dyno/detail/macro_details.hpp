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
};

} // namespace properties

namespace detail
{

struct macro_config_raw
{
  bool is_copy_constructible;
};

constexpr macro_config_raw default_config{
    /*.is_copy_constructible =*/ true
};

struct macro_config : macro_config_raw
{
  using PropertiesBitfield = properties::PropertiesBitfield;

  constexpr macro_config(const PropertiesBitfield prop)
  {
      is_copy_constructible = copy_constructible & prop ? true :
                              non_copy_constructible & prop ? false :
                              default_config.is_copy_constructible;
  }
};

/* The purpuse of below poly destruction policy is
 * to guard the storage from being destructed twice.
 * It's essential for macro exception safety of
 * assignemnt operator, which is using the
 * assign-with-placement-construction idiom, i.e:
 * T& operator=(T&& other)
 * {
 *   (&poly_)->~poly_t();
 *   new (static_cast<void*>(&poly_)) poly_t(construct_poly(std::forward<T>(other)));
 *   return *this;
 * }
 *
 * If the copy/move constructor threw an exception,
 * the poly destructor could be invoked twice:
 *   1) explicitly with (&poly_)->~poly_t();
 *   2) implicitly while unwinding stack of poly,
 *      if the exception was propagated outside the
 *      poly scope
 * Double destruction of the underlying storage is
 * undefined behavior. This policy ensures with a bool
 * flag, that the storage is destructed only once.
*/
class PolyGuardMultipleDestructionPolicy
{
  char was_destructed = 0;

protected:
  template< typename Storage, typename VTable >
  void destruct(Storage& storage, VTable& vtable)
  {
    if( not was_destructed )
    {
      storage.destruct(vtable);
      was_destructed = 1;
    }
  }
};


template< typename Concept, typename StorageType >
struct macro_traits
{
    using T1 = Concept;
    using T2 = StorageType;
    using T3 = dyno::vtable<dyno::remote<dyno::everything>>;
    using T4 = dyno::detail::PolyGuardMultipleDestructionPolicy;
    using poly_t = ::dyno::poly<T1,
                                T2,
                                T3,
                                T4 >;

    template <typename Macro, typename T, typename... Args >
    static auto construct_poly(T&& x, Args&&... argsForMake)
    {
        using RawT = std::decay_t<T>;
        constexpr bool is_RawT_a_type_of_that_macro_template =
                typename Macro:: template is_a_type_of_this_template<RawT>{};

        if constexpr(not is_RawT_a_type_of_that_macro_template)
        {
          if constexpr( dyno::detail::is_a_make_inplace<RawT> )
          {
            return poly_t{::std::forward<T>(x),
                          Macro::make_concept_map(),
                         ::std::forward<Args>(argsForMake)...};
          }
          else
          {
            static_assert(sizeof...(argsForMake) == 0,
                         "Variable argument pack is only for make_inplace<T> idiom!");
            return poly_t{::std::forward<T>(x),
                          Macro::make_concept_map()};
          }
        }
        else /* is_RawT_a_type_of_that_macro_template */
        {
          static_assert(Macro::config.is_copy_constructible or
                        not std::is_lvalue_reference_v<T>,
                        "Trying to copy or move a noncopyable object!");
          return poly_t{std::forward<T>(x).poly_};
        }
    }
};

}} // namespace dyno namespace detail

#endif //DYNO_DETAIL_MACRO_DETAILS_HPP
