//==============================================================================
//
//  CfgIntParm.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef CFGINTPARM_H_INCLUDED
#define CFGINTPARM_H_INCLUDED

#include "CfgParm.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Configuration parameter for integer values.
//
class CfgIntParm : public CfgParm
{
public:
   //  Creates a parameter with the specified attributes.
   //
   CfgIntParm(const char* key, const char* def, word* field,
      word min, word max, const char* expl);

   //  Virtual to allow subclassing.
   //
   virtual ~CfgIntParm();

   //  Overridden to display member variables.
   //
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   virtual void Patch(sel_t selector, void* arguments) override;
protected:
   //  Returns the parameter's current value.
   //
   word GetCurrValue() const { return *curr_; }

   //  Sets the parameter's next value.  May be overridden to prevent
   //  invalid settings.  If so, invoke this version before returning
   //  true.
   //
   virtual bool SetNextValue(word value);

   //  Overridden to prefix the parameter's type and allowed values.
   //
   virtual void Explain(std::string& expl) const override;

   //  Calls GetCurrValue and maps the result to a string that corresponds
   //  to the parameter's current value.
   //
   virtual std::string GetCurr() const override;

   //  Overridden to transfer next_ to curr_.
   //
   virtual void SetCurr() override;

   //  Calls SetNextValue based on the value of INPUT, which is converted
   //  to an integer by reading it from an istringstream.
   //
   virtual bool SetNext(const std::string& input) override;
private:
   //  A pointer to the field that contains the parameter's value.
   //
   word* const curr_;

   //  The value to be set during an appropriate restart.
   //
   word next_;

   //  The minimum value allowed for the parameter.
   //
   word min_;

   //  The maximum value allowed for the parameter.
   //
   word max_;
};
}
#endif
