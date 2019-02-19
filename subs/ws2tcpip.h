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
int inet_pton(uint16_t family, const char* addr, in_addr* buff);

void freeaddrinfo(addrinfo* info);

int getaddrinfo(const char* nodeName, const char* serviceName,
   const addrinfo* hints, addrinfo** result);

int getnameinfo(const sockaddr* addr, int addrLength, char* nodeBuffer,
   DWORD nodeBufferSize, char* serviceBuffer, DWORD serviceBufferSize, int flags);

#endif