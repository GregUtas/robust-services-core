//==============================================================================
//
//  cxxabi.h
//
#ifdef OS_LINUX
#ifndef CXXABI_H_INCLUDED
#define CXXABI_H_INCLUDED

namespace __cxxabiv1
{
   char* __cxa_demangle(const char* name, char* buffer, size_t* length, int* status);
}

#endif
#endif
