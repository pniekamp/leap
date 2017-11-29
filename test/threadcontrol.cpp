//
// threadcontrol.cpp
//
// Copyright (C) Peter Niekamp
//
// This code remains the property of the copyright holder.
// The code contained herein is licensed for use without limitation
//

#include <iostream>
#include <chrono>
#include <leap/threadcontrol.h>
#include <leap/concurrentqueue.h>
#include <leap/util.h>

using namespace std;
using namespace leap;
using namespace leap::threadlib;

void ThreadTest();

Mutex mtx;
CriticalSection cs;
Event evt;
ConcurrentQueue<int> bq;
ThreadControl tc;

struct Timer
{
  auto elapsed() { return chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now() - sa).count(); }

  chrono::system_clock::time_point sa = chrono::system_clock::now();
};

static long testthread(void *)
{
  cout << "  Start Thread!" << endl;

  evt.wait();
  cout << "  Received Event!" << endl;

  mtx.wait();
  cout << "  Aquired Mutex!" << endl;

  cs.wait();
  cout << "  Aquired Critical Section!" << endl;

  int i = 0;
  while (true)
  {
    if (bq.pop(&i))
    {
      cout << "  Queue Entry " << i << endl;

      if (i == 5)
        break;
    }

    sleep_til(bq.activity());
  }

  cout << "  Waiting for Termination";
  while (!tc.terminating())
  {
    cout << ".";

    sleep_for(25);
  }

  cout << "\n";

  cs.release();
  mtx.release();

  return 0;
}

class TestClass
{
  public:
    TestClass()
    {
      m_threadcontrol.create_thread(this, &TestClass::MemberThread);
    }

    ~TestClass()
    {
      m_threadcontrol.join_threads();
    }

    long MemberThread()
    {
      cout << "  Member Thread" << endl;

      return 0;
    }

    ThreadControl m_threadcontrol;
};


//|//////////////////// ThreadCreateTest ////////////////////////////////////
static void ThreadCreateTest()
{
  cout << "Thread Create Test Set\n";

  mtx.wait();
  cs.wait();

  tc.create_thread(testthread, NULL);

  sleep_for(100);

  evt.set();

  sleep_for(100);

  mtx.release();

  sleep_for(100);

  cs.release();

  sleep_for(100);

  bq.push(1);
  bq.push(2);
  bq.push(3);
  bq.push(5);

  tc.join_threads();

  cout << "\n";

  cout << "Member Thread Test\n";

  {
    TestClass test;
  }

  cout << "\n";

  cout << "Queue Activity Test\n";

  for(int i = 0; i < 100 && bq.activity(); ++i)
    ;

  if (bq.activity())
    cout << "** Empty Queue Activity" << endl;

  bq.push(1);
  bq.push(2);

  if (!bq.activity())
    cout << "** Non-Empty Queue InActivity" << endl;

  bq.pop(nullptr);

  if (!bq.activity())
    cout << "** Non-Empty Queue InActivity" << endl;

  bq.pop(nullptr);

  if (bq.activity())
    cout << "** Empty Queue Activity" << endl;

  cout << "\n";

  cout << "ArgPack Test\n";

  string h = "Hello";

  ArgPack opa(1);
  ArgPack opb(2, 12.9);
  ArgPack opc(3, 'a', h);

  if (opa.size() != 0)
    cout << "** Zero Parameter Count Error" << endl;

  if (opb.size() != 1)
    cout << "** Single Parameter Count Error" << endl;

  if (opc.size() != 2)
    cout << "** Double Parameter Count Error" << endl;

  if (opb.value<double>(1) != 12.9)
    cout << "** Parameter Lookup Error" << endl;

  opa = std::move(opc);

  if (opa.value<char>(1) != 'a')
    cout << "** Copied Parameter Lookup Error" << endl;

  if (opa.value<string>(2) != "Hello")
    cout << "** Copied String Parameter Lookup Error" << endl;

  ConcurrentQueue<ArgPack> queue;

  queue.push(ArgPack(1, 99));

  queue.pop(&opa);

  if (opa.code() != 1)
    cout << "** Queue Pop Error" << endl;

  if (opa.value<int>(1) != 99)
    cout << "** Queue Pop Error" << endl;

  cout << "\n";
}


//|//////////////////// ThreadLatch /////////////////////////////////////////
static void ThreadLatch()
{
  cout << "Thread Latch Test Set\n";

  Latch latch(128);

  cout << "  Latch Created\n";

  Timer tm;

  latch.wait(0);

  cout << "  Zero Wait Elapsed: " << tm.elapsed() << "\n";

  latch.wait(100);

  cout << "  Timeout Wait Elapsed: " << tm.elapsed() << "\n";

  ThreadControl tc;

  for(int i = 0; i < 128; ++i)
    tc.create_thread([&,i]() { sleep_for(100); latch.release(); return 0; });

  latch.wait();

  cout << "  Latch Complete\n";

  tc.join_threads();

  cout << "\n";
}


//|//////////////////// ThreadSemaphore /////////////////////////////////////
static void ThreadSemaphore()
{
  cout << "Thread Semaphore Test Set\n";

  Semaphore sem(128);
  cout << "  Semaphore Created\n";

  sem.release(5);

  for(int i = 0; i < 5; ++i)
  {
    sem.wait();
    cout << "  " << i;
  }
  cout << "\n";

  if (sem.wait(50))
    cout << "**Extra Semaphore Error!\n";

  ThreadControl tc;

  for(int i = 0; i < 128; ++i)
    tc.create_thread([&,i]() { sem.wait(); return 0; });

  sem.release(128);

  tc.join_threads();

  Timer tm;

  sem.wait(0);

  cout << "  Zero Wait Elapsed: " << tm.elapsed() << "\n";

  sem.wait(100);

  cout << "  Timeout Wait Elapsed: " << tm.elapsed() << "\n";

  cout << "\n";
}


//|//////////////////// ThreadWaitGroup /////////////////////////////////////
static void ThreadWaitGroup()
{
  cout << "Thread WaitGroup Test Set\n";

  Event event1;
  Latch event2(1);
  Semaphore event3(1);

  if (event1)
    cout << "  ** Unset Event Failed\n";

  WaitGroup group;

  group.add(event1);
  group.add(event2);
  group.add(event3);

  if (group.wait_any(0))
    cout << "  ** Non-Signaled Group Any Failed\n";

  if (group.wait_all(0))
    cout << "  ** Non-Signaled Group All Failed\n";

  event1.set();

  if (!group.wait_any(0))
    cout << "  ** Signaled Group Any Failed\n";

  if (group.wait_all(0))
    cout << "  ** Non-Signaled Group All Failed\n";

  event3.release();

  if (!group.wait_any(0))
    cout << "  ** Signaled Group Any Failed\n";

  event1.set();
  event3.release();

  if (group.wait_all(0))
    cout << "  ** Non-Signaled Group All Failed\n";

  event2.release();

  if (!group.wait_all(0))
    cout << "  ** Signaled Group All Failed\n";

  event1.reset();

  Timer tm;

  group.wait_any(100);

  cout << "  Timeout Wait Elapsed: " << tm.elapsed() << "\n";

  group.remove(event1);
  group.remove(event2);
  group.remove(event3);

  cout << "\n";
}


//|//////////////////// ThreadTest //////////////////////////////////////////
void ThreadTest()
{
  ThreadCreateTest();

  ThreadLatch();

  ThreadSemaphore();

  ThreadWaitGroup();

  cout << endl;
}
