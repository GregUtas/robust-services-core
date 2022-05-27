//==============================================================================
//
//  stdlib.h
//
#ifdef OS_WIN
#ifndef STDLIB_H_INCLUDED
#define STDLIB_H_INCLUDED

#include "cstdint"

constexpr int _WRITE_ABORT_MSG = 0x1;
constexpr int _CALL_REPORTFAULT = 0x2;

unsigned int _set_abort_behavior(unsigned int flags, unsigned int mask);

#endif
#endif
