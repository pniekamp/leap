//
// Sockets
//
//   Peter Niekamp, September 2000
//

//
// Copyright (C) Peter Niekamp
//
// This code remains the property of the copyright holder.
// The code contained herein may be used for any purpose provided
// this copyright notice is retained
//

#include "leap/sockets.h"
#include "leap/util.h"
#include <cstring>
#include <cassert>

using namespace std;
using namespace leap;
using namespace leap::socklib;
using namespace leap::threadlib;

#ifdef _WIN32
#  include <ws2tcpip.h>
#  define ssize int
#  define socklen_t int
#  define MSG_NOSIGNAL 0
#  undef EMSGSIZE
#  define EMSGSIZE WSAEMSGSIZE
#  undef EWOULDBLOCK
#  define EWOULDBLOCK WSAEWOULDBLOCK
#  undef EALREADY
#  define EALREADY WSAEALREADY
#  undef EINPROGRESS
#  define EINPROGRESS WSAEINPROGRESS
#  undef EISCONN
#  define EISCONN WSAEISCONN
#  undef ECONNRESET
#  define ECONNRESET WSAECONNRESET
#else
#  include <errno.h>
#  include <netdb.h>
#  include <arpa/inet.h>
#  include <fcntl.h>
#  include <signal.h>
#  include <sys/ioctl.h>
#  include <net/if.h>
#  define INVALID_SOCKET -1
#  define SOCKET_ERROR -1
#  define HOSTENT hostent
#  define IN_ADDR in_addr
#  define closesocket(s) ::close(s)
#  define GetLastError() errno
#endif

namespace leap { namespace socklib
{

  //|--- Socket Subsystem Functions ---
  //|

  //|///////////////////// InitialiseSocketSubsystem ////////////////////////
  ///
  /// Initialises the Socket Library for the process. Needs to be called once
  /// for an Application
  ///
  bool InitialiseSocketSubsystem()
  {
  #ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2,0), &wsaData) != 0)
      return false;
  #endif

    return true;
  }


  //|///////////////////// CloseSocketSubsystem /////////////////////////////
  ///
  /// Shuts down the Socket Library for the process.
  ///
  bool CloseSocketSubsystem()
  {
  #ifdef _WIN32
    WSACleanup();
  #endif

    return true;
  }


  //|///////////////////// ring_push ////////////////////////////////////////
  template<class Ring>
  static size_t ring_push(Ring *ring, size_t *tail, void *data, size_t n)
  {
    if (data != nullptr)
    {
      size_t tailspace = ring->size() - *tail;

      if (tailspace != 0)
        memcpy(ring->data() + *tail, data, min(n, tailspace));

      if (n > tailspace)
        memcpy(ring->data(), (uint8_t*)data + tailspace, n - tailspace);
    }

    *tail = (*tail + n) % ring->size();

    return n;
  }


  //|///////////////////// ring_pop /////////////////////////////////////////
  template<class Ring>
  static size_t ring_pop(Ring *ring, size_t *head, void *data, size_t n)
  {
    if (data != nullptr)
    {
      size_t tailspace = ring->size() - *head;

      if (tailspace != 0)
        memcpy(data, ring->data() + *head, min(n, tailspace));

      if (n > tailspace)
        memcpy((uint8_t*)data + tailspace, ring->data(), n - tailspace);
    }

    *head = (*head + n) % ring->size();

    return n;
  }


  //|///////////////////// select ///////////////////////////////////////////
  static bool select(int timeout, SOCKET readfd, SOCKET writefd)
  {
    fd_set readfds;
    FD_ZERO(&readfds);
    if (readfd != INVALID_SOCKET)
      FD_SET(readfd, &readfds);

    fd_set writefds;
    FD_ZERO(&writefds);
    if (writefd != INVALID_SOCKET)
      FD_SET(writefd, &writefds);

    timeval t;
    t.tv_sec = timeout / 1000;
    t.tv_usec = (timeout % 1000) * 1000;

    return (::select(max(readfd, writefd)+1, &readfds, &writefds, NULL, &t) != 0);
  }



  //|--------------------- SocketBase ---------------------------------------
  //|------------------------------------------------------------------------


  //|///////////////////// SocketBase::Constructor //////////////////////////
  SocketBase::SocketBase()
    : m_activity(1)
  {
    m_status = SocketStatus::Unborn;

    m_errorcondition = 0;

    m_connectedsocket = INVALID_SOCKET;
  }


  //|///////////////////// SocketBase::connected ////////////////////////////
  ///
  /// Returns true if the socket is connected (or if there is data waiting)
  ///
  bool SocketBase::connected() const
  {
    return (m_status == SocketStatus::Connected && m_connectedsocket != INVALID_SOCKET);
  }


  //|///////////////////// SocketBase::status ///////////////////////////////
  ///
  /// Returns the status of the socket connection
  ///
  SocketBase::SocketStatus SocketBase::status() const
  {
    return m_status;
  }


  //|///////////////////// SocketBase::statustxt ////////////////////////////
  ///
  /// Returns the status of the socket connection
  ///
  string SocketBase::statustxt() const
  {
    string result;

    switch (m_status)
    {
      case SocketStatus::Unborn:
        result = "Unborn";
        break;

      case SocketStatus::Created:
        result = "Created";
        break;

      case SocketStatus::Connected:
        result = "Connected";
        break;

      case SocketStatus::Cactus:
        result = "Cactus";
        break;

      case SocketStatus::Dead:
        result = "Dead";
        break;
    }

    if (m_errorcondition != 0)
    {
  #ifdef _WIN32
      char *lpMsgBuf;

      FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, m_errorcondition, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL);

      for(char *ch = lpMsgBuf; *ch != 0; ++ch)
        *ch = max(*ch, ' ');

      result += string(" (") + lpMsgBuf + string(")");

      LocalFree(lpMsgBuf);
  #else
      result += string(" (") + toa(m_errorcondition) + string(")");
  #endif
    }

    return result;
  }


  //|///////////////////// SocketBase::wait_on_activity /////////////////////
  ///
  /// Waits for Activity on the Socket (Receive data, Socket Status Change)
  ///
  /// \param[in] timeout Length of time to wait until activity
  ///
  /// \return true if successful wait, false if timeout
  ///
  bool SocketBase::wait_on_activity(int timeout)
  {
    return m_activity.wait(timeout);
  }


  //|///////////////////// SocketBase::destroy //////////////////////////////
  ///
  /// Destroy the socket
  ///
  void SocketBase::destroy()
  {
    // Mark the socket as dead
    m_status = SocketStatus::Dead;

    // Clean up the connected socket handle
    if (m_connectedsocket != INVALID_SOCKET)
      closesocket(m_connectedsocket);

    m_connectedsocket = INVALID_SOCKET;

    m_activity.release();
  }



  //|--------------------- StreamSocket -------------------------------------
  //|------------------------------------------------------------------------

  constexpr size_t StreamSocket::kSocketBufferSize;

  //|///////////////////// StreamSocket::Constructor ////////////////////////
  StreamSocket::StreamSocket()
  {
    m_bufferhead = 0;
    m_buffertail = 0;
    m_buffercount = 0;
  }


  //|///////////////////// StreamSocket::bytes_available ////////////////////
  ///
  /// \return number of bytes ready for reading
  ///
  size_t StreamSocket::bytes_available() const
  {
    if (!connected())
      return 0;

    return m_buffercount;
  }


  //|///////////////////// StreamSocket::wait_on_bytes //////////////////////
  ///
  /// Waits for at least minbytes to become available on the socket
  ///
  /// \param[in] minbytes Number of bytes to receive before returning
  /// \param[in] timeout Length of time to wait until activity
  ///
  /// \return true if successful wait, false if timeout
  ///
  bool StreamSocket::wait_on_bytes(size_t minbytes, int timeout)
  {
    while (true)
    {
      if (m_buffercount >= minbytes)
        return true;

      if (!connected())
        return false;

      if (!m_activity.wait(timeout))
        return false;
    }
  }


  //|///////////////////// StreamSocket::discard ////////////////////////////
  ///
  /// Discards a number of bytes from the receive buffer (gone!)
  ///
  /// \param[in] bytes number of bytes to throw away
  ///
  void StreamSocket::discard(size_t bytes)
  {
    size_t buffercount = m_buffercount.load(std::memory_order_relaxed);

    bytes = min(bytes, buffercount);

    // discard bytes
    ring_pop(&m_buffer, &m_bufferhead, nullptr, bytes);

    // update count (compare-and-swap)
    while (!m_buffercount.compare_exchange_weak(buffercount, buffercount - bytes))
      ;
  }


  //|///////////////////// StreamSocket::close //////////////////////////////
  ///
  /// Closes socket instance
  ///
  void StreamSocket::close()
  {
    if (m_connectedsocket != INVALID_SOCKET)
      closesocket(m_connectedsocket);

    m_connectedsocket = INVALID_SOCKET;
  }


  //|///////////////////// StreamSocket::receive ////////////////////////////
  ///
  /// Retreives data out of the receive buffer, and hence off the socket
  ///
  /// \param[out] buffer buffer for received data
  /// \param[in] n number of bytes in the buffer (or to receive)
  ///
  /// \return number of bytes received
  ///
  size_t StreamSocket::receive(void *buffer, size_t n)
  {
    // Ensure we are created
    if (m_status == SocketStatus::Unborn)
      throw socket_error("Socket Not Created");

    // Ensure we are connected
    if (!connected())
      throw socket_error("Socket Not Connected");

    size_t buffercount = m_buffercount.load(std::memory_order_relaxed);

    size_t bytes = min(n, buffercount);

    // transfer bytes
    ring_pop(&m_buffer, &m_bufferhead, buffer, bytes);

    // update count (compare-and-swap)
    while (!m_buffercount.compare_exchange_weak(buffercount, buffercount - bytes))
      ;

    return bytes;
  }


  //|///////////////////// StreamSocket::transmit ///////////////////////////
  ///
  /// Transmits data out the socket
  ///
  /// \param[in] buffer buffer containing data to transmit
  /// \param[in] bytes number of bytes to transmit
  //|
  void StreamSocket::transmit(const void *buffer, size_t bytes)
  {
    // Ensure we are created
    if (m_status == SocketStatus::Unborn)
      throw socket_error("Socket Not Created");

    // Ensure we are connected
    if (!connected())
      throw socket_error("Socket Not Connected");

    //
    // Send the data
    //

    ssize_t result = send(m_connectedsocket, (const char*)buffer, bytes, MSG_NOSIGNAL);

    while (result > 0 && (size_t)result < bytes)
    {
      bytes -= result;
      buffer = (const char *)(buffer) + result;

      select(10000, -1, m_connectedsocket);

      result = send(m_connectedsocket, (const char*)buffer, bytes, MSG_NOSIGNAL);
    }

    if (result == SOCKET_ERROR)
    {
      m_errorcondition = GetLastError();

      throw socket_error("Socket Transmit Error (" + toa(m_errorcondition) + ")");
    }
  }


  //|///////////////////// StreamSocket::connected_loop /////////////////////
  //
  // Loop until socket is disconnected. For use by ConnectionThreads
  // Moves data from the connected socket into the receive buffer.
  //
  void StreamSocket::connected_loop(Waitable &cancel)
  {
    // Assume we're now connected...
    m_bufferhead = 0;
    m_buffertail = 0;
    m_buffercount = 0;
    m_errorcondition = 0;
    m_status = SocketStatus::Connected;

    m_activity.release();

    //
    // While thread is running
    //

    while (!cancel)
    {
      if (m_connectedsocket == INVALID_SOCKET)
        break;

      if (!select(500, m_connectedsocket, -1))
        continue;

      size_t buffercount = m_buffercount.load(std::memory_order_relaxed);

      // No Room in buffer... wait a while
      if (buffercount == m_buffer.size())
      {
        sleep_for(32);
        continue;
      }

      void *buffer = m_buffer.data() + m_buffertail;

      size_t bufbytes = min(m_buffer.size() - buffercount, m_buffer.size() - m_buffertail);

      //
      // Receive any data that is available
      //

      ssize_t bytes = recv(m_connectedsocket, (char*)buffer, bufbytes, 0);

      // check if we are no longer connected...
      if (bytes == 0)
      {
        m_errorcondition = 0;
        break;
      }

      // or an error occured...
      else if (bytes == SOCKET_ERROR)
      {
        auto result = GetLastError();

        // was it a fatal error that results in a disconnect ?
        if (result != EMSGSIZE && result != EWOULDBLOCK)
        {
          m_errorcondition = result;
          break;
        }
      }

      else
      {
        //
        // Got some data.. update the read buffer
        //

        ring_push(&m_buffer, &m_buffertail, nullptr, bytes);

        // update count (compare-and-swap)
        while (!m_buffercount.compare_exchange_weak(buffercount, buffercount + bytes))
          ;

        m_activity.release();
      }
    }

    // Disconnect now...
    m_status = SocketStatus::Cactus;

    m_activity.release();
  }


  //|///////////////////// StreamSocket::destroy ////////////////////////////
  ///
  /// Destroy the socket
  ///
  void StreamSocket::destroy()
  {
    SocketBase::destroy();
  }




  //|--------------------- ServerSocket -------------------------------------
  //|------------------------------------------------------------------------


  //|///////////////////// ServerSocket::Constructor ////////////////////////
  ServerSocket::ServerSocket()
  {
    m_options = 0;
    m_listeningsocket = INVALID_SOCKET;
  }


  //|///////////////////// ServerSocket::Constructor ////////////////////////
  ServerSocket::ServerSocket(unsigned int port, const char *options)
    : ServerSocket()
  {
    create(port, options);
  }


  //|///////////////////// ServerSocket::Constructor ////////////////////////
  ServerSocket::ServerSocket(socket_t socket, const char *options)
    : ServerSocket()
  {
    attach(socket, options);
  }


  //|///////////////////// ServerSocket::Destructor /////////////////////////
  ServerSocket::~ServerSocket()
  {
    destroy();
  }


  //|///////////////////// ServerSocket::parse_options //////////////////////
  ///
  /// Parse the options string
  ///
  /// \param[in] options string containing options (reconnect, keepalive)
  ///
  void ServerSocket::parse_options(const char *options)
  {
    for(int i = 0; options[i] != 0; ++i)
    {
      if (options[i] <= ' ')
        continue;

      if (strncmp(&options[i], "keepalive", 9) == 0)
        m_options |= KeepAlive;
    }
  }


  //|///////////////////// ServerSocket::create /////////////////////////////
  ///
  /// Creates a Server Socket.
  ///
  /// \param[in] port The port number on which to create the socket
  /// \param[in] options Options string
  //|
  bool ServerSocket::create(unsigned int port, const char *options)
  {
    assert(m_status == SocketStatus::Unborn || m_status == SocketStatus::Dead);

    // Parse out the options

    m_options = Connectable;

    parse_options(options);

    //
    // Initialise and Create
    //

    memset(&m_sockaddr, 0, sizeof(m_sockaddr));

    m_sockaddr.sin_family = AF_INET;
    m_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    m_sockaddr.sin_port = htons((u_short)port);

    m_status = SocketStatus::Unborn;

    m_threadcontrol.create_thread(this, &ServerSocket::ServerSocketThread);

    return true;
  }


  //|///////////////////// ServerSocket::attach /////////////////////////////
  ///
  /// Attach to an already created socket
  ///
  /// \param[in] socket The socket to attach to
  /// \param[in] options Options string
  ///
  bool ServerSocket::attach(socket_t socket, const char *options)
  {
    assert(m_status == SocketStatus::Unborn || m_status == SocketStatus::Dead);

    // Parse the options string

    m_options = 0;

    parse_options(options);

    //
    // Initialise and Create
    //

    memset(&m_sockaddr, 0, sizeof(m_sockaddr));

    m_connectedsocket = socket.sid;

    m_status = SocketStatus::Unborn;

    m_threadcontrol.create_thread(this, &ServerSocket::ServerSocketThread);

    // Wait until socket attachs fully
    while (status() == SocketStatus::Unborn)
      wait_on_activity(100);

    return true;
  }


  //|///////////////////// ServerSocket::destroy ////////////////////////////
  void ServerSocket::destroy()
  {
    m_threadcontrol.join_threads();

    StreamSocket::destroy();
  }


  //|///////////////////// ServerSocket::create_and_bind ////////////////////
  bool ServerSocket::create_and_bind()
  {
    //
    // Create a socket that listens for connections
    //

    m_listeningsocket = ::socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_listeningsocket == INVALID_SOCKET)
    {
      m_errorcondition = GetLastError();
      return false;
    }

    //
    // Bind the listening socket to the port number
    //

    if (::bind(m_listeningsocket, (sockaddr*)&(m_sockaddr), sizeof(m_sockaddr)) != 0)
    {
      m_errorcondition = GetLastError();
      return false;
    }

    // Set socket to non-blocking mode
  #ifdef _WIN32
    u_long nonblocking = true;
    ioctlsocket(m_listeningsocket, FIONBIO, &nonblocking);
  #else
    fcntl(m_listeningsocket, F_SETFL, fcntl(m_listeningsocket, F_GETFL) | O_NONBLOCK);
  #endif

    return true;
  }


  //|///////////////////// ServerSocket::listen_and_accept //////////////////
  bool ServerSocket::listen_and_accept()
  {
    //
    // Set the socket to listen for incomming connections
    //

    if (::listen(m_listeningsocket, 1) == SOCKET_ERROR)
    {
      m_errorcondition = GetLastError();
      return false;
    }

    // Accept an incomming connection
    m_connectedsocket = accept(m_listeningsocket, NULL, NULL);

    if ((m_options & KeepAlive) != 0)
    {
      int one = 1;
      setsockopt(m_connectedsocket, SOL_SOCKET, SO_KEEPALIVE, (const char *)&one, sizeof(one));
    }

    return true;
  }


  //|///////////////////// ServerSocket::close_and_invalidate ///////////////
  void ServerSocket::close_and_invalidate()
  {
    if (m_connectedsocket != INVALID_SOCKET)
      closesocket(m_connectedsocket);

    m_connectedsocket = INVALID_SOCKET;
  }


  //|///////////////////// ServerSocket::close_listener /////////////////////
  void ServerSocket::close_listener()
  {
    if (m_listeningsocket != INVALID_SOCKET)
      closesocket(m_listeningsocket);

    m_listeningsocket = INVALID_SOCKET;
  }


  //|///////////////////// ServerSocket::ServerSocketThread /////////////////
  //
  // Server Thread... Connects and reconnects server socket
  //
  long ServerSocket::ServerSocketThread()
  {
    if (m_options & Connectable)
    {
      if (!create_and_bind())
      {
        m_status = SocketStatus::Dead;

        return 0;
      }
    }

    m_status = SocketStatus::Created;

    while (true)
    {
      if (m_options & Connectable)
      {
        listen_and_accept();
      }

      if (m_connectedsocket != INVALID_SOCKET)
      {
        //
        // Socket Connected
        //

        connected_loop(m_threadcontrol.terminate());

        close_and_invalidate();

        if (!(m_options & Connectable))
          break;
      }

      sleep_til(m_threadcontrol.terminate(), 1000);

      if (m_threadcontrol.terminating())
        break;
    }

    close_listener();

    m_status = SocketStatus::Dead;

    return 0;
  }




  //|--------------------- ClientSocket -------------------------------------
  //|------------------------------------------------------------------------


  //|///////////////////// ClientSocket::Constructor ////////////////////////
  ClientSocket::ClientSocket()
  {
    m_options = 0;
  }


  //|///////////////////// ClientSocket::Constructor ////////////////////////
  ClientSocket::ClientSocket(const char *ipaddress, unsigned int port, const char *options)
    : ClientSocket()
  {
    create(ipaddress, port, options);
  }


  //|///////////////////// ClientSocket::Destructor /////////////////////////
  ClientSocket::~ClientSocket()
  {
    destroy();
  }


  //|///////////////////// ClientSocket::parse_options //////////////////////
  ///
  /// Parse the options string
  ///
  /// \param[in] options String containing options (reconnect, keepalive)
  ///
  void ClientSocket::parse_options(const char *options)
  {
    for(int i = 0; options[i] != 0; ++i)
    {
      if (options[i] <= ' ')
        continue;

      if (strncmp(&options[i], "keepalive", 9) == 0)
        m_options |= keepalive;
    }
  }


  //|///////////////////// ClientSocket::create /////////////////////////////
  ///
  /// Creates a Client Socket.
  ///
  /// \param[in] ipaddress The address of the remote machine
  /// \param[in] port The port number on which to connect
  /// \param[in] options Options string
  ///
  bool ClientSocket::create(const char *ipaddress, unsigned int port, const char *options)
  {
    assert(m_status == SocketStatus::Unborn || m_status == SocketStatus::Dead);

    // Parse the options string

    m_options = 0;

    parse_options(options);

    //
    // Initialise and Create
    //

    memset(&m_sockaddr, 0, sizeof(m_sockaddr));

    m_sockaddr.sin_family = AF_INET;
    m_sockaddr.sin_addr.s_addr = 0;
    m_sockaddr.sin_port = htons((u_short)port);

    // Breakdown the ip address

    if (ipaddress != NULL)
    {
      m_sockaddr.sin_addr.s_addr = inet_addr(ipaddress);
      if (m_sockaddr.sin_addr.s_addr == INADDR_NONE)
      {
        //
        // Convert a name to an ip address
        //

        HOSTENT *lphost = gethostbyname(ipaddress);
        if (lphost != NULL)
          m_sockaddr.sin_addr.s_addr = ((IN_ADDR*)lphost->h_addr)->s_addr;
      }
    }

    m_status = SocketStatus::Unborn;

    m_threadcontrol.create_thread(this, &ClientSocket::ClientSocketThread);

    return true;
  }


  //|///////////////////// ClientSocket::wait_on_connect ////////////////////
  ///
  /// Waits for connection
  ///
  /// \param[in] timeout Length of time to wait until connection
  ///
  /// \return true if successful wait, false if timeout
  ///
  bool ClientSocket::wait_on_connect(int timeout)
  {
    while (true)
    {
      if (connect())
        return true;

      if (!m_activity.wait(timeout))
        return false;
    }
  }


  //|///////////////////// ClientSocket::connect ////////////////////////////
  ///
  /// Connects a Client Socket.
  ///
  bool ClientSocket::connect()
  {
    m_connect.set();

    return connected();
  }


  //|///////////////////// ClientSocket::destroy ////////////////////////////
  void ClientSocket::destroy()
  {
    m_threadcontrol.join_threads();

    StreamSocket::destroy();
  }


  //|///////////////////// ClientSocket::create_socket //////////////////////
  bool ClientSocket::create_socket()
  {
    //
    // Create the Client Socket
    //

    m_connectedsocket = ::socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_connectedsocket == INVALID_SOCKET)
    {
      m_errorcondition = GetLastError();
      return false;
    }

    // keepalive option
    if ((m_options & keepalive) != 0)
    {
      int one = 1;
      setsockopt(m_connectedsocket, SOL_SOCKET, SO_KEEPALIVE, (const char *)&one, sizeof(one));
    }

    // Set socket to non-blocking mode
  #ifdef _WIN32
    u_long nonblocking = true;
    ioctlsocket(m_connectedsocket, FIONBIO, &nonblocking);
  #else
    fcntl(m_connectedsocket, F_SETFL, fcntl(m_connectedsocket, F_GETFL) | O_NONBLOCK);
  #endif

    return true;
  }


  //|///////////////////// ClientSocket::connect_socket /////////////////////
  bool ClientSocket::connect_socket()
  {
    //
    // Connect the Client Socket
    //

    if (::connect(m_connectedsocket, (sockaddr*)&(m_sockaddr), sizeof(m_sockaddr)) == 0)
      return true;

    auto result = GetLastError();

    if (result != EINPROGRESS && result != EWOULDBLOCK && result != EALREADY && result != EISCONN)
    {
      m_errorcondition = result;

      close_socket();

      return false;
    }

    if (!select(500, -1, m_connectedsocket))
      return false;

    socklen_t resultlen = sizeof(result);
    getsockopt(m_connectedsocket, SOL_SOCKET, SO_ERROR, (char*)&result, &resultlen);

    if (result != 0)
    {
      m_errorcondition = result;

      close_socket();

      return false;
    }

    return true;
  }


  //|///////////////////// ClientSocket::close_socket ///////////////////////
  void ClientSocket::close_socket()
  {
    if (m_connectedsocket != INVALID_SOCKET)
      closesocket(m_connectedsocket);

    m_connectedsocket = INVALID_SOCKET;
  }


  //|///////////////////// ClientSocket::ClientSocketThread /////////////////
  //
  // Client Socket Connection Thread
  //
  long ClientSocket::ClientSocketThread()
  {
    WaitGroup events;
    events.add(m_connect);
    events.add(m_threadcontrol.terminate());

    m_status = SocketStatus::Created;

    while (true)
    {
      if (m_connect)
      {
        if (m_connectedsocket == INVALID_SOCKET)
        {
          if (!create_socket())
          {
            m_status = SocketStatus::Dead;

            return 0;
          }
        }

        if (connect_socket())
        {
          //
          // Socket Connected
          //

          connected_loop(m_threadcontrol.terminate());

          close_socket();

          m_connect.reset();

          m_activity.release();
        }

        sleep_til(m_threadcontrol.terminate(), 500);
      }

      sleep_any(events);

      if (m_threadcontrol.terminating())
        break;
    }

    m_status = SocketStatus::Dead;

    return 0;
  }




  //|--------------------- SocketPump ---------------------------------------
  //|------------------------------------------------------------------------


  //|///////////////////// SocketPump::Constructor //////////////////////////
  SocketPump::SocketPump()
    : m_activity(1)
  {
    m_port = 0;
    m_listeningsocket = INVALID_SOCKET;
  }


  //|///////////////////// SocketPump::Constructor //////////////////////////
  SocketPump::SocketPump(unsigned int port)
    : SocketPump()
  {
    create(port);
  }


  //|///////////////////// SocketPump::Destructor ///////////////////////////
  SocketPump::~SocketPump()
  {
    destroy();
  }


  //|///////////////////// SocketPump::create ///////////////////////////////
  ///
  /// Create the socket pump (the listening socket)
  ///
  /// \param[in] port The port number on which to listen for connections
  ///
  bool SocketPump::create(unsigned int port)
  {
    assert(m_listeningsocket == INVALID_SOCKET);

    m_port = port;

    //
    // Initialise the SockAddr Structure
    //

    memset(&m_sockaddr, 0, sizeof(m_sockaddr));

    m_sockaddr.sin_family = AF_INET;
    m_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    m_sockaddr.sin_port = htons((u_short)m_port);

    //
    // Create a socket that listens for connections
    //

    m_listeningsocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_listeningsocket == INVALID_SOCKET)
    {
      throw socketpump_error("Error Creating Listening Socket (" + toa(GetLastError()) + ")");
    }

    int one = 1;
    setsockopt(m_listeningsocket, SOL_SOCKET, SO_REUSEADDR, (const char *)&one, sizeof(one));

    //
    // Bind the listening socket to the port number
    //

    if (::bind(m_listeningsocket, (sockaddr*)&(m_sockaddr), sizeof(m_sockaddr)) != 0)
    {
      destroy();

      throw socketpump_error("Error Binding Listening Socket (" + toa(GetLastError()) + ")");
    }

    // Set socket to non-blocking mode
  #ifdef _WIN32
    u_long nonblocking = true;
    ioctlsocket(m_listeningsocket, FIONBIO, &nonblocking);
  #else
    fcntl(m_listeningsocket, F_SETFL, fcntl(m_listeningsocket, F_GETFL) | O_NONBLOCK);
  #endif

    if (listen(m_listeningsocket, SOMAXCONN) == SOCKET_ERROR)
    {
      destroy();

      throw socketpump_error("Error Listening on Socket (" + toa(GetLastError()) + ")");
    }

    m_threadcontrol.create_thread(this, &SocketPump::SocketPumpThread);

    return true;
  }


  //|///////////////////// SocketPump::destroy //////////////////////////////
  ///
  /// Destroys the socket pump (the listening socket)
  ///
  void SocketPump::destroy()
  {
    m_threadcontrol.join_threads();

    if (m_listeningsocket != INVALID_SOCKET)
      closesocket(m_listeningsocket);

    m_listeningsocket = INVALID_SOCKET;
  }


  //|///////////////////// SocketPump::wait_for_connection //////////////////
  bool SocketPump::wait_for_connection(int timeout)
  {
    return m_activity.wait(timeout);
  }


  //|///////////////////// SocketPump::accept_connection ////////////////////
  ///
  /// Check the listening socket for a client connection (and return it)
  ///
  /// \param[out] socket The socket that has connected
  /// \param[out] addr The SockAddr structure of the connected socket (may be NULL)
  ///
  /// if you supply addr, use inet_ntoa(addr.sin_addr) to convert to an IP address
  ///
  /// \return Will return true if a socket connects, otherwise false
  ///
  bool SocketPump::accept_connection(socket_t *socket, sockaddr_in *addr)
  {
    socklen_t addrlen = sizeof(*addr);

    //
    // Accept an incomming connection and return it
    //

    socket->sid = accept(m_listeningsocket, (sockaddr*)addr, &addrlen);

    if (socket->sid != INVALID_SOCKET)
      return true;

    return false;
  }


  //|///////////////////// ClientSocket::SocketPumpThread ///////////////////
  //
  // Socket Pump Thread
  //
  long SocketPump::SocketPumpThread()
  {
    while (!m_threadcontrol.terminating())
    {
      if (select(500, m_listeningsocket, -1))
      {
        m_activity.release();

        sleep_for(250);
      }
    }

    return 0;
  }



  //|--------------------- BroadcastSocket ----------------------------------
  //|------------------------------------------------------------------------

  constexpr size_t BroadcastSocket::kSocketBufferSize;

  //|///////////////////// BroadcastSocket::Constructor /////////////////////
  BroadcastSocket::BroadcastSocket()
  {
    m_options = 0;
    m_bufferhead = 0;
    m_buffertail = 0;
    m_buffercount = 0;
  }


  //|///////////////////// BroadcastSocket::Constructor /////////////////////
  BroadcastSocket::BroadcastSocket(int port, const char *options)
    : BroadcastSocket()
  {
    create(port, options);
  }


  //|///////////////////// BroadcastSocket::Constructor /////////////////////
  BroadcastSocket::BroadcastSocket(unsigned int ip, int port, const char *options)
    : BroadcastSocket()
  {
    create(ip, port, options);
  }


  //|///////////////////// BroadcastSocket::Destructor //////////////////////
  BroadcastSocket::~BroadcastSocket()
  {
    destroy();
  }


  //|///////////////////// BroadcastSocket::parse_options ///////////////////
  ///
  /// Parse the options string
  ///
  /// \param[in] options string containing options
  ///
  void BroadcastSocket::parse_options(const char */*options*/)
  {
  }


  //|///////////////////// BroadcastSocket::create //////////////////////////
  ///
  /// Creates a Broadcast Sink.
  ///
  /// \param[in] port The port number on which to create the socket
  /// \param[in] options Options string
  //|
  bool BroadcastSocket::create(int port, const char *options)
  {
    return create(htonl(INADDR_ANY), port, options);
  }


  //|///////////////////// BroadcastSocket::create //////////////////////////
  ///
  /// Creates a Broadcast Sink.
  ///
  /// \param[in] ip Interface Bind Address
  /// \param[in] port The port number on which to create the socket
  /// \param[in] options Options string
  //|
  bool BroadcastSocket::create(unsigned int ip, int port, const char *options)
  {
    assert(m_status == SocketStatus::Unborn || m_status == SocketStatus::Dead);

    // Parse out the options

    m_options = 0;

    parse_options(options);

    //
    // Initialise and Create
    //

    memset(&m_sockaddr, 0, sizeof(m_sockaddr));

    m_sockaddr.sin_family = AF_INET;
    m_sockaddr.sin_addr.s_addr = ip;
    m_sockaddr.sin_port = htons((u_short)port);

    m_threadcontrol.create_thread(this, &BroadcastSocket::BroadcastSocketThread);

    return true;
  }


  //|///////////////////// BroadcastSocket::destroy /////////////////////////
  void BroadcastSocket::destroy()
  {
    m_threadcontrol.join_threads();

    SocketBase::destroy();
  }


  //|///////////////////// BroadcastSocket::wait_on_connect /////////////////
  ///
  /// Waits for connection
  ///
  /// \param[in] timeout Length of time to wait until connection
  ///
  /// \return true if successful wait, false if timeout
  ///
  bool BroadcastSocket::wait_on_connect(int timeout)
  {
    while (true)
    {
      if (connected())
        return true;

      if (!m_activity.wait(timeout))
        return false;
    }
  }


  //|///////////////////// BroadcastSocket::packet_available ////////////////
  ///
  /// \return true if a packet is available
  ///
  bool BroadcastSocket::packet_available()
  {
    return (connected() && m_buffercount > 0);
  }


  //|///////////////////// BroadcastSocket::wait_on_packet //////////////////
  ///
  /// Waits for at least minpackets to become available on the socket
  ///
  /// \param[in] timeout Length of time to wait until activity
  ///
  /// \return true if successful wait, false if timeout
  ///
  bool BroadcastSocket::wait_on_packet(int timeout)
  {
    while (true)
    {
      // Check if we have enough bytes to satisfy the caller
      if (m_buffercount > 0)
        return true;

      // If the socket has been disconnected, exit via timeout
      if (!connected())
        return false;

      // Wait for some more activity
      if (!m_activity.wait(timeout))
        return false;
    }
  }


  //|///////////////////// BroadcastSocket::broadcast ///////////////////////
  //
  // send a broadcast
  //
  void BroadcastSocket::broadcast(const void *buffer, size_t bytes, unsigned int ip, int port)
  {
    sockaddr_in dest;

    memset(&dest, 0, sizeof(dest));

    dest.sin_family = AF_INET;
    dest.sin_addr.s_addr = ip;
    dest.sin_port = htons((u_short)port);

    //
    // Send the data
    //

    ssize_t result = sendto(m_connectedsocket, (const char*)buffer, bytes, MSG_NOSIGNAL, (sockaddr*)&dest, sizeof(dest));

    if (result == SOCKET_ERROR)
    {
      m_errorcondition = GetLastError();

      throw socket_error("Socket Broadcast Error (" + toa(m_errorcondition) + ")");
    }
  }


  //|///////////////////// BroadcastSocket::receive /////////////////////////
  //
  // receive a packet
  //
  size_t BroadcastSocket::receive(void *buffer, size_t n, sockaddr_in *addr)
  {
    // Ensure we are created
    if (m_status == SocketStatus::Unborn)
      throw socket_error("Socket Not Created");

    // Ensure we are connected
    if (!connected())
      throw socket_error("Socket Not Connected");

    size_t buffercount = m_buffercount.load(std::memory_order_relaxed);

    // receive packet
    size_t bytes = 0;

    if (buffercount > 0)
    {
      ring_pop(&m_buffer, &m_bufferhead, addr, sizeof(*addr));
      ring_pop(&m_buffer, &m_bufferhead, &bytes, sizeof(bytes));
      ring_pop(&m_buffer, &m_bufferhead, buffer, min(n, bytes));

      if (n < bytes)
        ring_pop(&m_buffer, &m_bufferhead, nullptr, bytes - n);

      // update count (compare-and-swap)
      while (!m_buffercount.compare_exchange_weak(buffercount, buffercount - bytes - sizeof(packet_t)))
        ;
    }

    return bytes;
  }


  //|///////////////////// BroadcastSocket::connected_loop //////////////////
  //
  // Loop until socket is disconnected. For use by ConnectionThreads
  //
  void BroadcastSocket::connected_loop(Waitable &cancel)
  {
    // Assume we're now connected...
    m_bufferhead = 0;
    m_buffertail = 0;
    m_buffercount = 0;
    m_errorcondition = 0;
    m_status = SocketStatus::Connected;

    m_activity.release();

    sockaddr_in addr;

    uint8_t buffer[4096];

    //
    // While thread is running
    //

    while (!cancel)
    {
      if (!select(500, m_connectedsocket, -1))
        continue;

      size_t buffercount = m_buffercount.load(std::memory_order_relaxed);

      socklen_t addrlen = sizeof(addr);

      ssize_t bytes = recvfrom(m_connectedsocket, (char*)buffer, sizeof(buffer), 0, (sockaddr*)&addr, &addrlen);

      // an error occured...
      if (bytes == SOCKET_ERROR)
      {
        auto result = GetLastError();

        // was it a fatal error that results in a disconnect ?
        if (result != EWOULDBLOCK && result != ECONNRESET)
        {
          m_errorcondition = result;
          break;
        }
      }

      else
      {
        //
        // Got some data.. move to the read buffer
        //

        size_t bufbytes = m_buffer.size() - buffercount;

        if (bytes + sizeof(packet_t) < bufbytes)
        {
          size_t size = bytes;

          ring_push(&m_buffer, &m_buffertail, &addr, sizeof(addr));
          ring_push(&m_buffer, &m_buffertail, &size, sizeof(size));
          ring_push(&m_buffer, &m_buffertail, buffer, size);

          // update count (compare-and-swap)
          while (!m_buffercount.compare_exchange_weak(buffercount, buffercount + bytes + sizeof(packet_t)))
            ;
        }

        m_activity.release();
      }
    }

    // Disconnect now...
    m_status = SocketStatus::Cactus;

    m_activity.release();
  }


  //|///////////////////// BroadcastSocket::create_and_bind /////////////////
  bool BroadcastSocket::create_and_bind()
  {
    //
    // Create a socket that listens for connections
    //

    m_connectedsocket = ::socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (m_connectedsocket == INVALID_SOCKET)
    {
      m_errorcondition = GetLastError();
      return false;
    }

    int one = 1;
    setsockopt(m_connectedsocket, SOL_SOCKET, SO_REUSEADDR, (const char *)&one, sizeof(one));
    setsockopt(m_connectedsocket, SOL_SOCKET, SO_BROADCAST, (const char *)&one, sizeof(one));

    //
    // Bind the socket to the port number
    //

    if (::bind(m_connectedsocket, (sockaddr*)&(m_sockaddr), sizeof(m_sockaddr)) != 0)
    {
      m_errorcondition = GetLastError();
      return false;
    }

    // Set socket to non-blocking mode
  #ifdef _WIN32
    u_long nonblocking = true;
    ioctlsocket(m_connectedsocket, FIONBIO, &nonblocking);
  #else
    fcntl(m_connectedsocket, F_SETFL, fcntl(m_connectedsocket, F_GETFL) | O_NONBLOCK);
  #endif

    return true;
  }


  //|///////////////////// BroadcastSocket::BroadcastSocketThread ///////////
  //
  // Broadcast Sink Thread...
  //
  long BroadcastSocket::BroadcastSocketThread()
  {
    if (!create_and_bind())
    {
      m_status = SocketStatus::Dead;

      return 0;
    }

    //
    // OK, the socket is now Created
    //

    m_status = SocketStatus::Created;

    connected_loop(m_threadcontrol.terminate());

    m_status = SocketStatus::Dead;

    return 0;
  }


  //|--------------------- Functions ----------------------------------------
  //|------------------------------------------------------------------------

  //////////////////////// readline /////////////////////////////////////////
  bool readline(StreamSocket &socket, char *buffer, int n, int timeout)
  {
    if (!socket.connected())
      throw SocketBase::socket_error("Not Connected");

    for(int i = 1; socket.wait_on_bytes(i, timeout) && i < n; ++i)
    {
      if (socket.peek(i-1) == '\n')
      {
        socket.receive(buffer, i);

        buffer[i-1] = 0;

        for(int k = i-2; k >= 0 && (buffer[k] == '\n' || buffer[k] == '\r'); --k)
          buffer[k] = 0;

        return true;
      }
    }

    return false;
  }


  //////////////////////// interfaces ///////////////////////////////////////
  vector<interface_t> interfaces()
  {
    vector<interface_t> result;

    char buffer[1024];

    SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == INVALID_SOCKET)
      return result;

  #ifdef _WIN32
    DWORD bytes = 0;
    INTERFACE_INFO *ifc = (INTERFACE_INFO*)buffer;
    WSAIoctl(sock, SIO_GET_INTERFACE_LIST, NULL, 0, buffer, sizeof(buffer), &bytes, NULL, NULL);

    for(INTERFACE_INFO *ifr = ifc; ifr < ifc + bytes / sizeof(INTERFACE_INFO); ++ifr)
    {
      interface_t iface;

      iface.name = "";
      iface.ip = ifr->iiAddress.AddressIn.sin_addr.s_addr;
      iface.mask = ifr->iiNetmask.AddressIn.sin_addr.s_addr;
      iface.bcast = (iface.ip | ~iface.mask);

      result.push_back(iface);
    }
  #else
    ifconf ifc;
    ifc.ifc_buf = buffer;
    ifc.ifc_len = sizeof(buffer);
    ioctl(sock, SIOCGIFCONF, &ifc);

    for(ifreq *ifr = ifc.ifc_req; ifr < ifc.ifc_req + ifc.ifc_len / sizeof(ifreq); ++ifr)
    {
      interface_t iface;

      iface.name = ifr->ifr_name;
      iface.ip = ((sockaddr_in*)&ifr->ifr_addr)->sin_addr.s_addr;

      ioctl(sock, SIOCGIFNETMASK, ifr);
      iface.mask = ((sockaddr_in*)&ifr->ifr_netmask)->sin_addr.s_addr;

      ioctl(sock, SIOCGIFBRDADDR, ifr);
      iface.bcast = ((sockaddr_in*)&ifr->ifr_broadaddr)->sin_addr.s_addr;

      result.push_back(iface);
    }
  #endif

    closesocket(sock);

    return result;
  }


} } // namespace socklib
