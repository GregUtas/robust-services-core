//==============================================================================
//
//  Algorithms.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef ALGORITHMS_H_INCLUDED
#define ALGORITHMS_H_INCLUDED

#include <cstddef>
#include <cstdint>

//------------------------------------------------------------------------------

namespace NodeBase
{
   //  Returns the distance in bytes between PTR1 and PTR2.
   //
   ptrdiff_t ptrdiff(const void* ptr1, const void* ptr2);

   //  Given PTR2 and the byte offset DIFF, returns PTR2 - DIFF.
   //
   void* getptr1(const void* ptr2, ptrdiff_t diff);

   //  Given PTR1 and the byte offset DIFF, returns PTR1 + DIFF.
   //
   void* getptr2(const void* ptr1, ptrdiff_t diff);

   //  Combines 2 uint32s into a uint64_t of the form 0xAABB.
   //
   uint64_t pack2(uint32_t a, uint32_t b);

   //  Combines 3 uint16s into a uint64_t of the form 0x0ABC.
   //
   uint64_t pack3(uint16_t a, uint16_t b, uint16_t c);

   //  Combines 4 uint16s into a uint64_t of the form 0xABCD.
   //
   uint64_t pack4(uint16_t a, uint16_t b, uint16_t c, uint16_t d);

   //  Returns an integer between MIN and MAX inclusive.
   //
   uint32_t rand(uint32_t min, uint32_t max);

   //  Returns a hash value for S.
   //
   uint32_t stringHash(const char* s);
}
#endif
