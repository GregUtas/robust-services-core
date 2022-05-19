//==============================================================================
//
//  errno.h
//
#ifdef OS_LINUX
#ifndef ERRNO_H_INCLUDED
#define ERRNO_H_INCLUDED

extern int errno;

constexpr int EPERM = 1;
constexpr int EINTR = 4;
constexpr int EWOULDBLOCK = 11;
constexpr int EINVAL = 22;
constexpr int EPROTONOSUPPORT = 93;
constexpr int ENETDOWN = 100;
constexpr int ECONNRESET = 104;
constexpr int ENOTCONN = 107;

#endif
#endif
