//
// Bezier - simple bezier curve
//
// Copyright (C) Peter Niekamp
//
// This code remains the property of the copyright holder.
// The code contained herein may be used for any purpose provided
// this copyright notice is retained
//

#ifndef LMLBEZIER_HH
#define LMLBEZIER_HH

#include <vector>
#include <leap/lml/point.h>
#include <leap/lml/geometry.h>
#include <cassert>

namespace leap { namespace lml
{

  //|-------------------- Bezier --------------------------------------------
  //|------------------------------------------------------------------------
  /**
   * \ingroup lmlinterp
   *
   * \brief A simple piecewise bezier handler
   *
   * \see leap::lml::Interpolator
  **/

  template<typename Point>
  class Bezier
  {
    public:

      typedef Point point_type;
      typedef coord_type_t<Point> value_type;
      typedef leap::lml::Vector<value_type, 2> control_type;

    public:
      Bezier() = default;
      Bezier(std::vector<Point> points, value_type k = 0.25);
      Bezier(std::vector<Point> points, std::vector<control_type> controls);

      Point value(float t) const;

    public:

      std::vector<point_type> const &points() const { return m_points; }
      std::vector<control_type> const &controls() const { return m_controls; }

    private:

      std::vector<point_type> m_points;
      std::vector<control_type> m_controls;
  };


  //|///////////////////// Bezier::Constructor //////////////////////////////
  /// Calculate a Bezier through the supplied points
  template<typename Point>
  Bezier<Point>::Bezier(std::vector<Point> points, value_type k)
  {
    assert(points.size() > 1);

    size_t n = points.size();

    //
    // Calculate the control points
    //    

    std::vector<control_type> R(2*n-2);

    // First Point

    R[0] = k * vec(points[0], points[1]);

    // Middle Points

    for(size_t i = 1; i < n-1; ++i)
    {
      R[2*i-1] = k * vec(points[i+1], points[i-1]);
      R[2*i] = k * vec(points[i-1], points[i+1]);
    }

    // Last Point

    R[2*n-3] = k * vec(points[n-1], points[n-2]);

    // Store

    m_controls = std::move(R);

    m_points = std::move(points);
  }


  //|///////////////////// Bezier::Constructor //////////////////////////////
  template<typename Point>
  Bezier<Point>::Bezier(std::vector<Point> points, std::vector<control_type> controls)
  {
    assert(points.size() > 1 && controls.size() == 2*points.size()-2);

    m_points = std::move(points);
    m_controls = std::move(controls);
  }


  //|///////////////////// Bezier::value ////////////////////////////////////
  template<typename Point>
  Point Bezier<Point>::value(float t) const
  {
    assert(m_controls.size() != 0);

    size_t k = (size_t)(t * (m_points.size()-1));

    auto x0 = get<0>(m_points[k]);
    auto x3 = get<0>(m_points[k+1]);
    auto x1 = x0 + m_controls[k*2](0);
    auto x2 = x3 + m_controls[k*2+1](0);

    auto y0 = get<1>(m_points[k]);
    auto y3 = get<1>(m_points[k+1]);
    auto y1 = y0 + m_controls[k*2](1);
    auto y2 = y3 + m_controls[k*2+1](1);

    auto u = t * (m_points.size()-1) - k;
    auto um1 = 1 - u;
    auto um13 = um1 * um1 * um1;
    auto u3 = u * u * u;

    auto x = um13*x0 + 3*u*um1*um1*x1 + 3*u*u*um1*x2 + u3*x3;
    auto y = um13*y0 + 3*u*um1*um1*y1 + 3*u*u*um1*y2 + u3*y3;

    return { x, y };
  }

} // namespace lml
} // namespace leap

#endif
