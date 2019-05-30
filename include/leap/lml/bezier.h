//
// Bezier - simple bezier curve
//
// Copyright (C) Peter Niekamp
//
// This code remains the property of the copyright holder.
// The code contained herein may be used for any purpose provided
// this copyright notice is retained
//

#pragma once

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

      using point_type = Point;
      using value_type = coord_type_t<Point>;
      using control_type = leap::lml::Vector<value_type, dim<Point>()>;

    public:
      Bezier() = default;
      explicit Bezier(std::vector<Point> points, value_type k = 0.25);
      explicit Bezier(std::vector<Point> points, std::vector<control_type> controls);

      Point value(float t) const;

      value_type length() const;
      value_type length(float t0, float t1) const;

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

    for(size_t i = 1; i + 1 < n; ++i)
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
    assert(m_points.size() >= 2);
    assert(m_controls.size() != 0);

    size_t k = std::min((size_t)(t * (m_points.size()-1)), m_points.size()-2);

    auto p0 = m_points[k];
    auto p3 = m_points[k+1];
    auto p1 = p0 + m_controls[k*2];
    auto p2 = p3 + m_controls[k*2+1];

    auto u = t * (m_points.size()-1) - k;
    auto um1 = 1 - u;
    auto um13 = um1 * um1 * um1;
    auto u3 = u * u * u;

    return um13*p0 + 3*u*um1*um1*p1 + 3*u*u*um1*p2 + u3*p3;
  }


  //|///////////////////// Bezier::length ///////////////////////////////////
  template<typename Point>
  typename Bezier<Point>::value_type Bezier<Point>::length() const
  {
    return length(0.0f, 1.0f);
  }


  //|///////////////////// Bezier::length ///////////////////////////////////
  template<typename Point>
  typename Bezier<Point>::value_type Bezier<Point>::length(float t0, float t1) const
  {
    auto quadraticlength = [](Point const &p0, Point const &p1, Point const &p2) {
      auto a0 = vec(p0, p1);
      auto a1 = vec(2 * p1, p0 + p2);

      auto c = 4 * dot(a1, a1);
      auto b = 8 * dot(a0, a1);
      auto a = 4 * dot(a0, a0);
      auto q = 4 * a * c - b * b;

      return fcmp(c, value_type(0)) ? 2 * norm(a0) : 0.25 * ((2*c + b) * std::sqrt(c + b + a) - b * std::sqrt(a)) / c + q * (std::log(2 * std::sqrt(c * (c + b + a)) + 2*c + b) - std::log(2 * std::sqrt(c * a) + b)) / (8 * std::pow(c, 1.5));
    };

    auto length = value_type(0);

    size_t k0 = std::min((size_t)(t0 * (m_points.size()-1)), m_points.size()-2);
    size_t k1 = std::min((size_t)(t1 * (m_points.size()-1)), m_points.size()-2);

    for(size_t k = k0; k < k1; ++k)
    {
      auto p0 = m_points[k];
      auto p3 = m_points[k+1];
      auto p1 = p0 + m_controls[k*2];
      auto p2 = p3 + m_controls[k*2+1];

      length += quadraticlength(p0, (3 * p2 - p3 + 3 * p1 - p0) / 4, p3);
    }

    auto u = t1 * (m_points.size()-1) - k1;

    auto p0 = m_points[k1];
    auto p3 = value(t1);
    auto p1 = (1-u)*p0 + u*(p0 + m_controls[k1*2]);
    auto p2 = (1-u)*p1 + u*((1-u)*(p0 + m_controls[k1*2]) + u*(p3 + m_controls[k1*2+1]));

    length += quadraticlength(p0, (3 * p2 - p3 + 3 * p1 - p0) / 4, p3);

    return length;
  }

  //|///////////////////// remap ////////////////////////////////////
  // distance parameterisation
  template<typename Bezier>
  float remap(Bezier const &bezier, float distance)
  {
    float tlo = 0.0f;
    float thi = 1.0f;

    while (thi - tlo > 0.001f)
    {
      float t = (tlo + thi) / 2;

      if (bezier.length(0.0f, t) > distance)
        thi = t;
      else
        tlo = t;
    }

    return (thi + tlo) / 2;
  }

} // namespace lml
} // namespace leap
