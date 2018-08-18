// Copyright Łukasz Drożdż 2018
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)

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
//////////////////////////////////////////////////////////////////////////////
