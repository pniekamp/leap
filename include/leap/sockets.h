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

#ifndef SOCKLIB_HH
#define SOCKLIB_HH

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
#  include <winsock2.h>
#else
#  include <sys/socket.h>
#  include <netinet/in.h>
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

  typedef sockaddr_in sockaddr_t;

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

      //-------------------------- socket_error -----------------------------
      //---------------------------------------------------------------------
      class socket_error : public std::runtime_error
      {
        public:
          socket_error(std::string const &arg)
            : runtime_error(arg)
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

      SocketStatus m_status;
      unsigned int m_errorcondition;

      threadlib::Semaphore m_activity;
  };


  //-------------------------- StreamSocket -----------------------------------
  //---------------------------------------------------------------------------
  /**
   * \brief StreamSocket
   *
   * The Sockets class implements a TCP/IP sockets based transport. Sockets
   * can be used for communication between processes on the same machine or
   * accross a network to another machine. The specific application is defined
   * via the use of a port.
   *
   * The Server Socket is always created on the local machine and will bind to
   * the specified port. It will then wait for a client connection.
   *
   * The Client Socket will connect to an existing Server Socket defined by a
   * Machine Name (IP Address) and a destination port number.
   *
   * An extention to the Server Socket is the Socket Pump. This binds to the
   * Server Port (on the local machine) and waits for client connections. Once
   * a client connects, the corresponding Server Socket instance is passed out,
   * and the pump returns to waiting for a client connection. Hence, multiple
   * clients can connect to a Server, each receiving their own instance of a
   * server socket connection.
   *
   * Operations on sockets occur in a semi-nonblocking mode. Outgoing data
   * transmits into the TCP Stack and will not block unless the Stack is full.
   * Incomming data is received by a thread of the socket class and placed
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
   * be used. Typically, WaitOnBytes will effeciently wait until either a timeout
   * occurs or a specified number of bytes are available on the socket.
   *
   * Link agains WS2_32.lib on windows
   *
  **/

  class StreamSocket : public SocketBase
  {
    public:

      static constexpr size_t kSocketBufferSize = 16384;

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

      void connected_loop(threadlib::Waitable &cancel);

      void destroy();

    private:

      size_t m_bufferhead;
      size_t m_buffertail;
      std::atomic<size_t> m_buffercount;
      std::array<uint8_t, kSocketBufferSize> m_buffer;
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
      bool listen_and_accept();
      void close_and_invalidate();
      void close_listener();

      sockaddr_t m_sockaddr;
      SOCKET m_listeningsocket;

      enum { Connectable = 0x01, KeepAlive = 0x04 };

      long m_options;

      threadlib::ThreadControl m_threadcontrol;

      long ServerSocketThread();
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
      explicit ClientSocket(const char *ipaddress, unsigned int port, const char *options = "");
      ~ClientSocket();

      bool create(const char *ipaddress, unsigned int port, const char *options = "");

      bool wait_on_connect(int timeout = -1);

      bool connect();

      void destroy();

    private:

      void parse_options(const char *options);
      bool create_socket();
      bool connect_socket();
      void close_socket();

      sockaddr_t m_sockaddr;

      enum { keepalive = 0x04 };

      long m_options;

      threadlib::Event m_connect;

      threadlib::ThreadControl m_threadcontrol;

      long ClientSocketThread();
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

      //-------------------------- socketpump_error -------------------------
      //---------------------------------------------------------------------
      class socketpump_error : public std::runtime_error
      {
        public:
          socketpump_error(std::string const &arg)
            : runtime_error(arg)
          {
          }
      };

    public:
      SocketPump();
      explicit SocketPump(unsigned int port);
      ~SocketPump();

      bool create(unsigned int port);

      void destroy();

      bool wait_for_connection(int timeout = -1);

      bool accept_connection(socket_t *socket, sockaddr_t *addr = nullptr);

    public:

      threadlib::Waitable &activity() { return m_activity; }

    private:

      unsigned int m_port;

      sockaddr_t m_sockaddr;
      SOCKET m_listeningsocket;

      threadlib::Semaphore m_activity;

      threadlib::ThreadControl m_threadcontrol;

      long SocketPumpThread();
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

      static constexpr size_t kSocketBufferSize = 8192;

    public:
      BroadcastSocket();
      explicit BroadcastSocket(int port, const char *options = "");
      explicit BroadcastSocket(unsigned int ip, int port, const char *options = "");
      ~BroadcastSocket();

      int port() const { return ntohs(m_sockaddr.sin_port); }

      bool wait_on_connect(int timeout = -1);

      bool packet_available();

      bool wait_on_packet(int timeout = -1);

      void broadcast(const void *buffer, size_t bytes, unsigned int ip, int port);

      size_t receive(void *buffer, size_t n, sockaddr_t *addr = NULL);

    public:

      bool create(int port, const char *options = "");
      bool create(unsigned int ip, int port, const char *options = "");

      void destroy();

    protected:

      bool create_and_bind();

      void connected_loop(threadlib::Waitable &cancel);

    private:

      struct packet_t
      {
        sockaddr_t addr;

        size_t bytes;

        uint8_t data[];
      };

      size_t m_bufferhead;
      size_t m_buffertail;
      std::atomic<size_t> m_buffercount;
      std::array<uint8_t, kSocketBufferSize> m_buffer;

    private:

      void parse_options(const char *options);

      sockaddr_t m_sockaddr;

      long m_options;

      threadlib::ThreadControl m_threadcontrol;

      long BroadcastSocketThread();
  };


  //-------------------------- Functions -------------------------------------
  //--------------------------------------------------------------------------

  bool readline(StreamSocket &socket, char *buffer, int n, int timeout = 0);

  std::vector<interface_t> interfaces();

} } // namespace socklib

#endif // SOCKLIB_HH
