//==============================================================================
//
//  poll.h
//
#ifdef OS_LINUX
#ifndef POLL_H_INCLUDED
#define POLL_H_INCLUDED

struct pollfd
{
   int fd;
   short events;
   short revents;
};

typedef unsigned long nfds_t;

constexpr int POLLERR = 0x008;
constexpr int POLLHUP = 0x010;
constexpr int POLLNVAL = 0x020;
constexpr int POLLRDNORM = 0x040;
constexpr int POLLRDBAND = 0x080;
constexpr int POLLWRNORM = 0x100;
constexpr int POLLWRBAND = 0x200;

int poll(pollfd* fds, nfds_t nfds, int timeout);

#endif
#endif
