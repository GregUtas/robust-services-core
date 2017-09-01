//==============================================================================
//
//  CliCookie.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef CLICOOKIE_H_INCLUDED
#define CLICOOKIE_H_INCLUDED

#include "Object.h"
#include <cstddef>

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Accesses the current location in a command's parameter tree when parsing
//  the input stream.
//
class CliCookie : public Object
{
public:
   //> The maximum depth of CLI parameter nesting.
   //
   static const size_t MaxParmDepth = 9;

   //  Public so that an instance can be declared as a member.
   //
   CliCookie();

   //  Not subclassed.
   //
   ~CliCookie();

   //  Initializes the cookie prior to parsing the next line in
   //  the input stream.
   //
   void Initialize();

   //  Returns the index associated with the parameter at DEPTH.
   //
   size_t Index(size_t depth) const;

   //  Proceeds to the next parameter at the same depth.
   //
   void Advance();

   //  Increases the parse depth when the current parameter has
   //  its own parameters.
   //
   void Descend();

   //  Increases the parse depth when the current parameter (at
   //  offset INDEX) has its own parameters.
   //
   void Descend(size_t index);

   //  Proceeds to the next parameter when all of the parameters
   //  at the current depth have been found.
   //
   void Ascend();

   //  Overridden to display member variables.
   //
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   virtual void Patch(sel_t selector, void* arguments) override;
private:
   //  The current location at each level in the parameter tree.
   //
   size_t index_[MaxParmDepth];

   //  The current depth in the parameter tree.
   //
   size_t depth_;
};
}
#endif
