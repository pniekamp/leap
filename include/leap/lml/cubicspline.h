//
// cubicspline - simple cubic spline
//
// Copyright (C) Peter Niekamp
//
// This code remains the property of the copyright holder.
// The code contained herein may be used for any purpose provided
// this copyright notice is retained
//

#ifndef LMLCUBICSPLINE_HH
#define LMLCUBICSPLINE_HH

#include <vector>
#include <leap/lml/point.h>
#include <leap/lml/geometry.h>
#include <cassert>

namespace leap { namespace lml
{

  //|-------------------- CubicSpline ---------------------------------------
  //|------------------------------------------------------------------------
  /**
   * \ingroup lmlinterp
   *
   * \brief A simple piecewise cubic spline handler
   *
   * \see leap::lml::Interpolator
  **/

  template<typename Point>
  class CubicSpline
  {
    public:

      typedef Point point_type;
      typedef coord_type_t<Point> value_type;

    public:
      CubicSpline() = default;
      CubicSpline(std::vector<Point> points, value_type initial = 1e30, value_type final = 1e30);

      auto value(value_type x) const;
      auto derivative(value_type x) const;

    public:

      std::vector<point_type> const &points() const { return m_points; }

    private:

      std::vector<point_type> m_points;
      std::vector<value_type> m_c;
  };


  //|///////////////////// CubicSpline::Constructor /////////////////////////
  ///
  /// Calculate a cubic spline through the supplied points
  ///
  /// \param[in] points coordinates
  /// \param[in] initial initial boundary condition or 1e30 for natural condition
  /// \param[in] final final boundary condition or 1e30 for natural condition
  ///
  template<typename Point>
  CubicSpline<Point>::CubicSpline(std::vector<Point> points, value_type initial, value_type final)
  {
    assert(points.size() > 1);

    size_t n = points.size();

    std::vector<value_type> R(n);
    std::vector<value_type> u(n);

    if (initial > 0.99e30)
    {
      R[0] = u[0] = 0;
    }
    else
    {
      R[0] = -0.5;
      u[0] = (3/(get<0>(points[1]) - get<0>(points[0]))) * (slope(points[0], points[1]) - initial);
    }

    for(size_t i = 1; i <= n-2; ++i)
    {
      value_type sig = (get<0>(points[i]) - get<0>(points[i-1]))/(get<0>(points[i+1]) - get<0>(points[i-1]));
      value_type p = sig * R[i-1] + 2;

      R[i] = (sig - 1)/p;

      u[i] = slope(points[i], points[i+1]) - slope(points[i-1], points[i]);
      u[i] = (6*u[i]/(get<0>(points[i+1]) - get<0>(points[i-1])) - sig*u[i-1])/p;
    }

    if (final > 0.99e30)
    {
      R[n-1] = 0;
    }
    else
    {
      value_type qn = 0.5;
      value_type un = (3/(get<0>(points[n-1]) - get<0>(points[n-2]))) * (final - slope(points[n-2], points[n-1]));

      R[n-1] = (un - qn * u[n-2]) / (qn * R[n-2] + 1);
    }

    for(int k = n-2; k >= 0; --k)
      R[k] = R[k] * R[k+1] + u[k];

    m_c = std::move(R);

    m_points = std::move(points);
  }


  //|///////////////////// CubicSpline::value ///////////////////////////////
  template<typename Point>
  auto CubicSpline<Point>::value(value_type x) const
  {
    assert(m_c.size() != 0);

    size_t klo = 0;
    size_t khi = m_points.size()-1;

    while (khi - klo > 1)
    {
      size_t k = (klo + khi) / 2;

      if (get<0>(m_points[k]) > x)
        khi = k;
      else
        klo = k;
    }

    auto dx = get<0>(m_points[khi]) - get<0>(m_points[klo]);

    auto a = (get<0>(m_points[khi]) - x) / dx;
    auto b = (x - get<0>(m_points[klo])) / dx;

    return a * get<1>(m_points[klo]) + b * get<1>(m_points[khi]) + ((a * a * a - a) * m_c[klo] + (b * b * b - b) *  m_c[khi]) * (dx * dx)/6;
  }


  //|///////////////////// CubicSpline::derivative //////////////////////////
  template<typename Point>
  auto CubicSpline<Point>::derivative(value_type x) const
  {
    assert(m_c.size() != 0);

    size_t klo = 0;
    size_t khi = m_points.size()-1;

    while (khi - klo > 1)
    {
      size_t k = (klo + khi) / 2;

      if (get<0>(m_points[k]) > x)
        khi = k;
      else
        klo = k;
    }

    auto dx = get<0>(m_points[khi]) - get<0>(m_points[klo]);
    auto dy = get<1>(m_points[khi]) - get<1>(m_points[klo]);

    auto a = (get<0>(m_points[khi]) - x) / dx;
    auto b = (x - get<0>(m_points[klo])) / dx;

    return dy/dx - (3*a*a-1)/6*dx*m_c[klo] + (3*b*b-1)/6*dx*m_c[khi];
  }


} // namespace lml
} // namespace leap

#endif
