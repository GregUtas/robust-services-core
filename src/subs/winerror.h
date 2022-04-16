//==============================================================================
//
//  winerror.h
//
#ifdef OS_WIN
#ifndef WINERROR_H_INCLUDED
#define WINERROR_H_INCLUDED

constexpr int WSAEINTR = 10004;
constexpr int WSAEPROTONOSUPPORT = 10043;
constexpr int WSAENETDOWN = 10050;
constexpr int WSASYSNOTREADY = 10091;
constexpr int WSAVERNOTSUPPORTED = 10092;
constexpr int WSANOTINITIALISED = 10093;

#endif
#endif
