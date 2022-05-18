//==============================================================================
//
//  execinfo.h
//
#ifdef OS_LINUX
#ifndef EXECINFO_H_INCLUDED
#define EXECINFO_H_INCLUDED

int backtrace(void** addrs, int size);
char** backtrace_symbols(const void** addrs, int size);

#endif
#endif
