//
// TCP/IP Sockets class definition.
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

#pragma once

#include <leap/stringview.h>
#include <cstdio>
#include <array>
#include <string>
#include <stdexcept>
#include <functional>
#include <atomic>

#ifdef _WIN32
#  ifdef __AFXSOCK_H__
#    undef socket
#    undef connect
#  endif
#  include <ws2tcpip.h>
#else
#  include <sys/types.h>
#  include <sys/socket.h>
#  include <netdb.h>
#  define SOCKET int
#endif

#include <leap/threadcontrol.h>

/**
 * \namespace leap::socklib
 * \brief TCP/IP Sockets
 *
**/

namespace leap { namespace socklib
{
  struct socket_t
  {
    SOCKET sid;
  };

  struct interface_t
  {
    std::string name;
    unsigned int ip;
    unsigned int mask;
    unsigned int bcast;
  };

  using sockaddr_t = sockaddr_in6;

  ///////////////////// InitialiseSocketSubsystem /////////////////////////////
  bool InitialiseSocketSubsystem();

  ///////////////////// CloseSocketSubsystem //////////////////////////////////
  bool CloseSocketSubsystem();


  // misc

  inline bool operator ==(socket_t const &lhs, socket_t const &rhs)
  {
    return (lhs.sid == rhs.sid);
  }

  inline bool operator !=(socket_t const &lhs, socket_t const &rhs)
  {
    return (lhs.sid != rhs.sid);
  }

  inline bool operator <(socket_t const &lhs, socket_t const &rhs)
  {
    return (lhs.sid < rhs.sid);
  }


  //-------------------------- SocketBase -------------------------------------
  //---------------------------------------------------------------------------

  class SocketBase
  {
    public:

      enum class SocketStatus
      {
        Unborn,      // not yet created.
        Created,     // created, but never connected
        Connected,   // connected successfully
        Cactus,      // connection lost, reconnectable
        Dead         // socket incapable of connection
      };

      class socket_error : public std::runtime_error
      {
        public:
          socket_error(std::string const &arg)
            : std::runtime_error(arg)
          {
          }
      };

    public:

      bool connected() const;

      SocketStatus status() const;
      std::string statustxt() const;

      bool wait_on_activity(int timeout = -1);

    public:

      threadlib::Waitable &activity() { return m_activity; }

    protected:

      SocketBase();
      ~SocketBase() = default;

      void destroy();

    protected:

      SOCKET m_connectedsocket;

      std::atomic<SocketStatus> m_status;
      std::atomic<unsigned int> m_errorcondition;

      threadlib::Semaphore m_activity;
  };


  //-------------------------- StreamSocket -----------------------------------
  //---------------------------------------------------------------------------
  /**
   * \brief StreamSocket
   *
   * The Sockets class implements a TCP/IP sockets based transport. Sockets
   * can be used for communication between processes on the same machine or
   * across a network to another machine. The specific application is defined
   * via the use of a port.
   *
   * The Server Socket is always created on the local machine and will bind to
   * the specified port. It will then wait for a client connection.
   *
   * The Client Socket will connect to an existing Server Socket defined by a
   * Machine Name (IP Address) and a destination port number.
   *
   * An extension to the Server Socket is the Socket Pump. This binds to the
   * Server Port (on the local machine) and waits for client connections. Once
   * a client connects, the corresponding Server Socket instance is passed out,
   * and the pump returns to waiting for a client connection. Hence, multiple
   * clients can connect to a Server, each receiving their own instance of a
   * server socket connection.
   *
   * Operations on sockets occur in a semi-nonblocking mode. Outgoing data
   * transmits into the TCP Stack and will not block unless the Stack is full.
   * Incoming data is received by a thread of the socket class and placed
   * into the internal read buffer. While the receive operations in the thread
   * may block, operations on the read buffer are nonblocking.
   *
   * The thread within the socket class will detect connection loss and set the
   * socket state accordingly. A socket may be set to automatically reconnect,
   * in which case, the class will handle all the reconnection semantics.
   * Note that a server socket connected via a socket pump will never be set
   * to automatically reconnect, as this will be handled by the pump issuing
   * a new connection.
   *
   * To reduce polling of the socket to receive data, the Wait Functions should
   * be used. Typically, WaitOnBytes will efficiently wait until either a timeout
   * occurs or a specified number of bytes are available on the socket.
   *
   * Link against WS2_32.lib on windows
   *
  **/

  class StreamSocket : public SocketBase
  {
    public:

      size_t bytes_available() const;

      bool wait_on_bytes(size_t minbytes = 1, int timeout = -1);

      void transmit(const void *buffer, size_t bytes);

      size_t receive(void *buffer, size_t n);

      void discard(size_t bytes);

      void close();

    public:

      uint8_t peek(size_t i) const { return m_buffer[(m_bufferhead + i) % m_buffer.size()]; }

    protected:

      StreamSocket();
      ~StreamSocket() = default;

      enum class StreamState
      {
        Ok,
        Stalled,
        Dead
      };

      StreamState init_stream();
      StreamState read_stream();

      void destroy();

    private:

      size_t m_bufferhead;
      size_t m_buffertail;
      std::atomic<size_t> m_buffercount;
      std::array<uint8_t, 16384> m_buffer;

      std::atomic<bool> m_closesignal;
  };



  //-------------------------- ServerSocket -----------------------------------
  //---------------------------------------------------------------------------
  /**
   * \brief Server Socket
   *
   * Server Socket, opens a tcp port and listens for a single connection
   *
   * \see leap::socklib::StreamSocket
   *
  **/

  class ServerSocket : public StreamSocket
  {
    public:
      ServerSocket();
      explicit ServerSocket(unsigned int port, const char *options = "");
      explicit ServerSocket(socket_t socket, const char *options = "");
      ~ServerSocket();

      bool create(unsigned int port, const char *options = "");

      bool attach(socket_t socket, const char *options = "");

      void destroy();

    private:

      void parse_options(const char *options);
      bool create_and_bind();
      void close_and_invalidate();
      void close_listener();
      void handle_created();
      void handle_connected();
      void handle_disconnect();

      unsigned int m_port;

      SOCKET m_listeningsocket;

      enum { Connectable = 0x01, KeepAlive = 0x04 };

      long m_options;

      std::atomic<int> m_destroysignal;
  };



  //-------------------------- ClientSocket -----------------------------------
  //---------------------------------------------------------------------------
  /**
   * \brief Client Socket
   *
   * Client Socket, connects to an open tcp port
   *
   * \see leap::socklib::StreamSocket
   *
  **/

  class ClientSocket : public StreamSocket
  {
    public:
      ClientSocket();
      explicit ClientSocket(string_view address, string_view service, const char *options = "");
      ~ClientSocket();

      bool create(string_view address, string_view service, const char *options = "");

      void destroy();

      bool wait_on_connect(int timeout = -1);

      bool connect();

    private:

      void parse_options(const char *options);
      bool create_and_connect_socket();
      void close_and_invalidate();
      void handle_created();
      void handle_connected();
      void handle_disconnect();

      char m_address[256];
      char m_service[128];

      enum { keepalive = 0x04 };

      long m_options;

      threadlib::Event m_connect;

      std::atomic<int> m_destroysignal;
  };


  //-------------------------- SocketPump -------------------------------------
  //---------------------------------------------------------------------------
  /**
   * \brief Socket Pump
   *
   * Socket Pump, opens a tcp port and listens for multiple connections
   *
   * \see leap::socklib::ClientSocket
   *
  **/

  class SocketPump
  {
    public:

      class socketpump_error : public std::runtime_error
      {
        public:
          socketpump_error(std::string const &arg)
            : std::runtime_error(arg)
          {
          }
      };

    public:
      SocketPump();
      explicit SocketPump(unsigned int port);
      ~SocketPump();

      unsigned int port() const { return m_port; }

    public:

      bool create(unsigned int port);

      void destroy();

      bool wait_for_connection(int timeout = -1);

      bool accept_connection(socket_t *socket, sockaddr_t *addr = nullptr);

    public:

      threadlib::Waitable &activity() { return m_activity; }

    private:

      bool create_and_bind();
      void close_listener();
      void handle_created();

      unsigned int m_port;

      SOCKET m_listeningsocket;

      unsigned int m_errorcondition;

      threadlib::Semaphore m_activity;

      std::atomic<int> m_destroysignal;
  };



  //-------------------------- BroadcastSocket --------------------------------
  //---------------------------------------------------------------------------
  /**
   * \brief Broadcast Socket
   *
   * Receives & Transmits udp broadcasts
   *
   * \see leap::socklib::SocketBase
   *
  **/

  class BroadcastSocket : public SocketBase
  {
    public:
      BroadcastSocket();
      explicit BroadcastSocket(unsigned int port, const char *options = "");
      explicit BroadcastSocket(unsigned int address, unsigned int port, const char *options = "");
      ~BroadcastSocket();

      unsigned int port() const { return m_port; }
      unsigned int address() const { return m_address; }

    public:

      bool create(unsigned int port, const char *options = "");
      bool create(unsigned int address, unsigned int port, const char *options = "");

      void destroy();

      bool packet_available();

      bool wait_on_packet(int timeout = -1);

      void broadcast(const void *buffer, size_t bytes, unsigned int ip, int port);

      size_t receive(void *buffer, size_t n, sockaddr_in *addr = nullptr);

    private:

      size_t m_bufferhead;
      size_t m_buffertail;
      std::atomic<size_t> m_buffercount;
      std::array<uint8_t, 16384> m_buffer;

    private:

      void parse_options(const char *options);
      bool create_and_bind();
      void close_and_invalidate();
      void handle_created();

      unsigned int m_port;
      unsigned int m_address;

      long m_options;

      std::atomic<int> m_destroysignal;
  };


  //-------------------------- Functions -------------------------------------
  //--------------------------------------------------------------------------

  bool readline(StreamSocket &socket, char *buffer, size_t n, int timeout = 0);

  std::vector<interface_t> interfaces();

} } // namespace socklib
