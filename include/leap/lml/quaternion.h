//
// quaternion - mathmatical quaternion class
//
// Copyright (C) Peter Niekamp
//
// This code remains the property of the copyright holder.
// The code contained herein may be used for any purpose provided
// this copyright notice is retained
//

#ifndef QUATERNION_HH
#define QUATERNION_HH

#include <leap/util.h>
#include <leap/lml/vector.h>


/**
 * \namespace leap::lml
 * \brief Leap Math Library containing mathmatical routines
 *
**/

/**
 * \defgroup lmlquaternion Mathmatical Quaternion Class
 * \brief Quaternion class and mathmatical routines
 *
**/

namespace leap { namespace lml
{

  //|-------------------- Quaternion ----------------------------------------
  //|------------------------------------------------------------------------
  /**
   * \ingroup lmlquaternion
   *
   * \brief Mathmatical Quaternion Class
   *
  **/

  template<typename T>
  class Quaternion : public VectorView<Quaternion<T>, T, 0, 1, 2, 3>
  {
    public:

      typedef T scalar_type;
      typedef Vector<T, 3> vector_type;
      typedef T& reference;
      typedef const T& const_reference;

    public:
      Quaternion() = default;
      constexpr Quaternion(T w, T x, T y, T z);
      explicit constexpr Quaternion(scalar_type w, vector_type const &vector);
      explicit constexpr Quaternion(vector_type const &axis, scalar_type angle);
      explicit constexpr Quaternion(vector_type const &xaxis, vector_type const &yaxis, vector_type const &zaxis);

      union
      {
        struct
        {
          T w;
          T x;
          T y;
          T z;
        };

        struct
        {
          scalar_type scalar;
          vector_type vector;
        };
      };

      // Euler
      scalar_type ax() const { return std::atan2(2*(y*z + x*w), 1 - 2*(x*x + z*z)); }
      scalar_type ay() const { return std::atan2(2*(x*z + y*w), 1 - 2*(x*x + y*y)); }
      scalar_type az() const { return std::atan2(2*(x*y + z*w), 1 - 2*(y*y + z*z)); }

      // Basis
      vector_type xaxis() const { return { 1 - 2*(y*y + z*z), 2*(x*y + z*w), 2*(x*z + y*w) }; }
      vector_type yaxis() const { return { 2*(x*y - z*w), 1 - 2*(x*x + z*z), 2*(y*z + x*w) }; }
      vector_type zaxis() const { return { 2*(x*z + y*w), 2*(y*z - x*w), 1 - 2*(x*x + y*y) }; }
  };


  //|///////////////////// Quaternion::Constructor //////////////////////////
  template<typename T>
  constexpr Quaternion<T>::Quaternion(T w, T x, T y, T z)
    : scalar(w),
      vector({ x, y, z })
  {
  }

  //|///////////////////// Quaternion::Constructor //////////////////////////
  template<typename T>
  constexpr Quaternion<T>::Quaternion(scalar_type w, vector_type const &vector)
    : scalar(w),
      vector(vector)
  {
  }


  //|///////////////////// Quaternion::Constructor //////////////////////////
  /// axis should be a unit vector
  template<typename T>
  constexpr Quaternion<T>::Quaternion(vector_type const &axis, scalar_type angle)
    : scalar(std::cos(T(0.5)*angle)),
      vector(axis * std::sin(T(0.5)*angle))
  {
  }


  //|///////////////////// Quaternion::Constructor //////////////////////////
  /// basis axis
  template<typename T>
  constexpr Quaternion<T>::Quaternion(vector_type const &xaxis, vector_type const &yaxis, vector_type const &zaxis)
  {
    if (xaxis(0) + yaxis(1) + zaxis(2) > T(0))
    {
      auto s = std::sqrt(xaxis(0) + yaxis(1) + zaxis(2) + T(1));
      auto t = T(0.5) / s;

      x = (yaxis(2) - zaxis(1)) * t;
      y = (zaxis(0) - xaxis(2)) * t;
      z = (xaxis(1) - yaxis(0)) * t;
      w = T(0.5) * s;
    }
    else if (xaxis(0) > yaxis(1) && xaxis(0) > zaxis(2))
    {
      auto s = std::sqrt(xaxis(0) - yaxis(1) - zaxis(2) + T(1));
      auto t = T(0.5) / s;

      x = T(0.5) * s;
      y = (yaxis(0) + xaxis(1)) * t;
      z = (xaxis(2) + zaxis(0)) * t;
      w = (yaxis(2) - zaxis(1)) * t;
    }
    else if (yaxis(1) > zaxis(2))
    {
      auto s = std::sqrt(-xaxis(0) + yaxis(1) - zaxis(2) + T(1));
      auto t = T(0.5) / s;

      x = (yaxis(0) + xaxis(1)) * t;
      y = T(0.5) * s;
      z = (zaxis(1) + yaxis(2)) * t;
      w = (zaxis(0) - xaxis(2)) * t;
    }
    else
    {
      auto s = std::sqrt(-xaxis(0) - yaxis(1) + zaxis(2) + T(1));
      auto t = T(0.5) / s;

      x = (zaxis(0) + xaxis(2)) * t;
      y = (zaxis(1) + yaxis(2)) * t;
      z = T(0.5) * s;
      w = (xaxis(1) - yaxis(0)) * t;
    }
  }



  /**
   * \name General Quaternion Operations
   * \ingroup lmlquaternion
   * Generic operations for quaternions
   * @{
  **/


  //|///////////////////// conjugate ////////////////////////////////////////
  /// Conjugate a quaternion
  template<typename T>
  constexpr Quaternion<T> conjugate(Quaternion<T> const &q)
  {
    return Quaternion<T>(q.w, -q.x, -q.y, -q.z);
  }


  //|///////////////////// operator * ///////////////////////////////////////
  /// Quaternion Multiplication
  template<typename T>
  constexpr Quaternion<T> operator *(Quaternion<T> const &q1, Quaternion<T> const &q2)
  {
    return Quaternion<T>(q1.w*q2.w - dot(q1.vector, q2.vector), q1.w*q2.vector + q2.w*q1.vector + cross(q1.vector, q2.vector));
  }


  //|///////////////////// Quaternion::rotate ///////////////////////////////
  /// Quaternion Vector Rotation
  template<typename Vector, typename T, size_t... Indices, std::enable_if_t<sizeof...(Indices) == 3>* = nullptr>
  constexpr Vector operator *(Quaternion<T> const &q, VectorView<Vector, T, Indices...> const &v)
  {
    auto result = (q * Quaternion<T>(0, get<0>(v), get<1>(v), get<2>(v)) * conjugate(q));

    return { result.x, result.y, result.z };
  }


  //|///////////////////// Quaternion::slerp ////////////////////////////////
  /// Quaternion slerp
  template<typename T>
  Quaternion<T> slerp(Quaternion<T> const &lower, Quaternion<T> const &upper, T alpha)
  {
    auto costheta = dot(lower, upper);

    T flip = std::copysign(1, costheta);

    if (costheta < -T(0.95) || costheta > T(0.95))
      return lerp(lower, flip*upper, alpha);

    auto theta = std::acos(flip*costheta);

    return (std::sin(theta*(1-alpha))*lower + std::sin(theta*alpha)*flip*upper) / std::sin(theta);
  }


  /**
   *  @}
  **/


  /**
   * \name Misc Quaternion
   * \ingroup lmlquaternion
   * Quaternion helpers
   * @{
  **/

  typedef Quaternion<float> Quaternion3f;
  typedef Quaternion<double> Quaternion3d;


  /**
   *  @}
  **/

} // namespace lml
} // namespace leap

#endif
