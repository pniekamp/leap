//
// lml io - lml stream io functions
//
// Copyright (C) Peter Niekamp
//
// This code remains the property of the copyright holder.
// The code contained herein may be used for any purpose provided
// this copyright notice is retained
//

#ifndef LMLIO_HH
#define LMLIO_HH

#include <iosfwd>
#include <vector>
#include <array>

namespace leap { namespace lml
{
  /**
   * \name Vector Stream IO
   * \ingroup lmlvector
   * @{
  **/

  template<typename T, size_t N> class Vector;
  template<typename Vector, typename T, size_t... Indices> class VectorView;


  //|//////////////////// operator << ///////////////////////////////////////
  /// stream operator for output of lml::VectorView
  template<typename Vector, typename T, size_t... Indices>
  std::ostream &operator <<(std::ostream &os, VectorView<Vector, T, Indices...> const &v)
  {
    T elements[] = { v[Indices]... };

    os << '(';

    if (v.size() > 0)
      os << elements[0];

    for(size_t i = 1; i < v.size(); ++ i)
      os << ',' << elements[i];

    os << ')';

    return os;
  }


  //|//////////////////// operator >> ///////////////////////////////////////
  /// stream operator for input of lml::Vector
  template<typename T, size_t N>
  std::istream &operator >>(std::istream &is, Vector<T, N> &v)
  {
    char ch;

    if (is >> ch && ch != '(')
    {
      is.putback(ch);
      is.setstate(std::ios_base::failbit);
      return is;
    }

    std::vector<T> elements;

    while (is >> ch && ch != ')')
    {
      is.putback(ch);

      T value;
      if (is >> value)
        elements.push_back(value);

      if (is >> ch && ch != ',')
      {
        is.putback(ch);

        if (ch != ')')
        {
          is.setstate(std::ios_base::failbit);
          return is;
        }
      }
    }

    if (elements.size() != N)
    {
      is.setstate(std::ios_base::failbit);
      return is;
    }

    v = Vector<T, N>(elements);

    return is;
  }


  /**
   * @}
  **/

  /**
   * \name Matrix Stream IO
   * \ingroup lmlmatrix
   * @{
  **/

  template<typename T, size_t M, size_t N, template<typename, size_t, size_t> class Backend> class Matrix;


  //|//////////////////// operator << ///////////////////////////////////////
  /// stream operator for output of lml::Matrix
  template<typename T, size_t M, size_t N, template<typename, size_t, size_t> class B>
  std::ostream &operator <<(std::ostream &os, Matrix<T, M, N, B> const &m)
  {
    os << '[';

    for(size_t i = 0; i < M; ++i)
    {
      os << '(';

      if (N > 0)
        os << m(i, 0);

      for(size_t j = 1; j < N; ++j)
        os << ',' << m(i, j);

      os << ')';
    }

    os << ']';

    return os;
  }


  //|//////////////////// operator >> ///////////////////////////////////////
  /// stream operator for input of lml::Matrix
  template<typename T, size_t M, size_t N, template<typename, size_t, size_t> class B>
  std::istream &operator >>(std::istream &is, Matrix<T, M, N, B> &m)
  {
    char ch;

    if (is >> ch && ch != '[')
    {
      is.putback(ch);
      is.setstate(std::ios_base::failbit);
      return is;
    }

    std::vector<std::vector<T>> elements;

    while (is >> ch && ch != ']')
    {
      is.putback(ch);

      Vector<T, N> column;

      if (is >> column)
        elements.push_back(std::vector<T>(column.data().begin(), column.data().end()));
    }

    if (elements.size() != M)
    {
      is.setstate(std::ios_base::failbit);
      return is;
    }

    m = Matrix<T, M, N, B>(elements);

    return is;
  }


  /**
   * @}
  **/

  /**
   * \name Quaternion Stream IO
   * \ingroup lmlquaternion
   * @{
  **/

  template<typename T> class Quaternion;


  //|//////////////////// operator << ///////////////////////////////////////
  /// stream operator for output of lml::quaternion
  template<typename T>
  std::ostream &operator <<(std::ostream &os, Quaternion<T> const &q)
  {
    os << '(' << q.w << ',' << q.x << ',' << q.y << ',' << q.z << ')';

    return os;
  }


  //|//////////////////// operator >> ///////////////////////////////////////
  /// stream operator for input of lml::quaternion
  template<typename T>
  std::istream &operator >>(std::istream &is, Quaternion<T> &q)
  {
    char ch;

    if (is >> ch && ch != '(')
    {
      is.putback(ch);
      is.setstate(std::ios_base::failbit);
      return is;
    }

    T w, x, y, z;

    is >> w;
    is >> ch;
    is >> x;
    is >> ch;
    is >> y;
    is >> ch;
    is >> z;

    if (is >> ch && ch != ')')
    {
      is.setstate(std::ios_base::failbit);
      return is;
    }

    q = Quaternion<T>(w, x, y, z);

    return is;
  }

  /**
   * @}
  **/


  /**
   * \name Bound Stream IO
   * \ingroup lmlbound
   * @{
  **/

  template<typename T, size_t N> class Bound;
  template<typename Bound, typename T, size_t Stride, size_t... Indices> struct BoundView;


  //|//////////////////// operator << ///////////////////////////////////////
  /// stream operator for output of lml::BoundView
  template<typename Bound, typename T, size_t Stride, size_t... Indices>
  std::ostream &operator <<(std::ostream &os, BoundView<Bound, T, Stride, Indices...> const &b)
  {
    T elements[2][b.size()] = { { b[0][Indices]... }, { b[1][Indices]... } };

    os << '[';

    for(size_t i = 0; i < 2; ++i)
    {
      os << '(';

      if (b.size() > 0)
        os << elements[i][0];

      for(size_t j = 1; j < b.size(); ++j)
        os << ',' << elements[i][j];

      os << ')';
    }

    os << ']';

    return os;
  }


  //|//////////////////// operator >> ///////////////////////////////////////
  /// stream operator for input of lml::Bound
  template<typename T, size_t N>
  std::istream &operator >>(std::istream &is, Bound<T, N> &b)
  {
    char ch;

    if (is >> ch && ch != '[')
    {
      is.putback(ch);
      is.setstate(std::ios_base::failbit);
      return is;
    }

    Vector<T, N> lo;
    Vector<T, N> hi;

    is >> lo;
    is >> hi;

    if (is >> ch && ch != ']')
    {
      is.setstate(std::ios_base::failbit);
      return is;
    }

    b = Bound<T, N>(lo.data(), hi.data());

    return is;
  }


  /**
   * @}
  **/

} // namespace lml
} // namespace leap

#endif
