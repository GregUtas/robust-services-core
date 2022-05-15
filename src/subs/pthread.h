//==============================================================================
//
//  pthread.h
//
#ifdef OS_LINUX
#ifndef PTHREAD_H_INCLUDED
#define PTHREAD_H_INCLUDED

#include "cstddef"

enum  // state
{
   PTHREAD_CREATE_JOINABLE,
   PTHREAD_CREATE_DETACHED
};

enum  // inherit
{
   PTHREAD_INHERIT_SCHED,
   PTHREAD_EXPLICIT_SCHED
};

enum  // policy
{
   SCHED_OTHER,
   SCHED_FIFO,
   SCHED_RR
};

union pthread_attr_t;
int pthread_attr_init(pthread_attr_t* attrs);
int pthread_attr_setdetachstate(pthread_attr_t* attrs, int state);
int pthread_attr_setinheritsched(pthread_attr_t* attrs, int inherit);
int pthread_attr_setschedpolicy(pthread_attr_t* attrs, int policy);
int pthread_attr_setstacksize(pthread_attr_t* attrs, size_t size);

typedef unsigned long int pthread_t;
pthread_t pthread_self();
int pthread_create(pthread_t* thread, const pthread_attr_t* attrs, void* (*entry)(void*), void* arg);
int pthread_setschedprio(pthread_t thread, int prio);

struct sched_param
{
   int sched_priority;
};

int pthread_setschedparam(pthread_t thread, int policy, const sched_param* parm);

#endif
#endif
