//==============================================================================
//
//  CliCharParm.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef CLICHARPARM_H_INCLUDED
#define CLICHARPARM_H_INCLUDED

#include "CliParm.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  CLI character parameter.
//
class CliCharParm : public CliParm
{
public:
   //  HELP and OPT are passed to CliParm.  CHARS lists the characters
   //  that are valid for this parameter.
   //
   CliCharParm(const char* help, const char* chars,
      bool opt = false, const char* tag = nullptr);

   //  Virtual to allow subclassing.
   //
   virtual ~CliCharParm();

   //  Overridden to display member variables.
   //
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   virtual void Patch(sel_t selector, void* arguments) override;
protected:
   //  Overridden to look for a valid character.
   //
   virtual Rc GetCharParmRc(char& c, CliThread& cli) const override;
private:
   //  Overridden to show the acceptable character inputs.
   //
   virtual bool ShowValues(std::string& values) const override;

   //> Separates valid input characters in parameter help text.
   //
   static const char CharSeparator;

   //  The characters that are valid for this parameter.
   //
   const char* chars_;
};
}
#endif
