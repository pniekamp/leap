//
// sockets.cpp
//
// Copyright (C) Peter Niekamp
//
// This code remains the property of the copyright holder.
// The code contained herein is licensed for use without limitation
//

#include <iostream>
#include <chrono>
#include <leap/sockets.h>
#include <leap/util.h>

#ifndef _WIN32
# include <arpa/inet.h>
#endif

using namespace std;
using namespace leap;
using namespace leap::socklib;

void SocketTest();

struct Timer
{
  auto elapsed() { return chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now() - sa).count(); }

  chrono::system_clock::time_point sa = chrono::system_clock::now();
};


//|//////////////////// SocketBasicTest /////////////////////////////////////
static void SocketBasicTest()
{
}


//|//////////////////// SimpleSocketTest ////////////////////////////////////
static void SimpleSocketTest()
{
  cout << "Simple Socket Test Set\n";

  Timer tm;

  ServerSocket server(1200);
  ClientSocket client("localhost", 1200);

  string lastserver;
  string lastclient;

  do
  {
    string serverstatus = server.statustxt();
    string clientstatus = client.statustxt();

    if (serverstatus != lastserver || clientstatus != lastclient)
      cout << "  " << tm.elapsed() << "\t" << server.statustxt() << "\t" << client.statustxt() << "\n";

    lastserver = serverstatus;
    lastclient = clientstatus;

    if (client.connect() && server.connected())
    {
      cout << "  " << tm.elapsed() << "\t" << "Connected\tConnected\n";
      break;
    }

  } while (server.status() != SocketBase::SocketStatus::Dead && client.status() != SocketBase::SocketStatus::Dead);

  for(int i = 0; i < 100; ++i)
    server.transmit("HelloWorld", 11);

  cout << "  " << tm.elapsed() << "\t" << "Transmit" << "\t";

  client.wait_on_bytes(11);

  cout << "Receive" << "\n";

  int bytes = 0;
  while (client.wait_on_bytes(1, 100))
  {
    uint8_t buffer[555];
    bytes += client.receive(buffer, sizeof(buffer));
  }

  if (bytes != 1100)
    cout << "** Insufficient Receive\n";

  client.destroy();

  while (server.connected())
    ;

  cout << "  " << tm.elapsed() << "\t" << server.statustxt() << "\t\t" << client.statustxt() << "\n";

  server.destroy();

  cout << "  " << tm.elapsed() << "\t" << server.statustxt() << "\t\t" << client.statustxt() << "\n";

  cout << endl;
}


//|//////////////////// BroadcastSocketTest /////////////////////////////////
static void BroadcastSocketTest()
{
  cout << "Broadcast Socket Test Set\n";

  Timer tm;

  BroadcastSocket server(1200);
  BroadcastSocket client(1208);

  string lastserver;
  string lastclient;

  do
  {
    string serverstatus = server.statustxt();
    string clientstatus = client.statustxt();

    if (serverstatus != lastserver || clientstatus != lastclient)
      cout << "  " << tm.elapsed() << "\t" << server.statustxt() << "\t" << client.statustxt() << "\n";

    lastserver = serverstatus;
    lastclient = clientstatus;

    if (client.connected() && server.connected())
    {
      cout << "  " << tm.elapsed() << "\t" << "Connected\tConnected\n";
      break;
    }

  } while (server.status() != SocketBase::SocketStatus::Dead && client.status() != SocketBase::SocketStatus::Dead);

  for(int i = 0; i < 100; ++i)
    server.broadcast("HelloWorld", 11, inet_addr("127.0.0.1"), 1208);

  cout << "  " << tm.elapsed() << "\t" << "Transmit" << "\t";

  client.wait_on_packet();

  cout << "Receive" << "\n";

  int bytes = 0;
  while (client.wait_on_packet(100))
  {
    uint8_t buffer[555];
    bytes += client.receive(buffer, sizeof(buffer));
  }

  if (bytes != 1100)
    cout << "** Insufficient Receive\n";

  client.destroy();
  server.destroy();

  cout << "  " << tm.elapsed() << "\t" << server.statustxt() << "\t\t" << client.statustxt() << "\n";

  cout << endl;
}


//|//////////////////// SocketPumpTest //////////////////////////////////////
static void SocketPumpTest()
{
  socket_t socket;
  sockaddr_in addr;

  cout << "Socket Pump Test Set\n";

  SocketPump pump(1201);

  if (pump.accept_connection(&socket))
    cout << "  ** Invalid Connection\n";

  {
    ClientSocket client("localhost", 1201);

    client.wait_on_connect(1000);

    pump.wait_for_connection(100);

    if (!pump.accept_connection(&socket, &addr))
      cout << "  ** Missing Connection\n";

    cout << "  Connected " << inet_ntoa(addr.sin_addr) << endl;
  }

  cout << endl;
}


//|//////////////////// SocketTest //////////////////////////////////////////
void SocketTest()
{
  InitialiseSocketSubsystem();

  SocketBasicTest();
  SimpleSocketTest();
  BroadcastSocketTest();
  SocketPumpTest();

  cout << "Interfaces Set\n";

  auto ifc = interfaces();
  for(auto i = ifc.begin(); i != ifc.end(); ++i)
    cout << "  " << i->name << " " << hex << i->ip << " " << i->mask << " " << i->bcast << dec << endl;

  cout << endl;
}
