//
// http.cpp
//
// Copyright (C) Peter Niekamp
//
// This code remains the property of the copyright holder.
// The code contained herein is licensed for use without limitation
//

#include <iostream>
#include <chrono>
#include <leap/http.h>

using namespace std;
using namespace leap;
using namespace leap::socklib;

void HTTPTest();

struct Timer
{
  unsigned int elapsed() { return chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now() - sa).count(); }

  chrono::system_clock::time_point sa = chrono::system_clock::now();
};


//|//////////////////// TestRequest /////////////////////////////////////////
void TestRequest()
{
  HTTPRequest request1("GET", "www.example.com/path/index.html");
  cout << "  " << request1.method() << " http://" << request1.server() << ":" << request1.port() << request1.location() << "\n";

  if (request1.server() != "www.example.com")
    cout << "** Wrong Domain\n";

  if (request1.port() != 80)
    cout << "** Wrong Port\n";

  if (request1.location() != "/path/index.html")
    cout << "** Wrong Location";

  HTTPRequest request2("GET", "http://www.example.com:81/path/index.html");
  cout << "  " << request2.method() << " http://" << request2.server() << ":" << request2.port() << request2.location() << "\n";

  if (request2.server() != "www.example.com")
    cout << "** Wrong Domain\n";

  if (request2.port() != 81)
    cout << "** Wrong Port\n";

  if (request2.location() != "/path/index.html")
    cout << "** Wrong Location";

  cout << endl;
}


//|//////////////////// TestWebSocket ///////////////////////////////////////
void TestWebSocket()
{
  WebSocket ws("ws://echo.websocket.org/");

  ws.onconnect([&]() {
    cout << "  WebSocket Connect\n";

    WebSocketMessage message("Test");

    ws.send(message);
  });

  ws.onmessage([&](WebSocketMessage const &msg) {
    cout << "  WebSocket Receive: " + string(msg.payload().data(), msg.payload().size()) + "\n";

    ws.close();
  });

  ws.ondisconnect([&]() {
    cout << "  WebSocket Disconnect\n";
  });

  while (ws.state() != WebSocket::SocketState::Cactus)
  {
    threadlib::sleep_for(100);
  }

  cout << endl;
}


//|//////////////////// TestClient //////////////////////////////////////////
void TestClient()
{
  threadlib::ThreadControl tc;

  HTTPServer server;

  server.sigRespond.attach([&](HTTPServer::socket_t socket, HTTPRequest const &request) {
    cout << "  ServerReceive: " << request.method() << " " << request.location() << endl;

    server.send(socket, HTTPResponse("<HTML>OK</HTML>"));
  });

  server.start(1202);

  HTTPResponse response;

  HTTPClient::execute(HTTPRequest("GET", "http://127.0.0.1:1202/objects"), &response);
//  HTTPClient::execute(HTTPRequest("GET", "http://www.websocket.org/echo.html"), &response);

  if (response.status() == 200)
  {
    cout << "  ClientReceive: " << string(response.payload().begin(), response.payload().end()) << "\n";
  }
  else
    cout << "  ** No Data\n";

  tc.join_threads();

  cout << endl;
}


//|//////////////////// HTTPTest ////////////////////////////////////////////
void HTTPTest()
{
  cout << "HTTP Test\n\n";

  TestRequest();

  TestWebSocket();

  TestClient();

  cout << endl;
}
