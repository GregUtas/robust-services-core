//==============================================================================
//
//  in.h
//
#ifdef OS_LINUX
#ifndef IN_H_INCLUDED
#define IN_H_INCLUDED

#include "cstdint"

constexpr int IPPROTO_IPV6 = 41;

constexpr int IPV6_V6ONLY = 26;

typedef uint32_t in_addr_t;

constexpr in_addr_t INADDR_ANY = 0;

struct in_addr
{
   in_addr_t s_addr;
};

struct in6_addr
{
   union
   {
      uint8_t	s6_addr[16];
      uint16_t s6_addr16[8];
      uint32_t s6_addr32[4];
   };
};

const in6_addr in6addr_any = { 0 };

uint32_t htonl(uint32_t hostlong);
uint16_t htons(uint16_t hostshort);
uint32_t ntohl(uint32_t netlong);
uint16_t ntohs(uint16_t netshort);

struct sockaddr_in
{
   uint16_t sin_family;
   uint16_t sin_port;
   in_addr sin_addr;
   uint8_t sin_zero[8];
};

struct sockaddr_in6
{
   uint16_t sin6_family;
   uint16_t sin6_port;
   uint32_t sin6_flowinfo;
   in6_addr sin6_addr;
   uint32_t sin6_scope_id;
}; 

#endif
#endif
