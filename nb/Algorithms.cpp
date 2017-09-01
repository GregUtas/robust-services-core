//==============================================================================
//
//  Algorithms.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "Algorithms.h"
#include <cstdlib>
#include <cstring>
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
void* getptr1(const void* ptr2, ptrdiff_t diff)
{
   return (void*) ((const_ptr_t) ptr2 - diff);
}

//------------------------------------------------------------------------------

void* getptr2(const void* ptr1, ptrdiff_t diff)
{
   return (void*) ((const_ptr_t) ptr1 + diff);
}

//------------------------------------------------------------------------------

uint64_t pack2(uint32_t a, uint32_t b)
{
   uint64_t result = a;
   result <<= 32;
   result += b;
   return result;
}

//------------------------------------------------------------------------------

uint64_t pack3(uint16_t a, uint16_t b, uint16_t c)
{
   uint64_t result = a;
   result <<= 32;
   result += ((b << 16) + c);
   return result;
}

//------------------------------------------------------------------------------

uint64_t pack4(uint16_t a, uint16_t b, uint16_t c, uint16_t d)
{
   uint64_t result = (a << 16) + b;
   result <<= 32;
   result += ((c << 16) + d);
   return result;
}

//------------------------------------------------------------------------------

ptrdiff_t ptrdiff(const void* ptr1, const void* ptr2)
{
   return ((const_ptr_t) ptr1 - (const_ptr_t) ptr2);
}

//------------------------------------------------------------------------------

uint32_t rand(uint32_t min, uint32_t max)
{
   //  Return a rand value, min <= value <= max.
   //
   return (double(::rand()) / (RAND_MAX + 1)) * (max - min) + min;
}

//------------------------------------------------------------------------------

uint32_t stringHash(const char* s)
{
   uint64_t hash = 0;

   auto size = strlen(s);

   for(size_t i = 0; i < size; ++i)
   {
      hash = s[i] + (hash << 16) + (hash << 6) - hash;
   }

   return hash;
}
}
