//
// simplematrix - mathmatical matrix class
//
// Copyright (C) Peter Niekamp
//
// This code remains the property of the copyright holder.
// The code contained herein may be used for any purpose provided
// this copyright notice is retained
//

#ifndef LMLSIMPLEMATRIX_HH
#define LMLSIMPLEMATRIX_HH

#include <array>

namespace leap { namespace lml
{

  //|-------------------- SimpleMatrix --------------------------------------
  //|------------------------------------------------------------------------
  /**
   * \ingroup lmlmatrix
   *
   * \brief A backend implementation of a mathmatical matrix class.
   *
   * uses std::array to store matrix data (column-major)
   *
   * This class is intended to be used through the lml::matrix class.
   * \code
   *   lml::matrix<float>
   * \endcode
   * \see leap::lml::matrix
  **/

  template<typename T, size_t M, size_t N>
  class SimpleMatrix
  {
    public:

      typedef size_t size_type;
      typedef std::array<std::array<T, M>, N> data_type;
      typedef T& reference;
      typedef const T& const_reference;

    public:

      // Storage Access
      data_type &data() { return m_data; }
      data_type const &data() const { return m_data; }

      // Element Access
      constexpr const_reference operator()(size_type i, size_type j) const { return m_data[j][i]; }
      reference operator()(size_type i, size_type j) { return m_data[j][i]; }

    private:

      std::array<std::array<T, M>, N> m_data;
  };

} // namespace lml
} // namespace leap

#endif
