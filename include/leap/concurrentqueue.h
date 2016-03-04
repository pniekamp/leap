//
// Concurrent Queue
//
//   Peter Niekamp, September, 2000
//

//
// Copyright (C) Peter Niekamp
//
// This code remains the property of the copyright holder.
// The code contained herein may be used for any purpose provided
// this copyright notice is retained
//

#ifndef CONCURRENTQUEUE_HH
#define CONCURRENTQUEUE_HH

#include <leap/threadcontrol.h>
#include <memory>
#include <algorithm>
#include <functional>
#include <type_traits>
#include <typeinfo>
#include <deque>
#include <atomic>

/**
 * \namespace leap::threadlib
 * \brief Theading Primitives and Operations
 *
**/

/**
 * \defgroup leapthread Thread and Signal Handling
 * \brief Thread and Signal Handling
 *
**/

namespace leap { namespace threadlib
{

  //|------------------------- ConcurrentQueue ------------------------------
  //|------------------------------------------------------------------------
  /**
   * \ingroup leapthread
   *
   * \brief Concurrent Queue
   *
   *  Thread safe Queue Object. The prupose of this class is to provide
   *  efficient Get and Put semantics for inter thread Queues.
   *
  **/

  template<typename T>
  class ConcurrentQueue
  {
    public:
      ConcurrentQueue();
      ConcurrentQueue(ConcurrentQueue const &) = delete;
      ConcurrentQueue(ConcurrentQueue &&) = delete;

      template<typename Q>
      bool push(Q &&object);

      bool pop(T *object);

      void flush();

      template<class Predicate>
      void remove_if(Predicate predicate);

      template<class Priority>
      void sort(Priority priority);

    public:

      threadlib::Waitable &activity() { return m_activity; }

    private:

      std::deque<T> m_queue;

      threadlib::Semaphore m_activity;

      mutable threadlib::CriticalSection m_mutex;
  };


  //|///////////////////// ConcurrentQueue::Constructor /////////////////////
  template<typename T>
  ConcurrentQueue<T>::ConcurrentQueue()
    : m_activity(std::numeric_limits<int>::max())
  {
  }


  //|///////////////////// ConcurrentQueue::flush ///////////////////////////
  ///
  /// Flush the Queue, discarding contents
  ///
  template<typename T>
  void ConcurrentQueue<T>::flush()
  {
    SyncLock M(m_mutex);

    m_queue.clear();
  }


  //|///////////////////// ConcurrentQueue::pop /////////////////////////////
  ///
  /// Removes the front item from the Queue
  ///
  /// \param[out] object Returns a pointer to the object
  ///
  template<typename T>
  bool ConcurrentQueue<T>::pop(T *object)
  {
    SyncLock M(m_mutex);

    if (m_queue.empty())
      return false;

    // Pop the Object

    if (object)
      *object = std::move(m_queue.front());

    m_queue.erase(m_queue.begin());

    return true;
  }


  //|///////////////////// ConcurrentQueue::push ////////////////////////////
  ///
  /// Places an object into the Queue.
  ///
  /// \param[in] object  - object to put on the Queue
  ///
  template<typename T>
  template<typename Q>
  bool ConcurrentQueue<T>::push(Q &&object)
  {
    {
      SyncLock M(m_mutex);

      // Push the object

      m_queue.push_back(std::forward<Q>(object));
    }

    m_activity.release();

    return true;
  }


  //|///////////////////// ConcurrentQueue::remove_if ///////////////////////
  ///
  /// Removes items from the Queue
  ///
  /// \param[in] predicate - functor
  ///
  template<typename T>
  template<class Predicate>
  void ConcurrentQueue<T>::remove_if(Predicate predicate)
  {
    SyncLock M(m_mutex);

    m_queue.erase(std::remove_if(m_queue.begin(), m_queue.end(), [&](T &i) { return predicate(i); }), m_queue.end());
  }


  //|///////////////////// ConcurrentQueue::sort ////////////////////////////
  ///
  /// Re-Orders Queue contents based on priority function
  ///
  /// \param[in] priority - functor
  ///
  template<typename T>
  template<class Priority>
  void ConcurrentQueue<T>::sort(Priority priority)
  {
    SyncLock M(m_mutex);

    std::stable_sort(m_queue.begin(), m_queue.end(), [&](T const &lhs, T const &rhs) { return priority(rhs) < priority(lhs); });
  }



  //|------------------------- ArgPack --------------------------------------
  //|------------------------------------------------------------------------
  /**
   * \ingroup leapthread
   *
   * \brief Argument Pack for Queue Submissions
   *
   *  An OpCode & Parameter Set useful for submitting operations to a Queue
   *
  **/

  class ArgPack
  {
    public:
      ArgPack(int opcode = -1);

      template<typename... Args>
      explicit ArgPack(int opcode, Args&&...);

      int code() const { return m_opcode; }

      size_t size() const { return m_parameters.size(); }

      template<typename T>
      T const &value(size_t i) const;

    private:

      template<typename Q>
      void add(Q &&p0);

      template<typename Q, typename... Args>
      void add(Q &&p0, Args&&...params);

    private:

      class holderbase
      {
        public:

          virtual ~holderbase()
          {
          }

          virtual const std::type_info &type() const = 0;

          virtual holderbase *clone() const = 0;
      };

      template<typename T>
      class holder : public holderbase
      {
        public:

          template<typename Q>
          holder(Q &&value)
            : held(std::forward<Q>(value))
          {
          }

          virtual const std::type_info &type() const
          {
            return typeid(T);
          }

          virtual holder *clone() const
          {
            return new holder(held);
          }

          T held;
      };

    private:

      int m_opcode;

      std::vector<std::unique_ptr<holderbase>> m_parameters;
  };



  //|///////////////////// ArgPack::Constructor /////////////////////////////
  inline ArgPack::ArgPack(int opcode)
    : m_opcode(opcode)
  {
  }


  //|///////////////////// ArgPack::Constructor /////////////////////////////
  template<typename... Args>
  ArgPack::ArgPack(int opcode, Args&&...params)
    : m_opcode(opcode)
  {
    m_parameters.reserve(sizeof...(params));

    add(std::forward<Args>(params)...);
  }


  //|///////////////////// ArgPack::add /////////////////////////////////////
  template<typename Q>
  void ArgPack::add(Q &&p0)
  {
    m_parameters.push_back(std::unique_ptr<holderbase>(new holder<std::decay_t<Q>>(std::forward<Q>(p0))));
  }


  //|///////////////////// ArgPack::add /////////////////////////////////////
  template<typename Q, typename... Args>
  void ArgPack::add(Q &&p0, Args&&...params)
  {
    add(std::forward<Q>(p0));
    add(std::forward<Args>(params)...);
  }


  //|///////////////////// ArgPack::value ///////////////////////////////////
  template<typename T>
  T const &ArgPack::value(size_t i) const
  {
    if (m_parameters[i-1]->type() != typeid(T))
      throw std::bad_cast();

    return static_cast<holder<T>*>(m_parameters[i-1].get())->held;
  }



  //|------------------------- ArgPackOp --------------------------------------
  //|--------------------------------------------------------------------------
  /**
   * \ingroup leapthread
   *
   * \brief Argument Pack Op Predicate
   *
   *  Predicate to test an ArgPack for specific opcode
   *
  **/

  class ArgPackOp : public std::unary_function<ArgPack, bool>
  {
    public:
      ArgPackOp(int op)
      {
        m_op = op;
      }

      bool operator()(ArgPack const &entry) const
      {
        return (entry.code() == m_op);
      }

      int m_op;
  };

} } // namespace


#endif //ConcurrentQueue_HH
