//==============================================================================
//
//  wait.h
//
#ifdef OS_LINUX
#ifndef WAIT_H_INCLUDED
#define WAIT_H_INCLUDED

#include "sched.h"

pid_t waitpid(pid_t pid, int* status, int options);

#endif
#endif
