//==============================================================================
//
//  CfgBoolParm.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef CFGBOOLPARM_H_INCLUDED
#define CFGBOOLPARM_H_INCLUDED

#include "CfgBitParm.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Configuration parameter for boolean values.
//
class CfgBoolParm : public CfgBitParm
{
public:
   //  Creates a parameter with the specified attributes.
   //
   CfgBoolParm(const char* key, const char* def, bool* field, const char* expl);

   //  Virtual to allow subclassing.
   //
   virtual ~CfgBoolParm();

   //  Overridden to display member variables.
   //
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   virtual void Patch(sel_t selector, void* arguments) override;
protected:
   //  Returns the the parameter's next value.
   //
   bool GetNextValue() const { return next_; }

   //  Overridden to return the parameter's current value.
   //
   virtual bool GetCurrValue() const override { return *curr_; }

   //  Overridden to transfer next_ to curr_.
   //
   virtual void SetCurr() override;

   //  Overridden to set the parameter's next value.
   //
   virtual bool SetNextValue(bool value) override;
private:
   //  A pointer to the field that contains the parameter's value.
   //
   bool* const curr_;

   //  The value to be set during an appropriate restart.
   //
   bool next_;
};
}
#endif
