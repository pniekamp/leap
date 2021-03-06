//
// optional
//
// Copyright (C) Peter Niekamp
//
// This code remains the property of the copyright holder.
// The code contained herein may be used for any purpose provided
// this copyright notice is retained
//

#pragma once

#include <utility>

/**
 * \namespace leap
 * \brief Leap Leap Library containing common helper routines
 *
**/

/**
 * \defgroup leapdata Data Containors
 * \brief Data Containors
 *
**/

namespace leap
{
  //|-------------------- optional ------------------------------------------
  //|------------------------------------------------------------------------
  /**
   * \ingroup ingroup leapdata
   *
   * \brief Simple Optional Class
   *
  **/

  template<typename T>
  class optional : public std::pair<T, bool>
  {
    public:

      using value_type = T;

    public:
      optional() = default;
      optional(T const &value);

      template<typename... Args>
      void emplace(Args&&... args);

      constexpr bool has_value() const noexcept { return std::pair<T, bool>::second; }

      T &operator*() { return std::pair<T, bool>::first; }
      T const &operator*() const { return std::pair<T, bool>::first; }
      T *operator->() { return &(std::pair<T, bool>::first); }
      T const *operator->() const { return &(std::pair<T, bool>::first); }

      explicit operator bool() const { return has_value(); }
  };

  template<typename T>
  optional<T>::optional(T const &value)
    : std::pair<T, bool>(value, true)
  {
  }

  template<typename T>
  template<typename... Args>
  void optional<T>::emplace(Args&&... args)
  {
    *this = T(args...);
  }

} // namespace
