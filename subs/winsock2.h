//==============================================================================
//
//  winsock2.h
//
#ifndef WINSOCK2_H_INCLUDED
#define WINSOCK2_H_INCLUDED

#include "cstddef"
#include "cstdint"
#include "windows.h"

//------------------------------------------------------------------------------
//
//  Windows sockets and networking
//
constexpr uint16_t AF_UNSPEC = 0;
constexpr uint16_t AF_INET = 2;
constexpr uint16_t AF_INET6 = 23;
constexpr int      SOCK_STREAM = 1;
constexpr int      SOCK_DGRAM = 2;
constexpr uint32_t IPPROTO_TCP = 6;
constexpr uint32_t IPPROTO_UDP = 17;
constexpr uint32_t INADDR_ANY = 0x0;
constexpr uint32_t INADDR_LOOPBACK = 0x7f000001;
constexpr uint32_t INADDR_NONE = 0xffffffff;

typedef uintptr_t SOCKET;
constexpr SOCKET INVALID_SOCKET = UINTPTR_MAX;

constexpr int NO_ERROR = 0;
constexpr int SOCKET_ERROR = -1;

constexpr int SD_SEND = 1;
constexpr int SD_BOTH = 2;
constexpr int SOMAXCONN = 0x7fffffff;

constexpr uint16_t SOL_SOCKET = 0xffff;
constexpr uint16_t SO_KEEPALIVE = 0x0008;
constexpr uint16_t SO_LINGER = 0x0080;
constexpr uint16_t SO_SNDBUF = 0x1001;
constexpr uint16_t SO_RCVBUF = 0x1002;
constexpr uint16_t SO_MAX_MSG_SIZE = 0x2003;

constexpr u_long FIONBIO = 126;
constexpr u_long FIONREAD = 127;

constexpr int IPPROTO_IPV6 = 41;
constexpr int IPV6_V6ONLY = 27;

struct sockaddr
{ 
   uint16_t sa_family;
   char     sa_data[14];
};

struct in_addr
{
   int32_t s_addr;
};

struct sockaddr_in
{
   uint16_t sin_family;
   uint16_t sin_port;
   in_addr  sin_addr;
   char     sin_zero[8];
};

struct addrinfo
{
   int       ai_flags;
   int       ai_family;
   int       ai_socktype;
   int       ai_protocol;
   size_t    ai_addrlen;
   char*     ai_canonname;
   sockaddr* ai_addr;
   addrinfo* ai_next;
};

struct linger
{
   uint16_t l_onoff;
   uint16_t l_linger;
};

constexpr int POLLERR = 0x0001;
constexpr int POLLHUP = 0x0002;
constexpr int POLLNVAL = 0x0004;
constexpr int POLLWRNORM = 0x0010;
constexpr int POLLWRBAND = 0x0020;
constexpr int POLLRDNORM = 0x0100;
constexpr int POLLRDBAND = 0x0200;

struct pollfd
{
   SOCKET  fd;
   int16_t events;
   int16_t revents;
};

uint64_t htonll(uint64_t hostllong);
uint32_t htonl(uint32_t hostlong);
uint16_t htons(uint16_t hostshort);
uint64_t ntohll(uint32_t netllong);
uint32_t ntohl(uint32_t netlong);
uint16_t ntohs(uint16_t netshort);

int    gethostname(char* name, int namelen);
SOCKET socket(int32_t af, int type, int protocol);
int    getsockname(SOCKET s, sockaddr* name, int* namelen);
int    getpeername(SOCKET s, sockaddr* name, int* namelen);
int    getsockopt(SOCKET s, int level, int optname, char* optval, int* optlen);
int    setsockopt(SOCKET s, int level, int optname, const char* optval, int optlen);
int    ioctlsocket(SOCKET s, int cmd, u_long* args);
int    bind(SOCKET s, const sockaddr* name, int namelen);
int    listen(SOCKET s, int backlog);
int    WSAPoll(pollfd* fdArray, uint32_t fds, int timeout);
SOCKET accept(SOCKET s, sockaddr* addr, int* addrlen);
int    recv(SOCKET s, char* buf, int len, int flags);
int    connect(SOCKET s, sockaddr* name, int namelen);
int    send(SOCKET s, const char* buf, int len, int flags);
int    closesocket(SOCKET s);
int    shutdown(SOCKET s, int how);
int    recvfrom(SOCKET s, char* buf, int len, int flags, sockaddr* from, int* fromlen);
int    sendto(SOCKET s, const char* buf, int len, int flags, const sockaddr* to, int tolen);

struct WSAData
{
   WORD  wVersion;
   WORD  wHighVersion;
   char  szDescription[257];
   char  szSystemStatus[129];
   WORD  iMaxSockets;
   WORD  iMaxUdpDg;
   char* lpVendorInfo;
};

constexpr int WSA_NOT_ENOUGH_MEMORY = 8;
constexpr int WSAEWOULDBLOCK = 10035;
constexpr int WSAENOPROTOOPT = 10042;
constexpr int WSAECONNRESET = 10054;
constexpr int WSAENOTCONN = 10057;

int WSAStartup(WORD versionRequested, WSAData* data);
int WSAGetLastError();
int WSACleanup();

#endif
