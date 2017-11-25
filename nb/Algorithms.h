//==============================================================================
//
//  Algorithms.h
//
//  Copyright (C) 2017  Greg Utas
//
//  This file is part of the Robust Services Core (RSC).
//
//  RSC is free software: you can redistribute it and/or modify it under the
//  terms of the GNU General Public License as published by the Free Software
//  Foundation, either version 3 of the License, or (at your option) any later
//  version.
//
//  RSC is distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
//  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
//  details.
//
//  You should have received a copy of the GNU General Public License along
//  with RSC.  If not, see <http://www.gnu.org/licenses/>.
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
