//==============================================================================
//
//  timeb.h
//
#ifndef TIMEB_H_INCLUDED
#define TIMEB_H_INCLUDED

#include "cstdint"
#include "windows.h"

//------------------------------------------------------------------------------
//
//  Windows clock time
//
struct _timeb
{
   time_t   time;
   uint16_t millitm;
   int16_t  timezone;
   int16_t  dstflag;
};

errno_t _ftime_s(_timeb* Time);

#endif
