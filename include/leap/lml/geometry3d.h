//
// geometry3d - basic 3d geometry routines
//
// Copyright (C) Peter Niekamp
//
// This code remains the property of the copyright holder.
// The code contained herein may be used for any purpose provided
// this copyright notice is retained
//

#pragma once

#include "lml.h"
#include <leap/lml/geometry.h>
#include <leap/lml/bound.h>

/**
 * \namespace leap::lml
 * \brief Leap Math Library containing mathmatical routines
 *
**/

/**
 * \defgroup lmlgeo Geometry
 * \brief Geometry Routines
 *
**/

namespace leap { namespace lml
{
  //|///////////////////// normal ///////////////////////////////////////////
  /// normal of ring (newell's method)
  template<typename InputIterator, std::enable_if_t<dim<typename std::iterator_traits<InputIterator>::value_type>() == 3>* = nullptr>
  auto normal(InputIterator f, InputIterator l)
  {
    auto result = Vector<decltype(get<0>(*f)*get<0>(*l)), 3>(0);

    for(InputIterator ic = f, ip = std::prev(l); ic != l; ip = ic, ++ic)
    {
      result(0) += (get<1>(*ip) - get<1>(*ic)) * (get<2>(*ip) + get<2>(*ic));
      result(1) += (get<2>(*ip) - get<2>(*ic)) * (get<0>(*ip) + get<0>(*ic));
      result(2) += (get<0>(*ip) - get<0>(*ic)) * (get<1>(*ip) + get<1>(*ic));
    }

    return normalise(result);
  }

  template<typename Ring, std::enable_if_t<dim<typename Ring::value_type>() == 3>* = nullptr>
  auto normal(Ring const &ring)
  {
    return normal(ring.begin(), ring.end());
  }

} // namespace lml
} // namespace leap
