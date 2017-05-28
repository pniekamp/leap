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
#include "leap/concurrentqueue.h"
#include <atomic>
#include <cstring>
#include <mutex>
#include <chrono>
#include <cassert>

using namespace std;
using namespace leap;
using namespace leap::socklib;
using namespace leap::threadlib;

#ifdef _WIN32
#  include <ws2tcpip.h>
#  define ssize_t int
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
#  define poll WSAPoll
#  define bind(s, a, l) ::bind(s, a, l)
#else
#  include <errno.h>
#  include <netdb.h>
#  include <arpa/inet.h>
#  include <fcntl.h>
#  include <signal.h>
#  include <sys/ioctl.h>
#  include <net/if.h>
#  include <unistd.h>
#  include <poll.h>
#  define INVALID_SOCKET -1
#  define SOCKET_ERROR -1
#  define HOSTENT hostent
#  define IN_ADDR in_addr
#  define bind(s, a, l) ::bind(s, a, l)
#  define closesocket(s) ::close(s)
#  define GetLastError() errno
#endif

#ifdef _WIN32
namespace
{
  int socketpair(int domain, int type, int protocol, SOCKET sv[2])
  {
    sockaddr_t addr = {};

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = 0;

    auto listeningsocket = socket(AF_INET, type, IPPROTO_TCP);

    int one = 1;
    setsockopt(listeningsocket, SOL_SOCKET, SO_REUSEADDR, (const char *)&one, sizeof(one));

    bind(listeningsocket, (sockaddr*)&addr, sizeof(addr));

    socklen_t addrlen = sizeof(addr);
    getsockname(listeningsocket, (sockaddr*)&addr, &addrlen);

    listen(listeningsocket, 1);

    sv[0] = socket(AF_INET, type, protocol);

    connect(sv[0], (sockaddr*)&addr, sizeof(addr));

    sv[1] = accept(listeningsocket, 0, 0);

    closesocket(listeningsocket);

    return (sv[0] != INVALID_SOCKET && sv[1] != INVALID_SOCKET);
  }
}
#endif

namespace
{
  //|----------- SocketWaitThread -------------------
  //|------------------------------------------------

  class SocketWaitThread
  {
    public:

      SocketWaitThread();
      ~SocketWaitThread();

      template<typename Func>
      void await(SOCKET handle, short int events, Func &&func, int timeout = -1);

      void signal(SOCKET handle);

    private:

      long await_thread();

    private:

      enum { Exit = 0, Signal = 1 };

      SOCKET m_cnc[2];

      vector<pollfd> m_fds;
      vector<function<void()>> m_funcs;

      vector<SOCKET> m_signals;

      vector<pair<chrono::steady_clock::time_point, SOCKET>> m_timeouts;

      CriticalSection m_mutex;

      ThreadControl m_threadcontrol;
  };

  //|///////////////////// Constructor ////////////////////////////
  SocketWaitThread::SocketWaitThread()
  {
    socketpair(AF_UNIX, SOCK_STREAM, 0, m_cnc);

    m_threadcontrol.create_thread([=]() { return await_thread(); });
  }

  //|///////////////////// Destructor /////////////////////////////
  SocketWaitThread::~SocketWaitThread()
  {
    send(m_cnc[1], "\0", 1, MSG_NOSIGNAL);

    m_threadcontrol.join_threads();
  }

  //|///////////////////// await //////////////////////////////////
  template<typename Func>
  void SocketWaitThread::await(SOCKET handle, short events, Func &&func, int timeout)
  {
    {
      SyncLock M(m_mutex);

      m_fds.push_back({ handle, events, 0 });
      m_funcs.emplace_back(std::forward<Func>(func));

      if (timeout >= 0)
      {
        m_timeouts.emplace_back(chrono::steady_clock::now() + chrono::milliseconds(timeout), handle);

      }
    }
  }

  //|///////////////////// signal /////////////////////////////////
  void SocketWaitThread::signal(SOCKET handle)
  {
    {
      SyncLock M(m_mutex);

      m_signals.push_back(handle);
    }

    send(m_cnc[1], "\1", 1, MSG_NOSIGNAL);
  }

  //|///////////////////// await_thread /////////////////////////////////
  long SocketWaitThread::await_thread()
  {
    vector<SOCKET> sigs;
    vector<pair<chrono::steady_clock::time_point, SOCKET>> timeouts;

    vector<pollfd> fds(1, { m_cnc[0], POLLIN, 0 });
    vector<function<void()>> funcs(1, nullptr);

    while (true)
    {
      {
        SyncLock M(m_mutex);

        fds.insert(fds.end(), m_fds.begin(), m_fds.end());
        funcs.insert(funcs.end(), make_move_iterator(m_funcs.begin()), make_move_iterator(m_funcs.end()));

        m_fds.clear();
        m_funcs.clear();

        for(auto &timeout : m_timeouts)
        {
          auto insertpos = timeouts.begin();
          while (insertpos != timeouts.end() && insertpos->first < timeout.first)
            ++insertpos;

          timeouts.insert(insertpos, timeout);
        }

        m_timeouts.clear();
      }

      sigs.clear();

      int timeout = -1;

      if (timeouts.size() != 0)
      {
        timeout = chrono::duration_cast<chrono::milliseconds>(timeouts.front().first - chrono::steady_clock::now()).count();

        if (timeout < 0)
        {
          timeout = 0;
          sigs.push_back(timeouts.front().second);
        }
      }

      poll(fds.data(), fds.size(), timeout);

      if (fds[0].revents != 0)
      {
        char cnc;
        recv(m_cnc[0], &cnc, sizeof(cnc), 0);

        if (cnc == Exit)
          return 0;

        if (cnc == Signal)
        {
          SyncLock M(m_mutex);

          sigs.insert(sigs.end(), m_signals.begin(), m_signals.end());

          m_signals.clear();
        }

        fds[0].revents = 0;
      }

      for(size_t i = 1; i < fds.size(); )
      {
        if (fds[i].revents != 0 || find(sigs.begin(), sigs.end(), fds[i].fd) != sigs.end())
        {
          funcs[i]();

          timeouts.erase(remove_if(timeouts.begin(), timeouts.end(), [&](auto &tm) { return tm.second == fds[i].fd; }), timeouts.end());

          fds.erase(fds.begin() + i);
          funcs.erase(funcs.begin() + i);
        }
        else
          ++i;
      }
    }
  }


  SocketWaitThread &socket_wait_thread()
  {
    static std::once_flag onceflag;
    static SocketWaitThread *instance;

    call_once(onceflag, [] { instance = new SocketWaitThread; });

    return *instance;
  }
}

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
    if (data)
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
    if (data)
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

    return (::select(max(readfd, writefd)+1, &readfds, &writefds, 0, &t) != 0);
  }

  //|///////////////////// setnonblocking ///////////////////////////////////
  static void setnonblocking(SOCKET socket)
  {
#ifdef _WIN32
    u_long nonblocking = true;
    ioctlsocket(socket, FIONBIO, &nonblocking);
#else
    fcntl(socket, F_SETFL, fcntl(socket, F_GETFL) | O_NONBLOCK);
#endif
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
    return (m_status == SocketStatus::Connected);
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

      FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, 0, m_errorcondition, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, 0);

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

    m_closesignal = false;
  }


  //|///////////////////// StreamSocket::bytes_available ////////////////////
  ///
  /// \return number of bytes ready for reading
  ///
  size_t StreamSocket::bytes_available() const
  {
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
    m_closesignal = true;

    socket_wait_thread().signal(m_connectedsocket);
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


  //|///////////////////// StreamSocket::init_stream ////////////////////////
  StreamSocket::StreamState StreamSocket::init_stream()
  {
    m_bufferhead = 0;
    m_buffertail = 0;
    m_buffercount = 0;
    m_errorcondition = 0;
    m_closesignal = false;
    m_status = SocketStatus::Connected;

    m_activity.release();

    return StreamState::Ok;
  }


  //|///////////////////// StreamSocket::read_stream ////////////////////////
  //
  // Moves data from the connected socket into the receive buffer.
  //
  StreamSocket::StreamState StreamSocket::read_stream()
  {   
    size_t buffercount = m_buffercount.load(std::memory_order_relaxed);

    // No Room in buffer...
    if (buffercount == m_buffer.size())
      return StreamState::Stalled;

    void *buffer = m_buffer.data() + m_buffertail;

    size_t bufbytes = min(m_buffer.size() - buffercount, m_buffer.size() - m_buffertail);

    //
    // Receive any data that is available
    //

    ssize_t bytes = recv(m_connectedsocket, (char*)buffer, bufbytes, 0);

    // check if we are no longer connected...
    if (bytes == 0 || m_closesignal)
    {
      m_errorcondition = 0;
      m_status = SocketStatus::Cactus;

      return StreamState::Dead;
    }

    // or an error occured...
    else if (bytes == SOCKET_ERROR)
    {
      auto result = GetLastError();

      // was it a fatal error that results in a disconnect ?
      if (result != EMSGSIZE && result != EWOULDBLOCK)
      {
        m_errorcondition = result;
        m_status = SocketStatus::Cactus;

        return StreamState::Dead;
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

    return StreamState::Ok;
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
    m_port = 0;
    m_options = 0;
    m_listeningsocket = INVALID_SOCKET;
    m_destroysignal = -1;
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
  /// \param[in] options string containing options
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
    assert(m_listeningsocket == INVALID_SOCKET);
    assert(m_status == SocketStatus::Unborn || m_status == SocketStatus::Dead);

    // Parse out the options

    m_options = Connectable;

    parse_options(options);

    //
    // Initialise and Create
    //

    m_port = port;

    if (!create_and_bind())
    {
      m_status = SocketStatus::Dead;

      throw socket_error("Error Binding Server Socket (" + toa(m_errorcondition) + ")");
    }

    m_status = SocketStatus::Created;

    m_destroysignal += 1;

    handle_created();

    socket_wait_thread().signal(m_listeningsocket);

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
    assert(socket.sid != INVALID_SOCKET);
    assert(m_listeningsocket == INVALID_SOCKET);
    assert(m_status == SocketStatus::Unborn || m_status == SocketStatus::Dead);

    // Parse the options string

    m_options = 0;

    parse_options(options);

    //
    // Initialise and Create
    //

    m_port = 0;

    m_connectedsocket = socket.sid;

    m_status = SocketStatus::Created;

    m_destroysignal += 1;

    handle_created();

    socket_wait_thread().signal(m_connectedsocket);

    return true;
  }


  //|///////////////////// ServerSocket::destroy ////////////////////////////
  void ServerSocket::destroy()
  {
    m_destroysignal += 1;

    while (m_destroysignal != 0)
    {
      close();
      socket_wait_thread().signal(m_listeningsocket);

      sleep_yield();
    }

    close_listener();

    m_destroysignal -= 1;

    StreamSocket::destroy();
  }


  //|///////////////////// ServerSocket::create_and_bind ////////////////////
  bool ServerSocket::create_and_bind()
  {
    sockaddr_t addr = {};

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons((u_short)m_port);

    //
    // Create a socket that listens for connections
    //

    m_listeningsocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (m_listeningsocket == INVALID_SOCKET)
    {
      m_errorcondition = GetLastError();

      return false;
    }

    setnonblocking(m_listeningsocket);

    //
    // Bind the listening socket to the port number
    //

    if (bind(m_listeningsocket, (sockaddr*)&addr, sizeof(addr)) != 0)
    {
      m_errorcondition = GetLastError();

      close_listener();

      return false;
    }

    //
    // Set the socket to listen for incomming connections
    //

    if (listen(m_listeningsocket, 1) == SOCKET_ERROR)
    {
      m_errorcondition = GetLastError();

      close_listener();

      return false;
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


  //|///////////////////// ServerSocket::created ////////////////////////////
  void ServerSocket::handle_created()
  {
    if (m_destroysignal)
    {
      m_destroysignal -= 1;

      return;
    }

    if (m_options & Connectable)
    {
      m_connectedsocket = accept(m_listeningsocket, 0, 0);
    }

    if (m_connectedsocket != INVALID_SOCKET)
    {
      setnonblocking(m_connectedsocket);

      if ((m_options & KeepAlive) != 0)
      {
        int one = 1;
        setsockopt(m_connectedsocket, SOL_SOCKET, SO_KEEPALIVE, (const char *)&one, sizeof(one));
      }

      init_stream();

      handle_connected();
    }
    else
    {
      socket_wait_thread().await(m_listeningsocket, POLLIN, [=]() { handle_created(); });
    }
  }


  //|///////////////////// ServerSocket::connected //////////////////////////
  void ServerSocket::handle_connected()
  {
    auto R = read_stream();

    switch(R)
    {
      case StreamState::Ok:
        socket_wait_thread().await(m_connectedsocket, POLLIN, [=]() { handle_connected(); });
        break;

      case StreamState::Stalled:
        socket_wait_thread().await(m_connectedsocket, POLLIN, [=]() { handle_connected(); }, 32);
        break;

      case StreamState::Dead:
        handle_disconnect();
        break;
    }
  }


  //|///////////////////// ServerSocket::disconnect /////////////////////////
  void ServerSocket::handle_disconnect()
  {
    close_and_invalidate();

    m_activity.release();

    if (m_options & Connectable)
    {
      handle_created();
    }
    else
    {
      m_destroysignal -= 1;
    }
  }



  //|--------------------- ClientSocket -------------------------------------
  //|------------------------------------------------------------------------


  //|///////////////////// ClientSocket::Constructor ////////////////////////
  ClientSocket::ClientSocket()
  {
    m_port = 0;
    m_options = 0;
    m_destroysignal = -1;
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
  /// \param[in] options String containing options
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

    m_port = port;
    m_address = ipaddress;

    m_status = SocketStatus::Created;

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
    if (!m_connect)
    {
      m_connect.set();

      m_destroysignal += 1;

      handle_created();

      socket_wait_thread().signal(m_connectedsocket);
    }

    return connected();
  }


  //|///////////////////// ClientSocket::destroy ////////////////////////////
  void ClientSocket::destroy()
  {
    m_destroysignal += 1;

    while (m_destroysignal != 0)
    {
      close();

      sleep_yield();
    }

    m_connect.reset();

    m_destroysignal -= 1;

    StreamSocket::destroy();
  }


  //|///////////////////// ClientSocket::create_socket //////////////////////
  bool ClientSocket::create_socket()
  {
    //
    // Create the Client Socket
    //

    m_connectedsocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (m_connectedsocket == INVALID_SOCKET)
    {
      m_errorcondition = GetLastError();

      return false;
    }

    setnonblocking(m_connectedsocket);

    // keepalive option
    if ((m_options & keepalive) != 0)
    {
      int one = 1;
      setsockopt(m_connectedsocket, SOL_SOCKET, SO_KEEPALIVE, (const char *)&one, sizeof(one));
    }

    return true;
  }


  //|///////////////////// ClientSocket::connect_socket /////////////////////
  bool ClientSocket::connect_socket()
  {
    sockaddr_t addr = {};

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = 0;
    addr.sin_port = htons((u_short)m_port);

    // Breakdown the ip address

    addr.sin_addr.s_addr = inet_addr(m_address.c_str());

    if (addr.sin_addr.s_addr == INADDR_NONE)
    {
      // Convert a name to an ip address

      if (auto host = gethostbyname(m_address.c_str()))
      {
        addr.sin_addr.s_addr = ((IN_ADDR*)host->h_addr)->s_addr;
      }
    }

    //
    // Connect the Client Socket
    //

    if (::connect(m_connectedsocket, (sockaddr*)&addr, sizeof(addr)) == 0)
      return true;

    auto result = GetLastError();

    if (result != EINPROGRESS && result != EWOULDBLOCK && result != EALREADY && result != EISCONN)
    {
      m_errorcondition = result;

      return false;
    }

    return (result == EISCONN);
  }


  //|///////////////////// ClientSocket::close_and_invalidate ///////////////
  void ClientSocket::close_and_invalidate()
  {
    if (m_connectedsocket != INVALID_SOCKET)
      closesocket(m_connectedsocket);

    m_connectedsocket = INVALID_SOCKET;
  }


  //|///////////////////// ClientSocket::created ////////////////////////////
  void ClientSocket::handle_created()
  {
    if (m_destroysignal)
    {
      m_destroysignal -= 1;

      return;
    }

    if (m_connectedsocket == INVALID_SOCKET)
    {
      if (!create_socket())
      {
        m_status = SocketStatus::Dead;

        m_destroysignal -= 1;

        return;
      }
    }

    if (connect_socket())
    {
      init_stream();

      handle_connected();
    }
    else
    {
      socket_wait_thread().await(m_connectedsocket, POLLOUT, [=]() { handle_created(); }, 2000);
    }
  }


  //|///////////////////// ClientSocket::connected //////////////////////////
  void ClientSocket::handle_connected()
  {
    auto R = read_stream();

    switch(R)
    {
      case StreamState::Ok:
        socket_wait_thread().await(m_connectedsocket, POLLIN, [=]() { handle_connected(); });
        break;

      case StreamState::Stalled:
        socket_wait_thread().await(m_connectedsocket, POLLIN, [=]() { handle_connected(); }, 32);
        break;

      case StreamState::Dead:
        handle_disconnect();
        break;
    }
  }


  //|///////////////////// ClientSocket::disconnect /////////////////////////
  void ClientSocket::handle_disconnect()
  {
    close_and_invalidate();

    m_connect.reset();
    m_activity.release();

    m_destroysignal -= 1;
  }



  //|--------------------- SocketPump ---------------------------------------
  //|------------------------------------------------------------------------


  //|///////////////////// SocketPump::Constructor //////////////////////////
  SocketPump::SocketPump()
    : m_activity(1)
  {
    m_port = 0;
    m_listeningsocket = INVALID_SOCKET;
    m_errorcondition = 0;
    m_destroysignal = -1;
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

    //
    // Initialise the SockAddr Structure
    //

    m_port = port;

    if (!create_and_bind())
    {
      throw socketpump_error("Error Binding Socket Pump (" + toa(m_errorcondition) + ")");
    }

    m_destroysignal += 1;

    handle_created();

    socket_wait_thread().signal(m_listeningsocket);

    return true;
  }


  //|///////////////////// SocketPump::destroy //////////////////////////////
  ///
  /// Destroys the socket pump (the listening socket)
  ///
  void SocketPump::destroy()
  {
    m_destroysignal += 1;

    while (m_destroysignal != 0)
    {
      socket_wait_thread().signal(m_listeningsocket);

      sleep_yield();
    }

    close_listener();

    m_destroysignal -= 1;
  }


  //|///////////////////// SocketPump::create_and_bind //////////////////////
  bool SocketPump::create_and_bind()
  {
    sockaddr_t addr = {};

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons((u_short)m_port);

    //
    // Create a socket that listens for connections
    //

    m_listeningsocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (m_listeningsocket == INVALID_SOCKET)
    {
      m_errorcondition = GetLastError();

      return false;
    }

    setnonblocking(m_listeningsocket);

    int one = 1;
    setsockopt(m_listeningsocket, SOL_SOCKET, SO_REUSEADDR, (const char *)&one, sizeof(one));

    //
    // Bind the listening socket to the port number
    //

    if (bind(m_listeningsocket, (sockaddr*)&addr, sizeof(addr)) != 0)
    {
      m_errorcondition = GetLastError();

      close_listener();

      return false;
    }

    if (listen(m_listeningsocket, SOMAXCONN) == SOCKET_ERROR)
    {
      m_errorcondition = GetLastError();

      close_listener();

      return false;
    }

    return true;
  }


  //|///////////////////// SocketPump::close_listener ///////////////////////
  void SocketPump::close_listener()
  {
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


  //|///////////////////// SocketPump::created //////////////////////////////
  void SocketPump::handle_created()
  {
    if (m_destroysignal)
    {
      m_destroysignal -= 1;

      return;
    }

    m_activity.release();

    socket_wait_thread().await(m_listeningsocket, POLLIN, [=]() { handle_created(); });
  }



  //|--------------------- BroadcastSocket ----------------------------------
  //|------------------------------------------------------------------------

  constexpr size_t BroadcastSocket::kSocketBufferSize;

  //|///////////////////// BroadcastSocket::Constructor /////////////////////
  BroadcastSocket::BroadcastSocket()
  {
    m_port = 0;
    m_options = 0;
    m_bufferhead = 0;
    m_buffertail = 0;
    m_buffercount = 0;
    m_destroysignal = -1;
  }


  //|///////////////////// BroadcastSocket::Constructor /////////////////////
  BroadcastSocket::BroadcastSocket(unsigned int port, const char *options)
    : BroadcastSocket()
  {
    create(port, options);
  }


  //|///////////////////// BroadcastSocket::Constructor /////////////////////
  BroadcastSocket::BroadcastSocket(unsigned int ip, unsigned int port, const char *options)
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
  void BroadcastSocket::parse_options(const char *options)
  {
  }


  //|///////////////////// BroadcastSocket::create //////////////////////////
  ///
  /// Creates a Broadcast Sink.
  ///
  /// \param[in] port The port number on which to create the socket
  /// \param[in] options Options string
  //|
  bool BroadcastSocket::create(unsigned int port, const char *options)
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
  bool BroadcastSocket::create(unsigned int address, unsigned int port, const char *options)
  {
    assert(m_status == SocketStatus::Unborn || m_status == SocketStatus::Dead);

    // Parse out the options

    m_options = 0;

    parse_options(options);

    //
    // Initialise and Create
    //

    m_port = port;
    m_address = address;

    if (!create_and_bind())
    {
      m_status = SocketStatus::Dead;

      throw socket_error("Error Binding Broadcast Socket (" + toa(m_errorcondition) + ")");
    }

    m_bufferhead = 0;
    m_buffertail = 0;
    m_buffercount = 0;
    m_errorcondition = 0;
    m_status = SocketStatus::Created;

    m_destroysignal += 1;

    handle_created();

    socket_wait_thread().signal(m_connectedsocket);

    return true;
  }


  //|///////////////////// BroadcastSocket::destroy /////////////////////////
  void BroadcastSocket::destroy()
  {
    m_destroysignal += 1;

    while (m_destroysignal != 0)
    {
      socket_wait_thread().signal(m_connectedsocket);

      sleep_yield();
    }

    m_destroysignal -= 1;

    SocketBase::destroy();
  }


  //|///////////////////// BroadcastSocket::packet_available ////////////////
  ///
  /// \return true if a packet is available
  ///
  bool BroadcastSocket::packet_available()
  {
    return (m_buffercount > 0);
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
    // Ensure we are created
    if (m_status == SocketStatus::Unborn)
      throw socket_error("Socket Not Created");

    sockaddr_in dest = {};

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


  //|///////////////////// BroadcastSocket::create_and_bind /////////////////
  bool BroadcastSocket::create_and_bind()
  {
    sockaddr_t addr = {};

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = m_address;
    addr.sin_port = htons((u_short)m_port);

    //
    // Create a socket that listens for connections
    //

    m_connectedsocket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if (m_connectedsocket == INVALID_SOCKET)
    {
      m_errorcondition = GetLastError();

      return false;
    }

    setnonblocking(m_connectedsocket);

    int one = 1;
    setsockopt(m_connectedsocket, SOL_SOCKET, SO_REUSEADDR, (const char *)&one, sizeof(one));
    setsockopt(m_connectedsocket, SOL_SOCKET, SO_BROADCAST, (const char *)&one, sizeof(one));

    //
    // Bind the socket to the port number
    //

    if (bind(m_connectedsocket, (sockaddr*)&addr, sizeof(addr)) != 0)
    {
      m_errorcondition = GetLastError();

      close_and_invalidate();

      return false;
    }

    return true;
  }


  //|///////////////////// BroadcastSocket::close_and_invalidate ////////////
  void BroadcastSocket::close_and_invalidate()
  {
    if (m_connectedsocket != INVALID_SOCKET)
      closesocket(m_connectedsocket);

    m_connectedsocket = INVALID_SOCKET;
  }


  //|///////////////////// BroadcastSocket::created /////////////////////////
  void BroadcastSocket::handle_created()
  {
    if (m_destroysignal)
    {
      m_destroysignal -= 1;

      return;
    }

    sockaddr_in addr;

    uint8_t buffer[4096];

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
        m_status = SocketStatus::Cactus;

        m_activity.release();
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

    socket_wait_thread().await(m_connectedsocket, POLLIN, [=]() { handle_created(); });
  }



  //|--------------------- Functions ----------------------------------------
  //|------------------------------------------------------------------------

  //////////////////////// readline /////////////////////////////////////////
  bool readline(StreamSocket &socket, char *buffer, int n, int timeout)
  {
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
    WSAIoctl(sock, SIO_GET_INTERFACE_LIST, 0, 0, buffer, sizeof(buffer), &bytes, 0, 0);

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
