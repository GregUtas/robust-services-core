//==============================================================================
//
//  io.h
//
#ifndef IO_H_INCLUDED
#define IO_H_INCLUDED

#include "cstdint"

//------------------------------------------------------------------------------
//
//  Windows files
//
struct _finddata_t
{
   uint32_t attrib;
   int64_t  time_create;
   int64_t  time_access;
   int64_t  time_write;
   uint32_t size;
   char     name[260];
};

const int _A_SUBDIR = 0x10;

int*     _errno();
intptr_t _findfirst(const char* Filename, _finddata_t* FindData);
int      _findnext(intptr_t FindHandle, _finddata_t* FindData);
int      _findclose(intptr_t FindHandle);

#endif