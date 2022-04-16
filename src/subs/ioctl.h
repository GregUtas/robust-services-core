//==============================================================================
//
//  ioctl.h
//
#ifdef OS_LINUX
#ifndef IOCTL_H_INCLUDED
#define IOCTL_H_INCLUDED

constexpr unsigned long FIONREAD = 0x541B;
constexpr unsigned long FIONBIO = 0x5421;

int ioctl(int fd, unsigned long request, unsigned long* args);

#endif
#endif
