//
// interpolation - mathmatical interpolation routines
//
// Copyright (C) Peter Niekamp
//
// This code remains the property of the copyright holder.
// The code contained herein may be used for any purpose provided
// this copyright notice is retained
//

#pragma once

#include <leap/lml/array.h>
#include <cassert>
#include <cstddef>
#include <vector>
#include <algorithm>
#include <memory>
#include <array>

/**
 * \namespace leap::lml
 * \brief Leap Math Library containing mathmatical routines
 *
**/

/**
 * \defgroup lmlinterp Mathmatical Interpolation Routines
 * \brief Interpolation routines
 *
**/

namespace leap { namespace lml
{
  //|-------------------- Interpolation -------------------------------------
  //|------------------------------------------------------------------------

  enum InterpolationType { linear = 1, cosine = 2, cubic = 3 };

  //|///////////////////// interpolate linear ///////////////////////////////
  template<InterpolationType type, class ForwardIterator, class Iterator, std::enable_if_t<type == linear>* = nullptr>
  auto interpolate(ForwardIterator xfirst, ForwardIterator xlast, Iterator yfirst, typename std::iterator_traits<ForwardIterator>::value_type x)
  {
    if (xfirst == xlast)
      return typename std::iterator_traits<Iterator>::value_type();

    if (xfirst+1 == xlast)
      return *yfirst;

    auto x1 = std::upper_bound(xfirst+1, xlast-1, x) - 1;
    auto x2 = x1+1;

    auto y1 = yfirst + (x1 - xfirst);
    auto y2 = y1+1;

    auto mu = (x-*x1)/(*x2-*x1);

    return (*y1)*(1-mu) + (*y2)*(mu);
  }


  //|///////////////////// interpolate cosine ///////////////////////////////
  template<InterpolationType type, class ForwardIterator, class Iterator, std::enable_if_t<type == cosine>* = nullptr>
  auto interpolate(ForwardIterator xfirst, ForwardIterator xlast, Iterator yfirst, typename std::iterator_traits<ForwardIterator>::value_type x)
  {
    if (xfirst == xlast)
      return typename std::iterator_traits<Iterator>::value_type();

    if (xfirst+1 == xlast)
      return *yfirst;

    auto x1 = std::upper_bound(xfirst+1, xlast-1, x) - 1;
    auto x2 = x1+1;

    auto y1 = yfirst + (x1 - xfirst);
    auto y2 = y1+1;

    auto mu = (1-cos((x-*x1)/(*x2-*x1)*3.14159265))/2;

    return (*y1)*(1-mu) + (*y2)*(mu);
  }


  //|///////////////////// interpolate cubic ////////////////////////////////
  template<InterpolationType type, class ForwardIterator, class Iterator, std::enable_if_t<type == cubic>* = nullptr>
  auto interpolate(ForwardIterator xfirst, ForwardIterator xlast, Iterator yfirst, typename std::iterator_traits<ForwardIterator>::value_type x)
  {
    if (std::distance(xfirst, xlast) < 3)
      return interpolate<linear>(xfirst, xlast, yfirst, x);

    auto x1 = std::upper_bound(xfirst+1, xlast-1, x) - 1;
    auto x2 = x1+1;

    auto y1 = yfirst + (x1 - xfirst);
    auto y2 = y1+1;

    auto d1 = (x1 == xfirst) ? (*y2-*y1) : (*(y2)-*(y1-1))/(*(x2)-*(x1-1))*(*x2-*x1);
    auto d2 = (x2+1 == xlast) ? (*y2-*y1) : (*(y2+1)-*(y1))/(*(x2+1)-*(x1))*(*x2-*x1);

    auto mu = (x-*x1)/(*x2-*x1);

    auto A = 2*(*y1) - 2*(*y2) + d1 + d2;
    auto B = -3*(*y1) + 3*(*y2) - d1 - d1 - d2;
    auto C = d1;
    auto D = *y1;

    return A*mu*mu*mu + B*mu*mu + C*mu + D;
  }


  //|///////////////////// interpolate //////////////////////////////////////
  template<InterpolationType type, typename P, typename Q>
  auto interpolate(P const &xa, Q const &ya, typename std::decay_t<P>::value_type const &x)
  {
    return interpolate<type>(begin(xa), end(xa), begin(ya), x);
  }


  //|///////////////////// interpolate linear ///////////////////////////////
  namespace InterpolateLinearImpl
  {
    template<size_t dimension, typename T>
    T interpolate(ArrayView<T const *, 1> const &ya, std::pair<size_t, size_t> const *idx, double const *mu)
    {
      if (idx->first+1 == idx->second)
        return ya[0];

      return (ya[idx->first])*(1-*mu) + (ya[idx->first+1])*(*mu);
    }

    template<size_t dimension, typename T>
    T interpolate(ArrayView<T const *, dimension> const &ya, std::pair<size_t, size_t> const *idx, double const *mu)
    {
      std::array<T, 2> yaa = {};

      for(auto i = idx->first; i != idx->second; ++i)
        yaa[i-idx->first] = interpolate<dimension-1>(ya[i], idx+1, mu+1);

      if (idx->first+1 == idx->second)
        return yaa[0];

      return (yaa[0])*(1-*mu) + (yaa[1])*(*mu);
    }
  }

  template<InterpolationType type, typename T, size_t dimension, std::enable_if_t<type == linear>* = nullptr>
  T interpolate(std::vector<double> const (&xa)[dimension], lml::Array<T, dimension> const &ya, double const (&x)[dimension])
  {
    double mu[dimension];
    std::pair<size_t, size_t> idx[dimension];

    for(size_t i = 0; i < dimension; ++i)
    {
      if (xa[i].size() < 2)
      {
        idx[i].first = 0;
        idx[i].second = 1;
        continue;
      }

      auto k = upper_bound(xa[i].begin()+1, xa[i].end()-1, x[i]) - 1;

      idx[i].first = k - xa[i].begin();
      idx[i].second = k - xa[i].begin() + 2;

      mu[i] = (x[i] - *k) / (*(k+1) - *k);
    }

    return InterpolateLinearImpl::interpolate<dimension, T>(ya, idx, mu);
  }


  //|///////////////////// interpolate cosine ///////////////////////////////
  template<InterpolationType type, typename T, size_t dimension, std::enable_if_t<type == cosine>* = nullptr>
  T interpolate(std::vector<double> const (&xa)[dimension], lml::Array<T, dimension> const &ya, double const (&x)[dimension])
  {
    throw std::runtime_error("Not Implemented");
  }


  //|///////////////////// interpolate cubic ///////////////////////////////
  namespace InterpolateCubicImpl
  {
    template<size_t dimension, typename T>
    T interpolate(std::vector<double> const *xa, ArrayView<T const *, 1> const &ya, std::pair<size_t, size_t> const *idx, double const *x)
    {
      return lml::interpolate<cubic>(xa->begin() + idx->first, xa->begin() + idx->second, ya.begin() + idx->first, *x);
    }

    template<size_t dimension, typename T>
    T interpolate(std::vector<double> const *xa, ArrayView<T const *, dimension> const &ya, std::pair<size_t, size_t> const *idx, double const *x)
    {
      std::array<T, 4> yaa;

      for(auto i = idx->first; i != idx->second; ++i)
        yaa[i-idx->first] = interpolate<dimension-1>(xa+1, ya[i], idx+1, x+1);

      return lml::interpolate<cubic>(xa->begin() + idx->first, xa->begin() + idx->second, yaa.begin(), *x);
    }
  }

  template<InterpolationType type, typename T, size_t dimension, std::enable_if_t<type == cubic>* = nullptr>
  T interpolate(std::vector<double> const (&xa)[dimension], lml::Array<T, dimension> const &ya, double const (&x)[dimension])
  {
    std::pair<size_t, size_t> idx[dimension];

    for(size_t i = 0; i < dimension; ++i)
    {
      if (xa[i].size() < 2)
      {
        idx[i].first = 0;
        idx[i].second = 1;
        continue;
      }

      auto k = upper_bound(xa[i].begin()+1, xa[i].end()-1, x[i]) - 1;

      idx[i].first = std::max<int>(0, k - xa[i].begin() - 1);
      idx[i].second = std::min<int>(k - xa[i].begin() + 3, xa[i].size());
    }

    return InterpolateCubicImpl::interpolate<dimension, T>(xa, ya, idx, x);
  }


  //|///////////////////// interpolate //////////////////////////////////////
  template<typename P, typename Q, typename X>
  auto interpolate(P &&xa, Q &&ya, X &&x, InterpolationType type)
  {
    switch (type)
    {
      case linear:
        return interpolate<linear>(std::forward<P>(xa), std::forward<Q>(ya), std::forward<X>(x));

      case cosine:
        return interpolate<cosine>(std::forward<P>(xa), std::forward<Q>(ya), std::forward<X>(x));

      case cubic:
        return interpolate<cubic>(std::forward<P>(xa), std::forward<Q>(ya), std::forward<X>(x));
    }
  }

} // namespace lml
} // namespace leap
