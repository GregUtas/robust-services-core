//==============================================================================
//
//  CfgBitParm.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef CFGBITPARM_H_INCLUDED
#define CFGBITPARM_H_INCLUDED

#include "CfgParm.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Configuration parameter virtual base class for bools and flags.
//
class CfgBitParm : public CfgParm
{
public:
   //  Virtual to allow subclassing.
   //
   virtual ~CfgBitParm();

   //  Overridden for patching.
   //
   virtual void Patch(sel_t selector, void* arguments) override;
protected:
   //  Creates a parameter with the specified attributes.  Protected
   //  because this class is virtual.
   //
   CfgBitParm(const char* key, const char* def, const char* expl);

   //  Returns the parameter's current value.
   //
   virtual bool GetCurrValue() const = 0;

   //  Overridden to prefix the parameter's type and allowed values.
   //
   virtual void Explain(std::string& expl) const override;

   //  Calls GetCurrValue and maps the result to ValidTrueChars[0]
   //  or ValidFalseChars[0].
   //
   virtual std::string GetCurr() const override;

   //  Calls SetNextValue based on the value of INPUT, which must
   //  be a character in ValidTrueChars or ValidFalseChars.
   //
   virtual bool SetNext(const std::string& input) override;
private:
   //  Sets the parameter's next value.  Returns false if VALUE
   //  is invalid.
   //
   virtual bool SetNextValue(bool value) = 0;

   //  Returns a string containing the characters that set a
   //  configuration parameter to "true".
   //
   static fixed_string ValidTrueChars();

   //  Returns a string containing the characters that set a
   //  configuration parameter to "false".
   //
   static fixed_string ValidFalseChars();
};
}
#endif
