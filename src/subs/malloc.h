//==============================================================================
//
//  malloc.h
//
#ifdef OS_LINUX
#ifndef MCHECK_H_INCLUDED
#define MCHECK_H_INCLUDED

#include "cstddef"

size_t malloc_usable_size(void* addr);

#endif
#endif
