//==============================================================================
//
//  resource.h
//
#ifdef OS_LINUX
#ifndef RESOURCE_H_INCLUDED
#define RESOURCE_H_INCLUDED

typedef unsigned int id_t;

enum priority_which
{
   PRIO_PROCESS = 0,
   PRIO_PGRP = 1,
   PRIO_USER = 2
};

int setpriority(priority_which which, id_t who, int priority);

#endif
#endif
