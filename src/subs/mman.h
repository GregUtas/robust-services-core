//==============================================================================
//
//  mman.h
//
#ifdef OS_LINUX
#ifndef MMAN_H_INCLUDED
#define MMAN_H_INCLUDED

#include "cstddef"

typedef long int off_t;

constexpr int PROT_NONE = 0x0;
constexpr int PROT_READ = 0x1;
constexpr int PROT_WRITE = 0x2;
constexpr int PROT_EXEC = 0x4;

constexpr int MAP_PRIVATE = 0x02;
constexpr int MAP_ANONYMOUS = 0x20;

extern void* mmap(void* addr, size_t len, int protection,int flags, int fd, off_t offset);
extern int munmap(void* addr, size_t len);
extern int mlock(const void* addr, size_t len);
extern int munlock(const void* addr, size_t len);
extern int mprotect(void* addr, size_t len, int protection);

#endif
#endif
