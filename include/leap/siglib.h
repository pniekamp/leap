//
// Signal Source/Sink Templates
//
//   Peter Niekamp, September, 2001
//

//
// Copyright (C) Peter Niekamp
//
// This code remains the property of the copyright holder.
// The code contained herein may be used for any purpose provided
// this copyright notice is retained
//

#ifndef SIGLIB_HH
#define SIGLIB_HH

#include <vector>
#include <functional>
#include <memory>

/**
 * \namespace leap::siglib
 * \brief Signal Source and Sink Operations
 *
**/

/**
 * \defgroup leapthread Thread and Signal Handling
 * \brief Thread and Signal Handling
 *
**/

namespace leap { namespace siglib
{
  template<typename R, typename... Args> class Signal; // undefined


  //|///////////////////////// Signal ///////////////////////////////////////
  /**
   * \ingroup leapthread
   *
   * \brief A Signal Source
   *
   * Signal Handling provides the ability for an object to provide one or more
   * sources of events. The object can fire events and have them received by
   * one or many listener functions (including member functions).
   *
   * Signals must have exactly one parameter (the type of which is specified
   * in the template argument)
   *
   * Typical use :
   *
   *  Source :
   *  \code
   *   class Source
   *   {
   *     public:
   *
   *       siglib::Signal<void (const char *)> sigConnectionLost;
   *
   *     public:
   *
   *       void ProcessingFunction()
   *       {
   *         if (!m_HaveConnection)
   *           sigConnectionLost("Lost Due to Processing Failure");
   *       }
   *    };
   *
   *  \endcode
   *
   *  Function Sink :
   *  \code
   *  void OnConnectionLost(const char *reason);
   *
   *  source.sigConnectionLost.attach(&OnConnectionLost);
   *
   *  \endcode
   *
   *  Object Sink :
   *  \code
   *  class Sink
   *  {
   *    void OnConnectionLost(const char *reason);
   *  };
   *
   *  Sink sink;
   *  source.sigConnectionLost.attach(&sink, &Sink::OnConnectionLost);
   *
   *  \endcode
  **/

  template<typename R, typename... Args> class Signal<R(Args...)>
  {
    public:
      Signal() = default;

      void operator()(Args... args);
      void operator()(Args... args) const;

      void attach(R (*func)(Args...));

      template<class Object>
      void attach(Object object);

      template<class Object>
      void attach(Object *object);

      template<class Object>
      void attach(Object *object, R (Object::*func)(Args...));

      void detach();

    private:

      std::vector<std::function<R(Args...)>> m_sinks;
  };


  //|///////////////////// Signal::emit /////////////////////////////////////
  template<typename R, typename... Args>
  void Signal<R(Args...)>::operator()(Args... args)
  {
    for(auto &sink : m_sinks)
      sink(args...);
  }


  //|///////////////////// Signal::emit /////////////////////////////////////
  template<typename R, typename... Args>
  void Signal<R(Args...)>::operator()(Args... args) const
  {
    for(auto &sink : m_sinks)
      sink(args...);
  }


  //|///////////////////// Signal::attach ///////////////////////////////////
  template<typename R, typename... Args>
  void Signal<R(Args...)>::attach(R (*func)(Args...))
  {
    m_sinks.push_back(func);
  }


  //|///////////////////// Signal::attach ///////////////////////////////////
  template<typename R, typename... Args>
  template<class Object>
  void Signal<R(Args...)>::attach(Object object)
  {
    m_sinks.push_back(std::move(object));
  }


  //|///////////////////// Signal::attach ///////////////////////////////////
  template<typename R, typename... Args>
  template<class Object>
  void Signal<R(Args...)>::attach(Object *object)
  {
    m_sinks.push_back([=](Args&&... args) { object->operator()(std::forward<Args>(args)...); });
  }


  //|///////////////////// Signal::attach ///////////////////////////////////
  template<typename R, typename... Args>
  template<class Object>
  void Signal<R(Args...)>::attach(Object *object, R (Object::*func)(Args...))
  {
    m_sinks.push_back([=](Args&&... args) { (object->*func)(std::forward<Args>(args)...); });
  }


  //|///////////////////// Signal::detach ///////////////////////////////////
  template<typename R, typename... Args>
  void Signal<R(Args...)>::detach()
  {
    m_sinks.clear();
  }


} } // namespace siglib

#endif // SIGLIB_HH
