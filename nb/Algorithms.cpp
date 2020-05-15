//==============================================================================
//
//  Algorithms.cpp
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
#include "Algorithms.h"
#include <cstdlib>
#include <cstring>

//------------------------------------------------------------------------------

namespace NodeBase
{
size_t find_first_one(uword n)
{
   if(n == 0) return BITS_PER_WORD;

   int i = 0;
   for(NO_OP; ((n & 0x01) == 0); ++i) n >>= 1;
   return i;
}

//------------------------------------------------------------------------------

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

size_t log2(size_t n, bool up)
{
   if(n == 0) return 0;

   size_t i = 0;

   if(up) --n;
   for(n; n > 0; n >>= 1) ++i;
   if(!up) --i;

   return i;
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
   uint32_t lower = uint32_t(b << 16) + c;
   result += lower;
   return result;
}

//------------------------------------------------------------------------------

uint64_t pack4(uint16_t a, uint16_t b, uint16_t c, uint16_t d)
{
   uint64_t result = uint32_t(a << 16) + b;
   result <<= 32;
   uint32_t lower = uint32_t(c << 16) + d;
   result += lower;
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

size_t round_to_2_exp_n(size_t n, size_t e, bool up)
{
   auto incr = (1 << e);
   if(up) n += (incr - 1);
   n = (n >> e) << e;
   return n;
}

//------------------------------------------------------------------------------

uint32_t string_hash(c_string s)
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
