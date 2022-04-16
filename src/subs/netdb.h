//==============================================================================
//
//  netdb.h
//
#ifdef OS_LINUX
#ifndef NETDB_H_INCLUDED
#define NETDB_H_INCLUDED

#include "socket.h"

struct addrinfo
{
   int ai_flags;
   int ai_family;
   int ai_socktype;
   int ai_protocol;
   socklen_t ai_addrlen;
   sockaddr* ai_addr;
   char* ai_canonname;
   addrinfo* ai_next;
};

int getaddrinfo(const char* name, const char* service,
   const addrinfo* req, addrinfo** pai);

void freeaddrinfo(addrinfo* ai);

int getnameinfo(const sockaddr* sa, socklen_t salen, char* host,
   socklen_t hostlen, char* serv, socklen_t servlen, int flags);

#endif
#endif
