//==============================================================================
//
//  ws2tcpip.h
//
#ifndef WS2TCPIP_H_INCLUDED
#define WS2TCPIP_H_INCLUDED

#include "cstdint"
#include "windows.h"
#include "winsock2.h"

//------------------------------------------------------------------------------
//
//  Windows TCP/IP
//
struct in6_addr
{
   uint8_t  s6_bytes[16];
   uint16_t s6_words[8];
};

const in6_addr in6addr_any = { 0 };

struct sockaddr_in6
{
   uint16_t sin6_family;
   uint16_t sin6_port;
   ULONG    sin6_flowinfo;
   in6_addr sin6_addr;
   ULONG    sin6_scope_id;
};

void freeaddrinfo(addrinfo* info);

int getaddrinfo(const char* nodeName, const char* serviceName,
   const addrinfo* hints, addrinfo** result);

int getnameinfo(const sockaddr* addr, int addrLength, char* nodeBuffer,
   DWORD nodeBufferSize, char* serviceBuffer, DWORD serviceBufferSize, int flags);

#endif
