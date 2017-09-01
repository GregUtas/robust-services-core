//==============================================================================
//
//  CfgFlagParm.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef CFGFLAGPARM_H_INCLUDED
#define CFGFLAGPARM_H_INCLUDED

#include "CfgBitParm.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Configuration parameter for flags.
//
class CfgFlagParm : public CfgBitParm
{
public:
   //  Creates a parameter with the specified attributes.
   //
   CfgFlagParm(const char* key, const char* def,
      Flags* field, FlagId fid, const char* expl);

   //  Virtual to allow subclassing.
   //
   virtual ~CfgFlagParm();

   //  Overridden to display member variables.
   //
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   virtual void Patch(sel_t selector, void* arguments) override;
protected:
   //  Overridden to return the parameter's current value.
   //
   virtual bool GetCurrValue() const override;

   //  Overridden to transfer next_ to curr_.
   //
   virtual void SetCurr() override;

   //  Overridden to set the parameter's next value.
   //
   virtual bool SetNextValue(bool value) override;
private:
   //  A pointer to the object that contains the flag's value.
   //
   Flags* curr_;

   //  The value to be set during an appropriate restart.
   //
   bool next_;

   //  The flag's identifier.
   //
   FlagId fid_;
};
}
#endif
