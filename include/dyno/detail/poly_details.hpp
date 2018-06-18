#ifndef DYNO_DETAIL_POLY_DETAILS_HPP
#define DYNO_DETAIL_POLY_DETAILS_HPP

namespace dyno::detail
{

class PolyDefaultDestructionPolicy
{
protected:
  template< typename Storage, typename VTable >
  static void destruct(Storage& storage, VTable& vtable)
  {
    storage.destruct(vtable);
  }
};

}

#endif // DYNO_DETAIL_POLY_DETAILS_HPP
