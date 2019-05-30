//
// geometry2d - basic 2d geometry routines
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
  // Ring Traits
  template<typename Ring, typename enable = void>
  struct ring_traits;

  // Ring Traits (std::vector<Point>)
  template<typename Point>
  struct ring_traits<std::vector<Point>, std::enable_if_t<point_traits<Point>::dimension != 0>>
  {
	static constexpr size_t dimension = dim<Point>();

    static constexpr int orientation = 1;
  };


  //|///////////////////// dim //////////////////////////////////////////////
  /// dimension of ring
  template<typename Ring>
  constexpr auto dim() -> decltype(ring_traits<Ring>::dimension)
  {
	  return ring_traits<Ring>::dimension;
  }


  //|///////////////////// area /////////////////////////////////////////////
  /// area of xy ring
  template<typename Ring, size_t dimension = dim<Ring>(), std::enable_if_t<dimension == 2>* = nullptr>
  auto area(Ring const &ring)
  {
    using std::begin;
    using std::end;
    using std::prev;
    using std::next;

    auto result = decltype(get<0>(*begin(ring))*get<0>(*end(ring)))(0);

    for (auto ic = begin(ring), ip = prev(end(ring)); ic != end(ring); ip = ic, ++ic)
	{
	  result += get<0>(*ip) * get<1>(*ic) - get<0>(*ic) * get<1>(*ip);
	}

	return std::abs(result) / 2;
  }


  //|///////////////////// orientation //////////////////////////////////////
  /// orientation of xy ring
  template<typename Ring, size_t dimension = dim<Ring>(), std::enable_if_t<dimension == 2>* = nullptr>
  auto orientation(Ring const &ring)
  {
    using std::begin;
    using std::end;
    using std::prev;
    using std::next;

    auto result = decltype(get<0>(*begin(ring))*get<0>(*end(ring)))(0);

    for (auto ic = begin(ring), ip = prev(end(ring)); ic != end(ring); ip = ic, ++ic)
	{
	  result += get<0>(*ip) * get<1>(*ic) - get<0>(*ic) * get<1>(*ip);
	}

	return fcmp(result, decltype(result)(0)) ? 0 : result;
  }


  //|///////////////////// contains /////////////////////////////////////////
  /// ring contains point
  template<typename Ring, typename Point, size_t dimension = dim<Ring>(), std::enable_if_t<dimension == 2>* = nullptr>
  bool contains(Ring const &ring, Point const &pt)
  {
    using std::begin;
    using std::end;
    using std::prev;
    using std::next;

    int count = 0;
    for (auto ic = begin(ring), ip = prev(end(ring)); ic != end(ring); ip = ic, ++ic)
    {
      auto x1 = get<0>(*ip);
      auto x2 = get<0>(*ic);
      auto y1 = get<1>(*ip);
      auto y2 = get<1>(*ic);

      if ((y1 <= get<1>(pt) && y2 > get<1>(pt)) || (y1 > get<1>(pt) && y2 <= get<1>(pt)))
      {
        if (get<0>(pt) < x1 + (get<1>(pt) - y1) / (y2 - y1) * (x2 - x1))
          ++count;
      }
    }

    return count & 1;
  }


  //|///////////////////// nearest_on_polygon ///////////////////////////////
  /// nearest point on ring edge
  template<typename Ring, typename Point, size_t dimension = dim<Ring>(), std::enable_if_t<dimension == 2>* = nullptr>
  auto nearest_on_polygon(Ring const &ring, Point const &pt)
  {
    using std::begin;
    using std::end;
    using std::prev;
    using std::next;

    auto result = Point{};

    auto mindist = std::numeric_limits<decltype(distsqr(result, result))>::max();

    for (auto ic = begin(ring), ip = prev(end(ring)); ic != end(ring); ip = ic, ++ic)
    {
      auto np = nearest_on_segment(*ip, *ic, pt);

      auto dist = distsqr(np, pt);

      if (dist < mindist)
      {
        result = np;
        mindist = dist;
      }
    }

    return result;
  }


  //|///////////////////// nearest_in_polygon ///////////////////////////////
  /// nearest point on or within ring
  template<typename Ring, typename Point, size_t dimension = dim<Ring>(), std::enable_if_t<dimension == 2>* = nullptr>
  auto nearest_in_polygon(Ring const &ring, Point const &pt)
  {
    return contains(ring, pt) ? pt : nearest_on_polygon(ring, pt);
  }


  //|/////////////////////// is_convex //////////////////////////////////////
  /// test ring convex
  template<typename InputIterator>
  bool is_convex(InputIterator f, InputIterator l)
  {
    using std::prev;
    using std::next;

    if (f != l && next(f) != l && next(next(f)) != l)
    {
      auto initial = orientation(*f, *next(f), *next(next(f)));

      for(auto ic = f, ib = prev(l), ia = prev(prev(l)); ic != l; ia = ib, ib = ic, ++ic)
      {
        if (initial * orientation(*ia, *ib, *ic) < 0)
          return false;
      }
    }

    return true;
  }

  template<typename Ring, size_t dimension = dim<Ring>(), std::enable_if_t<dimension == 2>* = nullptr>
  bool is_convex(Ring const &ring)
  {
    using std::begin;
    using std::end;

    return is_convex(begin(ring), end(ring));
  }


  //|/////////////////////// convex_hull ////////////////////////////////////
  /// convex hull
  template<typename Ring, typename InputIterator>
  Ring convex_hull_sorted(InputIterator f, InputIterator l)
  {
    int k = 0;
    int n = std::distance(f, l);

    Ring R(2*n);

    for(auto i = f; i != l; ++i)
    {
      while (k >= 2 && orientation(R[k-2], R[k-1], *i) <= 0)
        --k;

      R[k++] = *i;
    }

    int t = k+1;

    for(auto i = std::prev(l, std::min(n, 2)); i != f; --i)
    {
      while (k >= t && orientation(R[k-2], R[k-1], *i) <= 0)
        --k;

      R[k++] = *i;
    }

    while (k >= t && orientation(R[k-2], R[k-1], *f) <= 0)
      --k;

    R.resize(k);

    return R;
  }

  template<typename Points>
  auto convex_hull(Points &&points, std::false_type) // rvalue
  {
    using std::begin;
    using std::end;

    std::sort(begin(points), end(points), [](auto const &lhs, auto const &rhs) { return (get<0>(lhs) == get<0>(rhs)) ? (get<1>(lhs) < get<1>(rhs)) : (get<0>(lhs) < get<0>(rhs)); });

    return convex_hull_sorted<Points>(begin(points), end(points));
  }

  template<typename Points>
  auto convex_hull(Points &&points, std::true_type) // lvalue
  {
    return convex_hull(std::decay_t<Points>(points), std::false_type());
  }

  template<typename Points>
  auto convex_hull(Points &&points)
  {
    return convex_hull(std::forward<Points>(points), std::is_lvalue_reference<Points>());
  }


} // namespace lml
} // namespace leap


//---------------------- Polygon Set Ops ------------------------------------
//---------------------------------------------------------------------------

#include "polygonsetop_p.h"

namespace leap { namespace lml
{
  /**
   * \name Polygon Set Operations
   * \ingroup lmlgeo
   * Polygon Set Operations
   * @{
  **/


  //|///////////////////// boolean_union ////////////////////////////////////
  /// polygon union
  template<typename Ring, typename Point = typename std::iterator_traits<decltype(std::declval<Ring>().begin())>::value_type>
  std::vector<Ring> boolean_union(Ring const &p, Ring const &q)
  {
    using namespace PolygonSetOp;

    Graph<Point> graph(p.size(), q.size());

    graph.push_p(p.begin(), p.end());
    graph.push_q(q.begin(), q.end());

    graph.join();

    std::vector<Ring> result;

    polygon_setop(&result, graph, p, q, Op::Union);

    return result;
  }


  //|///////////////////// boolean_intersection /////////////////////////////
  /// polygon intersection
  template<typename Ring, typename Point = typename std::iterator_traits<decltype(std::declval<Ring>().begin())>::value_type>
  std::vector<Ring> boolean_intersection(Ring const &p, Ring const &q)
  {
    using namespace PolygonSetOp;

    Graph<Point> graph(p.size(), q.size());

    graph.push_p(p.begin(), p.end());
    graph.push_q(q.begin(), q.end());

    graph.join();

    std::vector<Ring> result;

    polygon_setop(&result, graph, p, q, Op::Intersection);

    return result;
  }


  //|///////////////////// boolean_difference ///////////////////////////////
  /// polygon difference
  template<typename Ring, typename Point = typename std::iterator_traits<decltype(std::declval<Ring>().begin())>::value_type>
  std::vector<Ring> boolean_difference(Ring const &p, Ring const &q)
  {
    using namespace PolygonSetOp;

    Graph<Point> graph(p.size(), q.size());

    graph.push_p(p.begin(), p.end());
    graph.push_q(std::reverse_iterator<decltype(q.end())>(q.end()), std::reverse_iterator<decltype(q.begin())>(q.begin()));

    graph.join();

    std::vector<Ring> result;

    polygon_setop(&result, graph, p, q, Op::Difference);

    return result;
  }

  /**
  *  @}
  **/

} // namespace lml
} // namespace leap


//---------------------- Polygon Simplify -----------------------------------
//---------------------------------------------------------------------------

#include "polygonsimplify_p.h"

namespace leap { namespace lml
{
  /**
   * \name Polygon Simplify Operations
   * \ingroup lmlgeo
   * Polygon Simplify Operations
   * @{
  **/

  //|/////////////////////// is_simple //////////////////////////////////////
  /// test ring simple
  template<typename Ring, typename Point = typename std::iterator_traits<decltype(std::declval<Ring>().begin())>::value_type>
  bool is_simple(Ring const &p)
  {
    using namespace PolygonSimplify;

    Graph<Point> graph(p.size());

    graph.push_p(p.begin(), p.end());

    graph.join();

    return graph.intersectsp().empty();
  }


  //|///////////////////// boolean_simplify /////////////////////////////////
  /// polygon simplify (divide at self intersections)
  template<typename Ring, typename Point = typename std::iterator_traits<decltype(std::declval<Ring>().begin())>::value_type>
  std::vector<Ring> boolean_simplify(Ring const &p)
  {
    using namespace PolygonSimplify;

    Graph<Point> graph(p.size());

    graph.push_p(p.begin(), p.end());

    graph.join();

    std::vector<PolygonSimplify::Ring<Point>> rings;

    polygon_simplify(&rings, graph);

    std::vector<Ring> result;

    for(auto &ring : rings)
    {
      result.push_back(make_ring<Ring>(ring));

      if ((std::abs(ring.winding) % 2 == 1 && ring.orientation != ring_traits<Ring>::orientation) || (std::abs(ring.winding) % 2 == 0 && ring.orientation != -ring_traits<Ring>::orientation))
        std::reverse(result.back().begin(), result.back().end());
    }

    return result;
  }

  /**
  *  @}
  **/

} // namespace lml
} // namespace leap


//---------------------- Delaunay -------------------------------------------
//---------------------------------------------------------------------------

#include "delaunay2d_p.h"
#include "voronoi2d_p.h"
