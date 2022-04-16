//==============================================================================
//
//  endian.h
//
#ifdef OS_LINUX
#ifndef ENDIAN_H_INCLUDED
#define ENDIAN_H_INCLUDED

#include "cstdint"

uint64_t htobe64(uint64_t hostllong);
uint64_t be64toh(uint64_t hostllong);

#endif
#endif
