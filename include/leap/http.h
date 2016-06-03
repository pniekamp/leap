//
// http protocol helpers
//
//   Peter Niekamp, February, 2008
//

//
// Copyright (C) Peter Niekamp
//
// This code remains the property of the copyright holder.
// The code contained herein may be used for any purpose provided
// this copyright notice is retained
//

#ifndef HTTPLIB_HH
#define HTTPLIB_HH

#include <leap/sockets.h>
#include <leap/siglib.h>
#include <leap/concurrentqueue.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <algorithm>


/**
 * \namespace leap::socklib
 * \brief TCP/IP Sockets
 *
**/

namespace leap { namespace socklib
{

  //-------------------------- HTTPBase ---------------------------------------
  //---------------------------------------------------------------------------

  class HTTPBase
  {
    public:
      HTTPBase();

      int status() const { return m_status; }

      std::string header(std::string const &name, std::string const &defaultvalue = "") const;

      std::unordered_map<std::string, std::string> const &headers() const { return m_header; }

      std::vector<char> const &payload() const { return m_payload; }

      virtual std::string head() const = 0;

    public:

      void clear();

      void set_status(int status);

      void add_header(std::string const &header);

      void add_payload(const std::string &buffer);
      void add_payload(const char *buffer, size_t bytes);

      char *reserve_payload(size_t bytes);
      char *release_payload(size_t bytes);

    private:

      int m_status;

      std::unordered_map<std::string, std::string> m_header;

      std::vector<char> m_payload;
  };


  //-------------------------- HTTPRequest ------------------------------------
  //---------------------------------------------------------------------------
  /**
   * \brief HTTP Request
   *
  **/

  class HTTPRequest : public HTTPBase
  {
    public:
      explicit HTTPRequest(int status = 408);
      explicit HTTPRequest(std::string const &method, std::string const &url, std::string const &payload = "");
      explicit HTTPRequest(std::string const &method, std::string const &server, unsigned int port, std::string const &location, std::string const &payload = "");

      std::string const &server() const { return m_server; }
      unsigned int port() const { return m_port; }

      std::string const &method() const { return m_method; }
      std::string const &location() const { return m_location; }

      virtual std::string head() const;

    public:

      void clear();

      void set_method(std::string const &method);
      void set_location(std::string const &location);

    private:

      std::string m_server;
      unsigned int m_port;

      std::string m_method;
      std::string m_location;
  };


  //-------------------------- HTTPResponse -----------------------------------
  //---------------------------------------------------------------------------
  /**
   * \brief HTTP Response
   *
  **/

  class HTTPResponse : public HTTPBase
  {
    public:
      explicit HTTPResponse(int status = 200, std::string const &statustxt = "OK");
      explicit HTTPResponse(std::string const &payload, std::string const &contenttype = "text/html");

      void set_statustxt(std::string const &statustxt);

      virtual std::string head() const;

    private:

      std::string m_statustxt;
  };



  //-------------------------- HTTPClient -------------------------------------
  //---------------------------------------------------------------------------
  /**
   * \brief HTTP Client Helpers
   *
  **/

  class HTTPClient
  {
    public:

      static bool execute(HTTPRequest const &request, HTTPResponse *response, int timeout = 30000);
      static bool execute(HTTPRequest const &request, HTTPResponse *response, leap::threadlib::Waitable *cancel, int timeout = 30000);

      static bool download(HTTPRequest const &request, std::ofstream &fout, leap::threadlib::Waitable *cancel, int timeout = 30000);

    public:

      class ConnectionPool;

      static bool perform(HTTPRequest const &request, HTTPResponse *response, leap::threadlib::Waitable *cancel, int timeout, std::function<int (StreamSocket &socket, size_t bytes, HTTPBase *msg)> const &callback);
  };



  //-------------------------- WebSocketMessage -------------------------------
  //---------------------------------------------------------------------------
  /**
   * \brief WebSocket Message
   *
  **/

  class WebSocketMessage
  {
    public:

      enum class MessageType
      {
        Text = 1,
        Binary = 2
      };

    public:
      WebSocketMessage();
      explicit WebSocketMessage(std::string const &payload);

      MessageType type() const { return m_type; }

      std::string endpoint() const { return m_endpoint; }

      std::vector<char> const &payload() const { return m_payload; }

    public:

      void clear();

      void set_type(MessageType type);

      void set_endpoint(std::string const &endpoint);

      void add_payload(const std::string &buffer);
      void add_payload(const char *buffer, size_t bytes);

      char *reserve_payload(size_t bytes);
      char *release_payload(size_t bytes);

    private:

      MessageType m_type;

      std::string m_endpoint;

      std::vector<char> m_payload;
  };


  //-------------------------- WebSocket --------------------------------------
  //---------------------------------------------------------------------------
  /**
   * \brief WebSocket
   *
  **/

  class WebSocket
  {
    public:

      enum class SocketState
      {
        Unborn,      // not yet created.
        Created,     // created, ready to connect
        Connected,   // connected successfully
        Cactus,      // connection lost, ready to connect
        Dead         // socket incapable of connection
      };

    public:
      WebSocket();
      explicit WebSocket(std::string const &url, std::string const &protocols = "");
      ~WebSocket();

      bool create(std::string const &url, std::string const &protocols = "");

      void destroy();

    public:

      SocketState state() const;

      bool ping(const void *buffer = "", size_t bytes = 0, uint32_t maskkey = 0);

      bool send(WebSocketMessage const &message, uint32_t maskkey = 0);
      bool send(const void *buffer, size_t bytes, uint32_t maskkey = 0);

      template<typename Function>
      void onconnect(Function function)
      {
        m_onconnect = std::move(function);
      }

      template<typename Function>
      void onmessage(Function function)
      {
        m_onmessage = std::move(function);
      }

      template<typename Function>
      void ondisconnect(Function function)
      {
        m_ondisconnect = std::move(function);
      }

      void close();

    private:

      SocketState m_state;

      std::string m_url;
      std::string m_protocols;

      /*std::atomic<*/std::function<void ()> m_onconnect;
      /*std::atomic<*/std::function<void (WebSocketMessage const &)> m_onmessage;
      /*std::atomic<*/std::function<void ()> m_ondisconnect;

      leap::socklib::ClientSocket m_socket;

      threadlib::ThreadControl m_threadcontrol;

      bool connect(int timeout = 20000);
      bool receive(WebSocketMessage *message, int timeout = 2000);

      long WebSocketThread();
  };

  std::string websocket_accept_key(std::string key);
  uint32_t websocket_mask_data(void *buffer, size_t bytes);


  //-------------------------- HTTPServer -------------------------------------
  //---------------------------------------------------------------------------
  /**
   * \brief Basic HTTP Server
   *
  **/

  class HTTPServer
  {
    class Connection;

    public:

      typedef Connection *socket_t;
      typedef leap::socklib::sockaddr_t sockaddr_t;

      leap::siglib::Signal<void (socket_t, sockaddr_t*)> sigAccept;
      leap::siglib::Signal<void (socket_t, HTTPRequest const &)> sigRespond;

      leap::siglib::Signal<void (socket_t, HTTPRequest const &)> sigUpgrade;
      leap::siglib::Signal<void (socket_t, WebSocketMessage const &)> sigReceive;

      leap::siglib::Signal<void (socket_t)> sigDisconnect;

    public:
      HTTPServer();
      ~HTTPServer();

      bool send(socket_t connection, HTTPResponse const &response);
      bool send_file(socket_t connection, std::string const &path, std::string const &contenttype);

      bool send(socket_t connection, WebSocketMessage const &message);

      void broadcast(std::string const &endpoint, WebSocketMessage const &message, socket_t ignore = 0);

      void drop(socket_t connection);

    public:

      void start(unsigned int bindport, int threads = 2);

      void terminate();

    private:

      class Connection
      {
        enum class SocketType
        {
          Http,
          WebSocket
        };

        Connection(leap::socklib::socket_t socket)
          : socket(socket)
        {
          type = SocketType::Http;
        }

        SocketType type;

        std::string endpoint;

        leap::socklib::ServerSocket socket;

        friend class HTTPServer;
      };

      std::vector<std::unique_ptr<Connection>> m_connections;

      long listen_thread(unsigned int bindport);
      long select_thread();
      long worker_thread();

      leap::threadlib::ConcurrentQueue<Connection*> m_selectqueue;
      leap::threadlib::ConcurrentQueue<Connection*> m_workerqueue;

      leap::threadlib::CriticalSection m_mutex;

      leap::threadlib::ThreadControl m_threadcontrol;
  };


  //-------------------------- Functions -------------------------------------
  //--------------------------------------------------------------------------

  // base64
  std::string base64_encode(std::vector<uint8_t> const &payload);
  std::string base64_encode(const void *payload, unsigned long size);
  std::vector<uint8_t> base64_decode(std::string const &payload);

} } // namespace socklib

#endif // HTTPLIB_HH
