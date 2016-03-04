//
// siglib.cpp
//
// Copyright (C) Peter Niekamp
//
// This code remains the property of the copyright holder.
// The code contained herein is licensed for use without limitation
//

#include <iostream>
#include <leap/siglib.h>

using namespace std;
using namespace leap;

void SigLibTest();

class Source
{
  public:

    siglib::Signal<void (const char *)> sigTestMessage;

  public:

    void ProcessingFunction()
    {
      sigTestMessage("Test Message");
    }
};


static void OnTestMessage(const char *message)
{
  cout << "  Function Sink Received : " << message << "\n";
}


class Sink
{
  public:
    void OnTestMessage(const char *message)
    {
      cout << "  Object Sink Received   : " << message << "\n";
    }

    void operator()()
    {
      cout << "  Operator Sink Received\n";
    }

    void OnVoid()
    {
    }

    void OnOne(int)
    {
    }

    void OnTwo(int, int)
    {
    }

    void OnChainMessage(const char *message)
    {
      cout << "  Chained Sink Received  : " << message << "\n";
    }

    void OnAgregateMessage(int id, const char *message)
    {
      cout << "  Agregate Sink Received : " << message << " from " << id << "\n";
    }
};


//|//////////////////// SimpleSigLibTests ///////////////////////////////////
static void SimpleSigLibTests()
{
  cout << "SigLib Test Set\n";

  Source source;
  source.ProcessingFunction();

  Sink sink;

  source.sigTestMessage.attach(&OnTestMessage);
  source.sigTestMessage.attach(&sink, &Sink::OnTestMessage);

  Source source2 = source;

  source2.ProcessingFunction();

  source.sigTestMessage.detach();

  source.ProcessingFunction();

  siglib::Signal<void ()> sigNone;
  sigNone.attach(&sink);

  siglib::Signal<void (int)> sigOne;
  sigOne.attach(&sink, &Sink::OnOne);

  siglib::Signal<void (int, int)> sigTwo;
  sigTwo.attach(&sink, &Sink::OnTwo);

  siglib::Signal<void ()> sigVoid;
  sigVoid.attach(&sink, &Sink::OnVoid);

  siglib::Signal<void (const char *)> sigChain;
  sigChain.attach(&sink, &Sink::OnChainMessage);

  source.sigTestMessage.attach(&sigChain);

  source.sigTestMessage.attach([&](const char *message) { sink.OnAgregateMessage(99, message); });

  source.ProcessingFunction();

  cout << "\n";
}


//|//////////////////// SigLibTest //////////////////////////////////////////
void SigLibTest()
{
  SimpleSigLibTests();

  cout << endl;
}
