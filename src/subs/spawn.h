//==============================================================================
//
//  spawn.h
//
#ifdef OS_LINUX
#ifndef SPAWN_H_INCLUDED
#define SPAWN_H_INCLUDED

#include "sched.h"

struct posix_spawnattr_t;

struct posix_spawn_file_actions_t;

int posix_spawnp(pid_t* pid,
   const char* exe,
   const posix_spawn_file_actions_t* actions,
   const posix_spawnattr_t* attrs,
   char* const argv[],
   char* const envp[]);

#endif
#endif
