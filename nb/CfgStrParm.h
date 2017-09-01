//==============================================================================
//
//  CfgStrParm.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef CFGSTRPARM_H_INCLUDED
#define CFGSTRPARM_H_INCLUDED

#include "CfgParm.h"
#include <string>

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Configuration parameter for string values.
//
class CfgStrParm : public CfgParm
{
public:
   //  Creates a parameter with the specified attributes.
   //
   CfgStrParm(const char* key, const char* def,
      std::string* field, const char* expl);

   //  Virtual to allow subclassing.
   //
   virtual ~CfgStrParm();

   //  Overridden to display member variables.
   //
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   virtual void Patch(sel_t selector, void* arguments) override;
protected:
   //  Overridden to prefix the parameter's type and allowed values.
   //
   virtual void Explain(std::string& expl) const override;

   //  Overridden to return the parameter's current value.
   //
   virtual std::string GetCurr() const override { return *curr_; }

   //  Overridden to transfer next_ to curr_.
   //
   virtual void SetCurr() override;

   //  Overridden to set the parameter's next value.  May be overridden
   //  to prevent invalid settings.  If so, invoke this version before
   //  returning true.
   //
   virtual bool SetNext(const std::string& input) override;
private:
   //  A pointer to the string that contains the parameter's value.
   //
   std::string* curr_;

   //  The value to be set during an appropriate restart.
   //
   std::string next_;  //r
};

//------------------------------------------------------------------------------
//
//  Configuration parameter for a filename that adds a suffix containing
//  the time at which the system was initialized.
//
class CfgFileTimeParm : public CfgStrParm
{
public:
   //  Creates a parameter with the specified attributes.
   //
   CfgFileTimeParm(const char* key, const char* def,
      std::string* field, const char* expl);

   //  Virtual to allow subclassing.
   //
   virtual ~CfgFileTimeParm();
protected:
   //  Overridden to add the time at which the system was initialized.
   //
   virtual bool SetNext(const std::string& input) override;
private:
   //  Overridden to return the filename without its time suffix.
   //
   virtual std::string GetInput() const override { return input_; }

   //  Preserves the filename's original value, without the time suffix.
   //
   std::string input_;  //r
};
}
#endif
