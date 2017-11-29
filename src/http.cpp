//
// HTTP protocol
//
//   Peter Niekamp, February 2008
//

//
// Copyright (C) Peter Niekamp
//
// This code remains the property of the copyright holder.
// The code contained herein may be used for any purpose provided
// this copyright notice is retained
//

#include "leap/http.h"
#include "leap/regex.h"
#include "leap/util.h"
#include "leap/hash.h"
#include <algorithm>
#include <fstream>
#include <cassert>
#include <random>
#include <mutex>

using namespace std;
using namespace leap;
using namespace leap::crypto;
using namespace leap::socklib;
using namespace leap::threadlib;

namespace
{

  //////////////////// rnd8 ////////////////////////////////
  uint8_t rnd8()
  {
    static thread_local mt19937 generator(random_device{}());

    return uniform_int_distribution<int>{0, numeric_limits<uint8_t>::max()}(generator);
  }


  //////////////////// read_http_state /////////////////////
  struct read_http_state
  {
    int step = 0;

    function<size_t (StreamSocket &socket, size_t bytes, HTTPBase *msg)> callback =
      [](StreamSocket &socket, size_t bytes, HTTPBase *msg) {
        char *buffer = msg->reserve_payload(bytes);

        socket.receive(buffer, bytes);

        return bytes;
      };

    bool chunked;
    size_t chunksize;
    size_t remaining;
  };


  //|///////////////////// send_http_request ////////////////////////////////
  bool send_http_request(StreamSocket &socket, HTTPRequest const &msg)
  {
    string head = msg.head();

    socket.transmit(head.c_str(), head.size());

    if (msg.payload().size() != 0)
      socket.transmit(msg.payload().data(), msg.payload().size());

    return true;
  }


  //|///////////////////// send_http_response ///////////////////////////////
  bool send_http_response(StreamSocket &socket, HTTPResponse const &msg)
  {
    string head = msg.head();

    socket.transmit(head.c_str(), head.size());

    if (msg.payload().size() != 0)
      socket.transmit(msg.payload().data(), msg.payload().size());

    return true;
  }


  //|///////////////////// read_http_base ///////////////////////////////////
  bool read_http_base(read_http_state &state, StreamSocket &socket, HTTPBase *msg)
  {
    char line[4096];

    loop:

    switch (state.step)
    {
      case 2:

        // Read Header

        while (true)
        {
          if (!readline(socket, line, sizeof(line)))
            return false;

          // blank line is end-of-header
          if (line[0] == 0)
            break;

          msg->add_header(line);
        }

        state.step = 3;

        [[fallthrough]];

      case 3:

        // Start Read Payload

        state.chunked = (msg->header("Transfer-Encoding") == "chunked");

        if (!state.chunked)
        {
          state.chunksize = 0;
          state.remaining = atoi(msg->header("Content-Length").c_str());
        }

        state.step = 4;

        [[fallthrough]];
        
      case 4:

        // Start Read Chunk

        if (state.chunked)
        {
          if (!readline(socket, line, sizeof(line)))
            return false;

          state.chunksize = strtol(line, NULL, 16);
          state.remaining = state.chunksize;
        }

        state.step = 5;

        [[fallthrough]];

      case 5:

        // Continue Read Chunk

        while (state.remaining != 0)
        {
          size_t bytes = min(state.remaining, StreamSocket::kSocketBufferSize);

          if (socket.bytes_available() < bytes)
            return false;

          bytes = state.callback(socket, bytes, msg);

          state.remaining -= bytes;
        }

        state.step = 6;

        [[fallthrough]];

      case 6:

        // Finish Read Chunk

        if (state.chunked)
        {
          if (!readline(socket, line, sizeof(line)))
            return false;

          if (state.chunksize != 0)
          {
            state.step = 4;
            goto loop;
          }
        }

        state.step = 7;
    }

    return true;
  }


  //|///////////////////// read_http_request ////////////////////////////////
  bool read_http_request(StreamSocket &socket, HTTPRequest *msg, int timeout = 20000)
  {
    msg->clear();

    char line[4096];

    read_http_state state;

    if (!readline(socket, line, sizeof(line)))
      return false;

    vector<string_view> fields = split(line);

    if (fields.size() != 3 || (fields[2] != "HTTP/1.0" && fields[2] != "HTTP/1.1"))
      throw SocketBase::socket_error("Invalid HTTP Header");

    msg->set_status(200);
    msg->set_method(toupper(fields[0].to_string()));
    msg->set_location(fields[1].to_string());

    state.step = 2;

    while (true)
    {
      if (read_http_base(state, socket, msg))
        return true;

      if (!socket.wait_on_activity(timeout))
        throw SocketBase::socket_error("Timeout Receiving Payload");
    }
  }


  //|///////////////////// read_http_response ///////////////////////////////
  bool read_http_response(StreamSocket &socket, HTTPResponse *msg, int timeout = 20000)
  {
    msg->clear();

    char line[4096];

    read_http_state state;

    if (!readline(socket, line, sizeof(line)))
      return false;

    if (line[0] != 'H' || line[1] != 'T' || line[2] != 'T' || line[3] != 'P')
      throw SocketBase::socket_error("Invalid HTTP Header");

    msg->set_status(ato<int>(&line[9]));

    state.step = 2;

    while (true)
    {
      if (read_http_base(state, socket, msg))
        return true;

      if (!socket.wait_on_activity(timeout))
        throw SocketBase::socket_error("Timeout Receiving Payload");
    }
  }


  //|///////////////////// read_http_response ///////////////////////////////
  bool read_http_response(read_http_state &state, StreamSocket &socket, HTTPResponse *msg)
  {
    char line[4096];

    switch (state.step)
    {
      case 0:
        msg->clear();

        state.step = 1;

        [[fallthrough]];

      case 1:

        if (!readline(socket, line, sizeof(line)))
          return false;

        if (line[0] != 'H' || line[1] != 'T' || line[2] != 'T' || line[3] != 'P')
          throw SocketBase::socket_error("Invalid HTTP Header");

        msg->set_status(ato<int>(&line[9]));

        state.step = 2;
    }

    return read_http_base(state, socket, msg);
  }


  //|///////////////////// send_websocket_frame /////////////////////////////
  bool send_websocket_frame(StreamSocket &socket, const void *buffer, size_t bytes, int opcode, bool fin, bool masked, uint32_t maskkey = 0)
  {
    size_t len = 0;
    uint8_t frame[16];

    frame[0] = (fin << 7) | (opcode << 0);
    frame[1] = (masked << 7);

    if (bytes < 126)
    {
      frame[1] |= bytes;

      len = 2;
    }
    else if (bytes < 65536)
    {
      frame[1] |= 126;
      frame[2] = (bytes & 0xFF00) >> 8;
      frame[3] = (bytes & 0x00FF);

      len = 4;
    }
    else
    {
      throw SocketBase::socket_error("Large Frames Not Supported");
    }

    if (masked)
    {
      frame[len++] = (maskkey & 0xFF000000) >> 24;
      frame[len++] = (maskkey & 0x00FF0000) >> 16;
      frame[len++] = (maskkey & 0x0000FF00) >> 8;
      frame[len++] = (maskkey & 0x000000FF) >> 0;
    }

    socket.transmit(frame, len);
    socket.transmit(buffer, bytes);

    return true;
  }


  //|///////////////////// send_websocket_message ///////////////////////////
  bool send_websocket_message(StreamSocket &socket, const void *buffer, size_t bytes, int opcode, bool masked, uint32_t maskkey = 0)
  {
    return send_websocket_frame(socket, buffer, bytes, opcode, true, masked, maskkey);
  }


  //|///////////////////// read_websocket_frame /////////////////////////////
  tuple<int, int, size_t> read_websocket_frame(StreamSocket &socket, WebSocketMessage *msg, int timeout)
  {
    uint8_t head[2];

    socket.wait_on_bytes(2);
    socket.receive(&head, 2);

    int fin = (head[0] & 0x80) >> 7;
    int masked = (head[1] & 0x80) >> 7;
    int opcode = (head[0] & 0x0F);

    size_t length = (head[1] & 0x7F);

    if (length == 126)
    {
      uint16_t len;

      socket.wait_on_bytes(2);
      socket.receive(&len, 2);

      length = ntohs(len);
    }
    else if (length == 127)
    {
      throw SocketBase::socket_error("Large Frames Not Supported");
    }

    uint8_t maskkey[4];

    if (masked)
    {
      socket.wait_on_bytes(4);
      socket.receive(&maskkey, 4);
    }

    for(size_t remaining = length; remaining > 0; )
    {
      size_t bytes = min(remaining, (size_t)4096);

      if (!socket.wait_on_bytes(bytes, timeout))
        throw SocketBase::socket_error("Timeout Receiving Payload");

      char *buffer = msg->reserve_payload(bytes);

      socket.receive(buffer, bytes);

      if (masked && (maskkey[0] != 0 || maskkey[1] != 0 || maskkey[2] != 0 || maskkey[3] != 0))
      {
        for(size_t i = 0; i < length; ++i)
          ((uint8_t*)buffer)[i] ^= maskkey[length - remaining + i % 4];
      }

      remaining -= bytes;
    }

    return make_tuple(opcode, fin, length);
  }


  //|///////////////////// read_websocket_message ///////////////////////////
  bool read_websocket_message(StreamSocket &socket, WebSocketMessage *msg, int timeout = 20000)
  {
    msg->clear();

    size_t s = 0;

    while (true)
    {
      int fin;
      int opcode;
      size_t bytes;

      if (s == 0 && socket.bytes_available() < 2)
        return false;

      if (s != 0 && !socket.wait_on_bytes(2, timeout))
        throw SocketBase::socket_error("Timeout Waiting for Next Frame");

      tie(opcode, fin, bytes) = read_websocket_frame(socket, msg, timeout);

      switch (opcode)
      {
        case 0:
        case 1:
        case 2:
          s += bytes;

          if (opcode == 1)
            msg->set_type(WebSocketMessage::MessageType::Text);

          if (opcode == 2)
            msg->set_type(WebSocketMessage::MessageType::Binary);

          if (fin)
            return true;

          break;

        case 9:  // Ping
          send_websocket_frame(socket, msg->payload().data()+s, bytes, 10, true, false);
          msg->release_payload(bytes);
          break;

        case 10: // Pong
          msg->release_payload(bytes);
          break;

        default:
          throw SocketBase::socket_error("Opcode Not Supported");
      }
    }
  }

} // namespace


namespace leap { namespace socklib
{

  //|--------------------- HTTPBase -----------------------------------------
  //|------------------------------------------------------------------------


  //|///////////////////// HTTPBase::Constructor ////////////////////////////
  HTTPBase::HTTPBase()
  {
    m_status = 408;
  }


  //|///////////////////// HTTPBase::header /////////////////////////////////
  string const &HTTPBase::header(string_view name) const
  {
    static string nullstr;

    auto value = m_header.find(name);

    if (value != m_header.end())
      return value->second;

    return nullstr;
  }


  //|///////////////////// HTTPBase::clear //////////////////////////////////
  void HTTPBase::clear()
  {
    m_status = 408;

    m_header.clear();

    m_payload.clear();
  }


  //|///////////////////// HTTPBase::set_status /////////////////////////////
  void HTTPBase::set_status(int status)
  {
    m_status = status;
  }


  //|///////////////////// HTTPBase::add_header /////////////////////////////
  void HTTPBase::add_header(string_view header)
  {
    auto div = header.find_first_of(':');

    if (div != string_view::npos)
    {
      auto name = header.substr(0, div);

      while (header[div+1] == ' ')
        ++div;

      auto value = header.substr(div+1, string_view::npos);

      add_header(name.to_string(), value.to_string());
    }
  }


  //|///////////////////// HTTPBase::add_header /////////////////////////////
  void HTTPBase::add_header(string name, string value)
  {
    m_header.emplace(std::move(name), std::move(value));
  }


  //|///////////////////// HTTPBase::add_payload ////////////////////////////
  void HTTPBase::add_payload(string_view buffer)
  {
    add_payload(buffer.data(), buffer.size());
  }


  //|///////////////////// HTTPBase::add_payload ////////////////////////////
  void HTTPBase::add_payload(const char *buffer, size_t bytes)
  {
    m_payload.reserve(m_payload.size() + bytes);
    m_payload.insert(m_payload.end(), buffer, buffer+bytes);
  }


  //|///////////////////// HTTPBase::reserve_payload ////////////////////////
  char *HTTPBase::reserve_payload(size_t bytes)
  {
    m_payload.resize(m_payload.size() + bytes);

    return m_payload.data() + m_payload.size() - bytes;
  }


  //|///////////////////// HTTPBase::release_payload ////////////////////////
  char *HTTPBase::release_payload(size_t bytes)
  {
    m_payload.resize(m_payload.size() - bytes);

    return m_payload.data() + m_payload.size() + bytes;
  }


  //|--------------------- HTTPRequest --------------------------------------
  //|------------------------------------------------------------------------


  //|///////////////////// HTTPRequest::Constructor /////////////////////////
  HTTPRequest::HTTPRequest(int status)
  {
    set_status(status);
  }


  //|///////////////////// HTTPRequest::Constructor /////////////////////////
  HTTPRequest::HTTPRequest(string method, string_view url, string_view payload)
  {
    vector<string_view> groups;

    regex::match("^(.+://|)(.+)(:.+|)(/.*|)$", url, &groups);

    string_view address = (groups.size() < 1 || groups[1].empty()) ? "localhost" : groups[1];
    string_view service = (groups.size() < 2 || groups[2].empty()) ? "80" : groups[2].substr(1);
    string_view location = (groups.size() < 3 || groups[3].empty()) ? "/index.html" : groups[3];

    m_server = address.to_string();
    m_service = service.to_string();

    m_method = std::move(method);
    m_location = location.to_string();

    add_payload(payload);
  }


  //|///////////////////// HTTPRequest::Constructor /////////////////////////
  HTTPRequest::HTTPRequest(string method, string server, string service, string location, string_view payload)
    : m_server(std::move(server)), m_service(std::move(service))
  {
    m_method = std::move(method);
    m_location = std::move(location);

    add_payload(payload);
  }


  //|///////////////////// HTTPRequest::clear ///////////////////////////////
  void HTTPRequest::clear()
  {
    m_method = "";
    m_location = "";

    HTTPBase::clear();
  }


  //|///////////////////// HTTPRequest::set_method //////////////////////////
  void HTTPRequest::set_method(string method)
  {
    m_method = std::move(method);
  }


  //|///////////////////// HTTPRequest::set_location ////////////////////////
  void HTTPRequest::set_location(string location)
  {
    m_location = std::move(location);
  }


  //|///////////////////// HTTPRequest::head ////////////////////////////////
  string HTTPRequest::head() const
  {
    string result;

    result += method() + " " + location() + " HTTP/1.1\r\n";
    result += "Host: " + server() + ":" + service() + "\r\n";

    for(auto &header : headers())
      result += header.first + ": " + header.second + "\r\n";

    result += "Content-Length: " + toa(payload().size()) + "\r\n";

    result += "\r\n";

    return result;
  }



  //|--------------------- HTTPResponse -------------------------------------
  //|------------------------------------------------------------------------


  //|///////////////////// HTTPResponse::Constructor ////////////////////////
  HTTPResponse::HTTPResponse(int status, string statustxt)
  {
    set_status(status);
    set_statustxt(std::move(statustxt));
  }


  //|///////////////////// HTTPResponse::Constructor ////////////////////////
  HTTPResponse::HTTPResponse(string_view payload, string contenttype)
  {
    set_status(200);
    set_statustxt("OK");

    add_header("Content-Type", std::move(contenttype));

    add_payload(payload);
  }


  //|///////////////////// HTTPResponse::head ///////////////////////////////
  void HTTPResponse::set_statustxt(string statustxt)
  {
    m_statustxt = std::move(statustxt);
  }


  //|///////////////////// HTTPResponse::head ///////////////////////////////
  string HTTPResponse::head() const
  {
    string result;

    result += "HTTP/1.1 " + toa(status()) + " " + m_statustxt + "\r\n";

    for(auto &header : headers())
      result += header.first + ": " + header.second + "\r\n";

    result += "Content-Length: " + toa(payload().size()) + "\r\n";

    result += "\r\n";

    return result;
  }



  //|--------------------- HTTPClient ---------------------------------------
  //|------------------------------------------------------------------------


  //|----------- ConnectionPool ---------------------
  //|------------------------------------------------

  class HTTPClient::ConnectionPool
  {
    public:

      struct Connection
      {
        Connection(string_view server, string_view service)
          : server(server.data(), server.size()),
            service(service.data(), service.size())
        {
          socket = make_unique<ClientSocket>(this->server.c_str(), this->service.c_str());

          idletime = time(NULL);
        }

        operator ClientSocket&() { return *socket; }

        string server;
        string service;

        time_t idletime;

        unique_ptr<ClientSocket> socket;
      };

    public:

      ConnectionPool()
      {
        m_threadcontrol.create_thread([=]() { return cleanup_thread(); });
      }

      ~ConnectionPool()
      {
        m_threadcontrol.join_threads();
      }

      Connection acquire(string const &server, string const &service)
      {
        SyncLock M(m_mutex);

        for(auto i = m_connections.begin(); i != m_connections.end(); ++i)
        {
          if (i->server == server && i->service == service && i->socket->connected())
          {
            Connection result = std::move(*i);

            m_connections.erase(i);

            return result;
          }
        }

        return Connection(server, service);
      }

      void release(Connection &&connection)
      {
        SyncLock M(m_mutex);

        connection.idletime = time(NULL);

        m_connections.push_back(std::move(connection));
      }

      long cleanup_thread()
      {
        while (true)
        {
          time_t now = time(NULL);

          {
            SyncLock M(m_mutex);

            m_connections.erase(remove_if(m_connections.begin(), m_connections.end(), [&](Connection const &i) { return difftime(now, i.idletime) > 90.0; }), m_connections.end());
          }

          sleep_til(m_threadcontrol.terminate(), 30000);

          if (m_threadcontrol.terminating())
            return 0;
        }
      }

    private:

      vector<Connection> m_connections;

      CriticalSection m_mutex;

      ThreadControl m_threadcontrol;
  };


  HTTPClient::ConnectionPool &global_connection_pool()
  {
    static std::once_flag onceflag;
    static HTTPClient::ConnectionPool *instance;

    call_once(onceflag, [] { instance = new HTTPClient::ConnectionPool; });

    return *instance;
  }


  //|///////////////////// HTTPClient::perform //////////////////////////////
  bool HTTPClient::perform(HTTPRequest const &request, HTTPResponse *response, leap::threadlib::Waitable *cancel, int timeout, function<size_t (StreamSocket &socket, size_t bytes, HTTPBase *msg)> const &callback)
  {
    response->clear();

    read_http_state state;

    state.callback = std::cref(callback);

    auto connection = global_connection_pool().acquire(request.server(), request.service());

    WaitGroup events;
    events.add(connection.socket->activity());

    if (cancel)
      events.add(*cancel);

    time_t start = time(NULL);

    try
    {
      int step = 0;

      while (true)
      {
        switch (step)
        {
          case 0:

            if (!connection.socket->connect())
              break;

            step = 1;

            [[fallthrough]];

          case 1:

            send_http_request(connection, request);

            step = 2;

            [[fallthrough]];

          case 2:

            if (read_http_response(state, connection, response))
            {
              if (response->header("Connection") == "close")
                connection.socket->close();

              global_connection_pool().release(std::move(connection));

              return true;
            }
        }

        if (!events.wait_any(timeout))
          throw SocketBase::socket_error("Timeout Receiving Payload");

        if (difftime(time(NULL), start) > timeout/1000.0)
          break;

        if (cancel && *cancel)
          break;
      }
    }
    catch(SocketBase::socket_error &)
    {
    }

    response->set_status(408);

    return false;
  }


  //|///////////////////// HTTPClient::execute //////////////////////////////
  bool HTTPClient::execute(HTTPRequest const &request, HTTPResponse *response, int timeout)
  {
    return perform(request, response, nullptr, timeout, read_http_state().callback);
  }


  //|///////////////////// HTTPClient::execute //////////////////////////////
  bool HTTPClient::execute(HTTPRequest const &request, HTTPResponse *response, leap::threadlib::Waitable *cancel, int timeout)
  {
    return perform(request, response, cancel, timeout, read_http_state().callback);
  }


  //|///////////////////// HTTPClient::download /////////////////////////////
  bool HTTPClient::download(HTTPRequest const &request, ofstream &fout, leap::threadlib::Waitable *cancel, int timeout)
  {
    HTTPResponse response;

    auto callback = [&](StreamSocket &socket, size_t bytes, HTTPBase *msg) {
      char buffer[4096];

      size_t n = min(bytes, sizeof(buffer));

      socket.receive(buffer, n);

      fout.write(buffer, n);

      return n;
    };

    return perform(request, &response, cancel, timeout, callback);
  }



  //|--------------------- WebSocketMessage ---------------------------------
  //|------------------------------------------------------------------------


  //|///////////////////// WebSocketMessage::Constructor ////////////////////
  WebSocketMessage::WebSocketMessage()
  {
    m_type = MessageType::Binary;
  }


  //|///////////////////// WebSocketMessage::Constructor ////////////////////
  WebSocketMessage::WebSocketMessage(string_view payload)
  {
    m_type = MessageType::Text;

    add_payload(payload);
  }


  //|///////////////////// WebSocketMessage::clear //////////////////////////
  void WebSocketMessage::clear()
  {
    m_payload.clear();
  }


  //|///////////////////// WebSocketMessage::set_type ///////////////////////
  void WebSocketMessage::set_type(WebSocketMessage::MessageType type)
  {
    m_type = type;
  }


  //|///////////////////// WebSocketMessage::set_endpoint ///////////////////
  void WebSocketMessage::set_endpoint(string endpoint)
  {
    m_endpoint = std::move(endpoint);
  }


  //|///////////////////// WebSocketMessage::add_payload ////////////////////
  void WebSocketMessage::add_payload(string_view buffer)
  {
    add_payload(buffer.data(), buffer.size());
  }


  //|///////////////////// WebSocketMessage::add_payload ////////////////////
  void WebSocketMessage::add_payload(const char *buffer, size_t bytes)
  {
    m_payload.reserve(m_payload.size() + bytes);
    m_payload.insert(m_payload.end(), buffer, buffer+bytes);
  }


  //|///////////////////// WebSocketMessage::reserve_payload ////////////////
  char *WebSocketMessage::reserve_payload(size_t bytes)
  {
    m_payload.resize(m_payload.size() + bytes);

    return m_payload.data() + m_payload.size() - bytes;
  }


  //|///////////////////// WebSocketMessage::release_payload ////////////////
  char *WebSocketMessage::release_payload(size_t bytes)
  {
    m_payload.resize(m_payload.size() - bytes);

    return m_payload.data() + m_payload.size() + bytes;
  }



  //|--------------------- WebSocket ----------------------------------------
  //|------------------------------------------------------------------------


  //|///////////////////// WebSocket::Constructor ///////////////////////////
  WebSocket::WebSocket()
  {
    m_state = SocketState::Unborn;

    m_onconnect = []() { };
    m_onmessage= [](WebSocketMessage const &) { };
    m_ondisconnect = []() { };
  }


  //|///////////////////// WebSocket::Constructor ///////////////////////////
  WebSocket::WebSocket(string url, string protocols)
    : WebSocket()
  {
    create(std::move(url), std::move(protocols));
  }


  //|///////////////////// WebSocket::Destructor ////////////////////////////
  WebSocket::~WebSocket()
  {
    destroy();
  }


  //|///////////////////// WebSocket::create ////////////////////////////////
  bool WebSocket::create(string url, string protocols)
  {
    assert(m_state == SocketState::Unborn || m_state == SocketState::Dead);

    m_url = std::move(url);
    m_protocols = std::move(protocols);

    vector<string_view> groups;

    regex::match("^(.+://)(.+)(:.+|)(/.*)$", m_url, &groups);

    string_view address = (groups.size() < 1 || groups[1].empty()) ? "localhost" : groups[1];
    string_view service = (groups.size() < 2 || groups[2].empty()) ? "80" : groups[2].substr(1);

    m_socket.create(address, service);

    m_threadcontrol.create_thread(this, &WebSocket::WebSocketThread);

    return true;
  }


  //|///////////////////// WebSocket::destroy ///////////////////////////////
  void leap::socklib::WebSocket::destroy()
  {
    m_threadcontrol.join_threads();

    m_socket.destroy();

    m_state = SocketState::Dead;
  }


  //|///////////////////// WebSocket::state /////////////////////////////////
  WebSocket::SocketState WebSocket::state() const
  {
    return m_state;
  }


  //|///////////////////// WebSocket::connect ///////////////////////////////
  bool WebSocket::connect(int timeout)
  {
    if (m_socket.connect())
    {
      try
      {
        vector<uint8_t> nonce(16, 0);

        for(auto i = nonce.begin(); i != nonce.end(); ++i)
          *i = rnd8();

        HTTPRequest request("GET", m_url);

        request.add_header("Origin: http://" + request.server());
        request.add_header("Upgrade: websocket");
        request.add_header("Sec-WebSocket-Version: 13");
        request.add_header("Sec-WebSocket-Protocol: " + m_protocols);
        request.add_header("Sec-WebSocket-Key: " + base64_encode(nonce));
        request.add_header("Connection: keep-alive, upgrade");

        send_http_request(m_socket, request);

        HTTPResponse response;

        m_socket.wait_on_bytes(1, timeout);

        read_http_response(m_socket, &response, timeout);

        if (response.status() == 101 && response.header("Sec-WebSocket-Accept") == websocket_accept_key(base64_encode(nonce)))
        {
          m_state = SocketState::Connected;

          return true;
        }
      }
      catch(SocketBase::socket_error &)
      {
      }

      m_socket.close();
    }

    return false;
  }


  //|///////////////////// WebSocket::ping //////////////////////////////////
  bool WebSocket::ping(const void *buffer, size_t bytes, uint32_t maskkey)
  {
    if (m_state != SocketState::Connected)
      return false;

    try
    {
      return send_websocket_message(m_socket, buffer, bytes, 9, true, maskkey);
    }
    catch(SocketBase::socket_error &)
    {
      m_socket.close();
    }

    return false;
  }


  //|///////////////////// WebSocket::send //////////////////////////////////
  bool WebSocket::send(WebSocketMessage const &message, uint32_t maskkey)
  {
    if (m_state != SocketState::Connected)
      return false;

    try
    {
      return send_websocket_message(m_socket, message.payload().data(), message.payload().size(), static_cast<int>(message.type()), true, maskkey);
    }
    catch(SocketBase::socket_error &)
    {
      m_socket.close();
    }

    return false;
  }


  //|///////////////////// WebSocket::send //////////////////////////////////
  bool WebSocket::send(const void *buffer, size_t bytes, uint32_t maskkey)
  {
    if (m_state != SocketState::Connected)
      return false;

    try
    {
      return send_websocket_message(m_socket, buffer, bytes, 2, true, maskkey);
    }
    catch(SocketBase::socket_error &)
    {
      m_socket.close();
    }

    return false;
  }


  //|///////////////////// WebSocket::receive ///////////////////////////////
  bool WebSocket::receive(WebSocketMessage *message, int timeout)
  {
    if (m_state != SocketState::Connected)
      return false;

    try
    {
      message->set_endpoint(m_url);

      return read_websocket_message(m_socket, message, timeout);
    }
    catch(SocketBase::socket_error &)
    {
      m_socket.close();
    }

    return false;
  }


  //|///////////////////// WebSocket::close /////////////////////////////////
  void WebSocket::close()
  {
    m_socket.close();
  }


  //|///////////////////// WebSocket::WebSocketThread ///////////////////////
  //
  // WebSocket Thread... Connects and reconnects websocket
  //
  long WebSocket::WebSocketThread()
  {
    WaitGroup events;
    events.add(m_socket.activity());
    events.add(m_threadcontrol.terminate());

    m_state = SocketState::Created;

    while (true)
    {
      if (connect())
      {
        m_onconnect();

        while (m_socket.connected())
        {
          WebSocketMessage message;

          while (receive(&message))
          {
            m_onmessage(message);
          }

          sleep_any(events);

          if (m_threadcontrol.terminating())
            break;
        }

        m_ondisconnect();

        m_state = SocketState::Cactus;
      }
      else
      {
        sleep_any(events);
      }

      if (m_threadcontrol.terminating())
        break;
    }

    m_state = SocketState::Dead;

    return 0;
  }


  //|////////////////////////// websocket_accept_key ////////////////////////
  string websocket_accept_key(string key)
  {
    key += "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

    return base64_encode(sha1digest(key.data(), key.size()).data(), sha1::size());
  }


  //|///////////////////// websocket_mask_data //////////////////////////////
  uint32_t websocket_mask_data(void *buffer, size_t bytes)
  {
    uint8_t maskkey[4] = { rnd8(), rnd8(), rnd8(), rnd8() };

    for(size_t i = 0; i < bytes; ++i)
    {
      ((uint8_t*)buffer)[i] ^= maskkey[i % 4];
    }

    return (maskkey[0] << 24) + (maskkey[1] << 16) + (maskkey[2] << 8) + (maskkey[3] << 0);
  }



  //|--------------------- HTTPServer ---------------------------------------
  //|------------------------------------------------------------------------


  //|///////////////////// HTTPServer::Constructor //////////////////////////
  HTTPServer::HTTPServer()
  {
  }


  //|///////////////////// HTTPServer::Destructor ///////////////////////////
  HTTPServer::~HTTPServer()
  {
    terminate();
  }


  //|///////////////////// HTTPServer::send /////////////////////////////////
  bool HTTPServer::send(socket_t connection, HTTPResponse const &response)
  {
    try
    {
      send_http_response(connection->socket, response);

      return true;
    }
    catch(SocketBase::socket_error &)
    {
    }

    return false;
  }


  //|///////////////////// HTTPServer::send_file ////////////////////////////
  bool HTTPServer::send_file(socket_t connection, const char *path, const char *contenttype)
  {
    try
    {
      char buffer[4096];

      ifstream fin(path, ios_base::in | ios_base::binary);
      if (!fin)
      {
        sprintf(buffer, "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n");

        connection->socket.transmit(buffer, strlen(buffer));

        return false;
      }

      fin.seekg(0, std::ios_base::end);
      size_t contentlength = fin.tellg();
      fin.seekg(0, std::ios_base::beg);

      sprintf(buffer, "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-Length: %zu\r\n\r\n", contenttype, contentlength);

      connection->socket.transmit(buffer, strlen(buffer));

      while (auto bytes = fin.readsome(buffer, sizeof(buffer)))
      {
        connection->socket.transmit(buffer, bytes);
      }

      return true;
    }
    catch(SocketBase::socket_error &)
    {
    }

    return false;
  }


  //|///////////////////// HTTPServer::send /////////////////////////////////
  bool HTTPServer::send(socket_t connection, WebSocketMessage const &message)
  {
    try
    {
      send_websocket_message(connection->socket, message.payload().data(), message.payload().size(), static_cast<int>(message.type()), false);
    }
    catch(SocketBase::socket_error &)
    {
    }

    return false;
  }


  //|///////////////////// HTTPServer::broadcast ////////////////////////////
  void HTTPServer::broadcast(string_view endpoint, WebSocketMessage const &message, socket_t ignore)
  {
    SyncLock M(m_mutex);

    for(auto &connection : m_connections)
    {
      if (connection.get() == ignore)
        continue;

      if (connection->endpoint != endpoint)
        continue;

      try
      {
        send_websocket_message(connection->socket, message.payload().data(), message.payload().size(), static_cast<int>(message.type()), false);
      }
      catch(SocketBase::socket_error &)
      {
      }
    }
  }


  //|///////////////////// HTTPServer::drop /////////////////////////////////
  void HTTPServer::drop(socket_t connection)
  {
    connection->socket.close();
  }


  //|///////////////////// HTTPServer::start ////////////////////////////////
  void HTTPServer::start(unsigned int bindport, int threads)
  {
    m_threadcontrol.create_thread([=]() { return listen_thread(bindport); });

    m_threadcontrol.create_thread([=]() { return select_thread(); });

    for(int i = 0; i < threads; ++i)
      m_threadcontrol.create_thread([=]() { return worker_thread(); });
  }


  //|///////////////////// HTTPServer::terminate ////////////////////////////
  void HTTPServer::terminate()
  {
    m_threadcontrol.join_threads();
  }


  //|///////////////////// HTTPServer::listen_thread ////////////////////////
  long HTTPServer::listen_thread(unsigned int bindport)
  {
    leap::socklib::SocketPump socketpump(bindport);

    WaitGroup events;
    events.add(socketpump.activity());
    events.add(m_threadcontrol.terminate());

    while (true)
    {
      socklib::socket_t socket;
      socklib::sockaddr_t addr;

      while (socketpump.accept_connection(&socket, &addr))
      {
        Connection *connection = new Connection(socket);

        {
          SyncLock M(m_mutex);

          m_connections.push_back(unique_ptr<Connection>(connection));
        }

        sigAccept(connection, &addr);

        m_selectqueue.push(connection);
      }

      sleep_any(events);

      if (m_threadcontrol.terminating())
        return 0;
    }
  }


  //|///////////////////// HTTPServer::select_thread ////////////////////////
  long HTTPServer::select_thread()
  {
    WaitGroup events;
    events.add(m_selectqueue.activity());
    events.add(m_threadcontrol.terminate());

    vector<Connection*> idle;

    while (true)
    {
      bool exhausted = true;

      Connection *connection;

      if (m_selectqueue.pop(&connection))
      {
        idle.push_back(connection);
        events.add(connection->socket.activity());

        exhausted = false;
      }

      for(auto i = idle.begin(); i != idle.end(); )
      {
        connection = *i;

        if (connection->socket.bytes_available() || !connection->socket.connected())
        {
          events.remove(connection->socket.activity());

          m_workerqueue.push(connection);

          i = idle.erase(i);

          exhausted = false;
        }
        else
          ++i;
      }

      if (exhausted)
      {
        sleep_any(events);
      }

      if (m_threadcontrol.terminating())
        return 0;
    }
  }


  //|///////////////////// HTTPServer::worker_thread ////////////////////////
  long HTTPServer::worker_thread()
  {
    WaitGroup events;
    events.add(m_workerqueue.activity());
    events.add(m_threadcontrol.terminate());

    while (true)
    {
      Connection *connection;

      while (m_workerqueue.pop(&connection))
      {
        if (connection->socket.connected())
        {
          try
          {
            if (connection->type == Connection::SocketType::Http)
            {
              HTTPRequest request;

              if (read_http_request(connection->socket, &request))
              {
                if (request.header("Upgrade") == "websocket")
                {
                  sigUpgrade(connection, request);

                  connection->type = Connection::SocketType::WebSocket;

                  connection->endpoint = request.location();
                }
                else
                {
                  sigRespond(connection, request);
                }
              }
            }

            if (connection->type == Connection::SocketType::WebSocket)
            {
              WebSocketMessage message;

              message.set_endpoint(connection->endpoint);

              if (read_websocket_message(connection->socket, &message))
              {
                sigReceive(connection, message);
              }
            }
          }
          catch(SocketBase::socket_error &)
          {
            connection->socket.close();
          }

          m_selectqueue.push(connection);
        }
        else
        {
          sigDisconnect(connection);

          connection->socket.destroy();

          {
            SyncLock M(m_mutex);

            m_connections.erase(find_if(m_connections.begin(), m_connections.end(), [&](unique_ptr<Connection> const &i) { return i.get() == connection; }));
          }
        }
      }

      sleep_any(events);

      if (m_threadcontrol.terminating())
        return 0;
    }
  }



  //|--------------------- Functions ----------------------------------------
  //|------------------------------------------------------------------------


  //////////////////////////// base64_encode ////////////////////////////////
  string base64_encode(vector<uint8_t> const &payload)
  {
    return base64_encode(payload.data(), payload.size());
  }


  //////////////////////////// base64_encode ////////////////////////////////
  string base64_encode(const void *payload, size_t size)
  {
    static const char encode[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    string result;

    for(uint8_t *in = (uint8_t*)payload; size > 0; )
    {
      int len = 0;
      uint8_t trip[3] = { 0, 0, 0 };

      for(int i = 0; i < 3 && size > 0; ++i, ++in, --size)
      {
        trip[len++] = *in;
      }

      result += encode[trip[0] >> 2];
      result += encode[((trip[0] & 0x03) << 4) | ((trip[1] & 0xf0) >> 4)];
      result += (len > 1) ? encode[((trip[1] & 0x0f) << 2) | ((trip[2] & 0xc0) >> 6)] : '=';
      result += (len > 2) ? encode[((trip[2] & 0x3f))] : '=';
    }

    return result;
  }


  //////////////////////////// base64_decode ////////////////////////////////
  vector<uint8_t> base64_decode(string_view payload)
  {
    static const char decode[] = "|$$$}rstuvwxyz{$$$$$$$>?@ABCDEFGHIJKLMNOPQRSTUVW$$$$$$XYZ[\\]^_`abcdefghijklmnopq";

    vector<uint8_t> result;

    for(auto in = payload.begin(); in != payload.end(); )
    {
      int len = 0;
      uint8_t quad[4];

      for(int i = 0; i < 4 && in != payload.end(); ++i, ++in)
      {
        if (*in > 42 && *in < 123)
          quad[len++] = decode[*in - 43] - 62;
      }

      if (len > 1)
        result.push_back(quad[0] << 2 | quad[1] >> 4);

      if (len > 2)
        result.push_back(quad[1] << 4 | quad[2] >> 2);

      if (len > 3)
        result.push_back(((quad[2] << 6) & 0xc0) | quad[3]);
    }

    return result;
  }

} } // namespace socklib
