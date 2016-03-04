//
// point traits
//
// Copyright (C) Peter Niekamp
//
// This code remains the property of the copyright holder.
// The code contained herein may be used for any purpose provided
// this copyright notice is retained
//

#ifndef LMLPOINT_HH
#define LMLPOINT_HH

#include <leap/lml/vector.h>

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
  // Point Traits
  template<typename Point, typename enable = void>
  struct point_traits;


  // Point Traits (Pair)
  template<typename T>
  struct point_traits<std::pair<T, T>>
  {
    static constexpr size_t dimension = 2;
  };


  // Point Traits (Array)
  template<typename T, size_t N>
  struct point_traits<std::array<T, N>>
  {
    static constexpr size_t dimension = N;
  };


  // Point Traits (VectorView Derivatives)
  template<typename T>
  struct point_traits<T, std::enable_if_t<is_vector_view<T>::value>>
  {
    static constexpr size_t dimension = T::size();
  };


  /**
   * \name Point Interface
   * \ingroup lmlgeo
   * Point Interface
   * @{
  **/


  //|///////////////////// position_type ////////////////////////////////////
  /// position_type of point
  template<typename Point>
  struct position_type
  {
    template<typename, typename = void>
    struct detail
    {
      typedef void type;
    };

    template<typename T>
    struct detail<T, std::enable_if_t<point_traits<std::decay_t<T>>::dimension>>
    {
      typedef typename std::decay_t<T> type;
    };

    template<typename T>
    struct detail<T, std::enable_if_t<point_traits<std::decay_t<decltype(position(std::declval<T&>()))>>::dimension>>
    {
      typedef std::decay_t<decltype(position(std::declval<T&>()))> type;
    };

    using type = typename detail<Point>::type;
  };

  template<typename Point>
  using position_type_t = typename position_type<Point>::type;


  //|///////////////////// coord_type ///////////////////////////////////////
  /// coord_type of point
  template<typename Point>
  using coord_type_t = std::decay_t<decltype(get<0>(std::declval<position_type_t<Point>&>()))>;


  //|///////////////////// dim //////////////////////////////////////////////
  /// dimension of point
  template<typename Point>
  constexpr auto dim() -> decltype(point_traits<position_type_t<Point>>::dimension)
  {
    return point_traits<position_type_t<Point>>::dimension;
  }


  //|///////////////////// get //////////////////////////////////////////////
  /// get position from point
  template<size_t i, typename Point>
  constexpr auto get(Point const &pt) noexcept -> decltype(get<i>(position(std::declval<Point&>())))
  {
    return get<i>(position(pt));
  }


  //|///////////////////// vec //////////////////////////////////////////////
  /// vector from point to point
  template<typename Point, size_t... I>
  auto vec(Point const &a, Point const &b, index_sequence<I...>) -> Vector<coord_type_t<Point>, dim<Point>()>
  {
    return { (get<I>(b) - get<I>(a))... };
  }

  template<typename Point>
  auto vec(Point const &a, Point const &b)
  {
    return vec(a, b, make_index_sequence<0, dim<Point>()>());
  }


  //|///////////////////// translate ////////////////////////////////////////
  /// translate point by vector
  template<typename Point, size_t... I>
  auto translate(Point const &pt, Vector<coord_type_t<Point>, dim<Point>()> const &v, index_sequence<I...>) -> Point
  {
    return { (get<I>(pt) + get<I>(v))... };
  }

  template<typename Point>
  auto translate(Point const &pt, Vector<coord_type_t<Point>, dim<Point>()> const &v)
  {
    return translate(pt, v, make_index_sequence<0, dim<Point>()>());
  }

  template<typename Point, typename T, size_t... Indices, std::enable_if_t<!std::is_same<Point, Vector<coord_type_t<Point>, dim<Point>()>>::value>* = nullptr>
  auto operator -(VectorView<Point, T, Indices...> const &pt, Vector<coord_type_t<Point>, dim<Point>()> const &v)
  {
    return translate(pt(), -v, make_index_sequence<0, dim<Point>()>());
  }

  template<typename Point, std::enable_if_t<!is_vector_view<Point>::value>* = nullptr>
  auto operator -(Point const &pt, Vector<coord_type_t<Point>, dim<Point>()> const &v)
  {
    return translate(pt, -v, make_index_sequence<0, dim<Point>()>());
  }

  template<typename Point, typename T, size_t... Indices, std::enable_if_t<!std::is_same<Point, Vector<coord_type_t<Point>, dim<Point>()>>::value>* = nullptr>
  auto operator +(VectorView<Point, T, Indices...> const &pt, Vector<coord_type_t<Point>, dim<Point>()> const &v)
  {
    return translate(pt(), v, make_index_sequence<0, dim<Point>()>());
  }

  template<typename Point, std::enable_if_t<!is_vector_view<Point>::value>* = nullptr>
  auto operator +(Point const &pt, Vector<coord_type_t<Point>, dim<Point>()> const &v)
  {
    return translate(pt, v, make_index_sequence<0, dim<Point>()>());
  }

  /**
  *  @}
  **/

} // namespace lml
} // namespace leap

#endif
