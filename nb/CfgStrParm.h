//==============================================================================
//
//  CfgStrParm.h
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
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
protected:
   //  Overridden to prefix the parameter's type and allowed values.
   //
   void Explain(std::string& expl) const override;

   //  Overridden to return the parameter's current value.
   //
   std::string GetCurr() const override { return *curr_; }

   //  Overridden to transfer next_ to curr_.
   //
   void SetCurr() override;

   //  Overridden to set the parameter's next value.  May be overridden
   //  to prevent invalid settings.  If so, invoke this version before
   //  returning true.
   //
   bool SetNext(const std::string& input) override;
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
   bool SetNext(const std::string& input) override;
private:
   //  Overridden to return the filename without its time suffix.
   //
   std::string GetInput() const override { return input_; }

   //  Preserves the filename's original value, without the time suffix.
   //
   std::string input_;  //r
};
}
#endif
