//==============================================================================
//
//  socket.h
//
#ifdef OS_LINUX
#ifndef SOCKET_H_INCLUDED
#define SOCKET_H_INCLUDED

#include "cstddef"
#include "cstdint"

typedef unsigned int socklen_t;
typedef long ssize_t;

constexpr uint16_t AF_UNSPEC = 0;
constexpr uint16_t AF_INET = 2;
constexpr uint16_t AF_INET6 = 10;
constexpr int      SOCK_STREAM = 1;
constexpr int      SOCK_DGRAM = 2;
constexpr uint32_t IPPROTO_TCP = 6;
constexpr uint32_t IPPROTO_UDP = 17;

struct sockaddr
{
   uint16_t sa_family;
   char sa_data[14];
};

struct linger
{
   int l_onoff;
   int l_linger;
};

constexpr int SOL_SOCKET = 1;
constexpr int SO_SNDBUF = 7;
constexpr int SO_RCVBUF = 8;
constexpr int SO_KEEPALIVE = 9;
constexpr int SO_LINGER	= 13;

constexpr int SHUT_WR = 1;
constexpr int SHUT_RDWR = 2;

constexpr int SOMAXCONN = 4096;

int socket(int domain, int type, int protocol);
int getsockname(int fd, sockaddr* addr, socklen_t* len);
int getpeername(int fd, sockaddr* addr, socklen_t* len);
int getsockopt(int fd, int level, int optname, void* optval, socklen_t* optlen);
int setsockopt(int fd, int level, int optname, const void* optval, socklen_t optlen);
int bind(int fd, const sockaddr* addr, socklen_t len);
int listen(int fd, int n);
int accept(int fd, sockaddr* addr, socklen_t* addr_len);
int recv(int fd, void* buf, size_t n, int flags);
int connect(int fd, const sockaddr* addr, socklen_t len);
ssize_t send(int fd, const void* buf, size_t n, int flags);
ssize_t recvfrom(int fd, void* buf, size_t n, int flags, sockaddr* addr, socklen_t* addr_len);
ssize_t sendto(int fd, const void* buf, size_t n, int flags, const sockaddr* addr, socklen_t addr_len);
int shutdown(int fd, int how);

#endif
#endif
