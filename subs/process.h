//==============================================================================
//
//  process.h
//
#ifndef PROCESS_H_INCLUDED
#define PROCESS_H_INCLUDED

#include "cstdint"

//------------------------------------------------------------------------------
//
//  Windows threads
//
typedef unsigned (*_beginthreadex_proc_type)(void*);

uintptr_t _beginthreadex(void* securityAttributes, unsigned stackSize,
                         _beginthreadex_proc_type entryFunction, void* argument,
                         unsigned initFlags, unsigned* threadId);
#endif