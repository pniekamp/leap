//
// array - multidemension array
//
// Copyright (C) Peter Niekamp
//
// This code remains the property of the copyright holder.
// The code contained herein may be used for any purpose provided
// this copyright notice is retained
//

#ifndef LMLARRAY_HH
#define LMLARRAY_HH

#include <vector>
#include <cassert>
#include <cstddef>
#include <stdexcept>
#include <array>
#include <numeric>
#include <memory>
#include <initializer_list>


/**
 * \namespace leap::lml
 * \brief Leap Math Library containing mathmatical routines
 *
**/

/**
 * \defgroup leapdata Data Containors
 * \brief Data Containors
 *
**/

namespace leap { namespace lml
{

  namespace ArrayImpl
  {

    //|-------------------- ArrayBase ---------------------------------------
    //|----------------------------------------------------------------------

    template<typename Iterator, size_t dimension>
    class ArrayBase
    {
      public:

        typedef typename std::iterator_traits<Iterator>::value_type value_type;
        typedef typename std::iterator_traits<Iterator>::reference reference;
        typedef typename std::iterator_traits<Iterator>::pointer pointer;
        typedef typename std::iterator_traits<Iterator>::value_type const &const_reference;
        typedef typename std::iterator_traits<Iterator>::value_type const *const_pointer;

      public:

        size_t dimensions() const { return dimension; }

        size_t size() const { return m_extents[0] * m_strides[0]; }

        const size_t *shape() const { return m_extents; }

        const size_t *strides() const { return m_strides; }

      protected:

        pointer data() { return m_data; }
        const_pointer data() const { return m_data; }

        void set_view(Iterator const &data, size_t const *extents, size_t const *strides);

      private:

        size_t const *m_extents;
        size_t const *m_strides;

        Iterator m_data;
    };


    //|///////////////////// ArrayBase::set_view ////////////////////////////
    template<typename Iterator, size_t dimension>
    void ArrayBase<Iterator, dimension>::set_view(Iterator const &data, size_t const *extents, size_t const *strides)
    {
      m_data = data;
      m_extents = extents;
      m_strides = strides;
    }


    //|///////////////////// array_size /////////////////////////
    template<typename... Args>
    size_t array_size(Args... extents)
    {
      size_t args[] = { extents... };

      return std::accumulate(args, args+sizeof...(extents), 1, std::multiplies<int>());
    }
  }


  //|-------------------- ArrayView ---------------------------------------
  //|----------------------------------------------------------------------

  template<typename Iterator, size_t dimension>
  class ArrayView : public ArrayImpl::ArrayBase<Iterator, dimension>
  {
    public:

      typedef typename std::iterator_traits<Iterator>::value_type value_type;
      typedef typename std::iterator_traits<Iterator>::reference reference;
      typedef typename std::iterator_traits<Iterator>::pointer pointer;
      typedef typename std::iterator_traits<Iterator>::value_type const &const_reference;
      typedef typename std::iterator_traits<Iterator>::value_type const *const_pointer;

    public:

      template<typename Iter>
      ArrayView(ArrayView<Iter, dimension> const &that)
      {
        this->set_view(that.data(), that.shape(), that.strides());
      }

      ArrayView<Iterator, dimension-1> operator [](size_t i)
      {
        return ArrayView<Iterator, dimension-1>(this->data()+i*this->strides()[0], this->shape()+1, this->strides()+1);
      }

      ArrayView<const_pointer, dimension-1> operator [](size_t i) const
      {
        return ArrayView<const_pointer, dimension-1>(this->data()+i*this->strides()[0], this->shape()+1, this->strides()+1);
      }

    protected:

      explicit ArrayView(Iterator const &data = NULL, size_t const *extents = NULL, size_t const *strides = NULL)
      {
        this->set_view(data, extents, strides);
      }

      template<typename, size_t> friend class ArrayView;
  };

  template<typename Iterator>
  class ArrayView<Iterator, 1> : public ArrayImpl::ArrayBase<Iterator, 1>
  {
    public:

      typedef typename std::iterator_traits<Iterator>::value_type value_type;
      typedef typename std::iterator_traits<Iterator>::reference reference;
      typedef typename std::iterator_traits<Iterator>::pointer pointer;
      typedef typename std::iterator_traits<Iterator>::value_type const &const_reference;
      typedef typename std::iterator_traits<Iterator>::value_type const *const_pointer;

    public:
      template<typename Iter>
      ArrayView(ArrayView<Iter, 1> const &that)
      {
        this->set_view(that.data(), that.shape(), that.strides());
      }

      reference operator [](size_t i)
      {
        assert(i < this->shape()[0]);

        return this->data()[i];
      }

      const_reference operator [](size_t i) const
      {
        assert(i < this->shape()[0]);

        return this->data()[i];
      }

      pointer begin() { return this->data(); }
      pointer end() { return this->data()+this->size(); }

      const_pointer begin() const { return this->data(); }
      const_pointer end() const { return this->data()+this->size(); }

    protected:

      explicit ArrayView(Iterator const &data = NULL, size_t const *extents = NULL, size_t const *strides = NULL)
      {
        this->set_view(data, extents, strides);
      }

      template<typename, size_t> friend class ArrayView;
  };



  //|-------------------- Array ---------------------------------------------
  //|------------------------------------------------------------------------
  /**
   * \ingroup ingroup leapdata
   *
   * \brief Multidemension Array Class
   *
  **/

  template<typename T, size_t dimension>
  class Array : public ArrayView<T*, dimension>
  {
    public:

      typedef T value_type;
      typedef T &reference;
      typedef T *pointer;
      typedef T const &const_reference;
      typedef T const *const_pointer;

    public:

      Array() = default;

      template<typename... Args>
      explicit Array(Args... extents);

      // Copyable & Moveable
      Array(Array const &other);
      Array(Array &&other);
      Array &operator=(Array other);

      template<typename... Args>
      void reshape(Args... extents);

    public:

      std::vector<T> const &data() const { return m_data; }

      typename std::vector<T>::iterator begin() { return m_data.begin(); }
      typename std::vector<T>::iterator end() { return m_data.end(); }

      typename std::vector<T>::const_iterator begin() const { return m_data.begin(); }
      typename std::vector<T>::const_iterator end() const { return m_data.end(); }

    private:

      std::vector<size_t> m_extents;
      std::vector<size_t> m_strides;

      std::vector<T> m_data;
  };


  //|///////////////////// Array::Constructor //////////////////////////////
  template<typename T, size_t dimension>
  template<typename... Args>
  Array<T, dimension>::Array(Args... extents)
    : m_extents(dimension),
      m_strides(dimension),
      m_data(ArrayImpl::array_size(extents...))
  {
    this->set_view(m_data.data(), m_extents.data(), m_strides.data());

    reshape(extents...);
  }


  //|///////////////////// Array::Constructor //////////////////////////////
  template<typename T, size_t dimension>
  Array<T, dimension>::Array(Array const &other)
    : ArrayView<T*, dimension>(),
      m_extents(other.m_extents),
      m_strides(other.m_strides),
      m_data(other.m_data)
  {
    this->set_view(m_data.data(), m_extents.data(), m_strides.data());
  }


  //|///////////////////// Array::Constructor //////////////////////////////
  template<typename T, size_t dimension>
  Array<T, dimension>::Array(Array &&other)
    : ArrayView<T*, dimension>(),
      m_extents(std::move(other.m_extents)),
      m_strides(std::move(other.m_strides)),
      m_data(std::move(other.m_data))
  {
    this->set_view(m_data.data(), m_extents.data(), m_strides.data());
  }


  //|///////////////////// Array::Constructor //////////////////////////////
  template<typename T, size_t dimension>
  Array<T, dimension> &Array<T, dimension>::operator =(Array other)
  {
    std::swap(m_extents, other.m_extents);
    std::swap(m_strides, other.m_strides);
    std::swap(m_data, other.m_data);

    this->set_view(m_data.data(), m_extents.data(), m_strides.data());

    return *this;
  }


  //|///////////////////// Array::reshape ///////////////////////////////////
  template<typename T, size_t dimension>
  template<typename... Args>
  void Array<T, dimension>::reshape(Args... extents)
  {
    size_t args[dimension] = { extents... };

    for(size_t i = 0; i < dimension; ++i)
      m_extents[i] = (i < sizeof...(extents)) ? args[i] : 1;

    for(size_t i = 0; i < dimension; ++i)
      m_strides[i] = std::accumulate(m_extents.data()+i+1, m_extents.data()+dimension, 1, std::multiplies<int>());
  }



  /**
   *  @}
  **/

  /**
   * \name Misc Array
   * \ingroup leapdata
   * Array helpers
   * @{
  **/


  /**
   *  @}
  **/


} // namespace lml
} // namespace leap

#endif
