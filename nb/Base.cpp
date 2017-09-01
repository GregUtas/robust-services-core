//==============================================================================
//
//  Base.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "Base.h"
#include <ostream>
#include <typeinfo>
#include "Debug.h"
#include "Formatters.h"
#include "NbTypes.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
fn_name Base_ctor = "Base.ctor";

Base::Base()
{
   Debug::ft(Base_ctor);
}

//------------------------------------------------------------------------------

fn_name Base_ClaimBlocks = "Base.ClaimBlocks";

void Base::ClaimBlocks()
{
   Debug::ft(Base_ClaimBlocks);

   Base* objects[MaxSubtendedCount];
   size_t count = 0;

   //  Claim this object and all of the objects that it owns.
   //
   GetSubtended(objects, count);

   for(size_t i = 0; i < count; ++i)
   {
      objects[i]->Claim();
   }
}

//------------------------------------------------------------------------------

const char* Base::ClassName() const
{
   try
   {
      return typeid(*this).name();
   }

   catch(...)
   {
      return ERROR_STR;
   }
}

//------------------------------------------------------------------------------

void Base::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   stream << prefix << strClass(this) << CRLF;
   stream << prefix << "this : " << this << CRLF;
}

//------------------------------------------------------------------------------

fn_name Base_GetSubtended = "Base.GetSubtended";

void Base::GetSubtended(Base* objects[], size_t& count) const
{
   Debug::ft(Base_GetSubtended);

   if(count < MaxSubtendedCount)
   {
      objects[count++] = const_cast< Base* >(this);
      return;
   }

   Debug::SwErr(Base_GetSubtended, count, 0);
}

//------------------------------------------------------------------------------

fn_name Base_LogSubtended = "Base.LogSubtended";

void Base::LogSubtended(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Debug::ft(Base_LogSubtended);

   Base* objects[MaxSubtendedCount];
   size_t count = 0;

   GetSubtended(objects, count);

   for(size_t i = 0; i < count; ++i)
   {
      if(i > 0) stream << prefix << string(60 - prefix.size(), '-') << CRLF;
      objects[i]->Display(stream, prefix, options);
   }
}

//------------------------------------------------------------------------------

void Base::Nullify(size_t n)
{
   //  Set this object's vptr to a value that will cause a trap if a virtual
   //  function on the object is invoked.
   //
   auto obj = reinterpret_cast< ObjectStruct* >(this);
   obj->vptr = BAD_POINTER;

   if(n > 0)
   {
      //  Nullify the object's data so that N bytes (including the vptr) end
      //  up being nullified.
      //
      n = (n >> BYTES_PER_WORD_LOG2) - 1;
      for(size_t i = 0; i < n; ++i) obj->data[i] = BAD_POINTER;
   }
}

//------------------------------------------------------------------------------

void Base::Output(ostream& stream, col_t indent, bool verbose) const
{
   auto opts = Flags();
   if(verbose) opts.set(DispVerbose);
   Display(stream, spaces(indent), opts);
}

//------------------------------------------------------------------------------

Base::vptr_t Base::Vptr() const
{
   //  Return this object's vptr.
   //
   auto obj = reinterpret_cast< const ObjectStruct* >(this);
   return obj->vptr;
}
}
