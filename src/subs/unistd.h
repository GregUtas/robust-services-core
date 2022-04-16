//==============================================================================
//
//  unistd.h
//
#ifdef OS_LINUX
#ifndef UNISTD_H_INCLUDED
#define UNISTD_H_INCLUDED

#include "cstddef"

int gethostname(char* name, size_t len);
int close(int fd);

#endif
#endif
