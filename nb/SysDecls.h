//==============================================================================
//
//  SysDecls.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
//  Operating system abstraction layer: basic types.
//
//  This file is platform dependent and must therefore be replaced on each
//  platform.  This one is for 32-bit Windows, although it currently works for
//  64-bit Windows as well.  Low level, platform independent types--even if
//  defined in terms of types in this header-- should be defined in SysTypes.h.
//
//  Sys*.h files define the operating system abstraction layer.  Except for
//  this one, they should not have to be modified to support a new platform.
//  Similarly, Sys*.cpp files are intended to be platform independent and
//  should not require modification.  However, Sys*.<os>.cpp files, where
//  <os> is a particular platform, will need to be replaced.
//
#ifndef SYSDECLS_H_INCLUDED
#define SYSDECLS_H_INCLUDED

#include <cstdint>

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Handles for native OS objects.
//
typedef void* SysHeap_t;
typedef void* SysThread_t;
typedef uint32_t SysThreadId;
typedef void* SysMutex_t;
typedef void* SysSentry_t;
typedef uintptr_t SysSocket_t;
}
#endif
