//==============================================================================
//
//  CfgParm.h
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
#ifndef CFGPARM_H_INCLUDED
#define CFGPARM_H_INCLUDED

#include "Persistent.h"
#include <cstddef>
#include <string>
#include "Q1Link.h"
#include "SysTypes.h"

namespace NodeBase
{
   class CfgTuple;
}

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Virtual class for configuration parameters.  After an application creates
//  a configuration parameter, it must call CfgParmRegistry::BindParm to add it
//  to the registry.  Each parameter subclass has a field_ member that points
//  to the parameter's value.  This pointer is based on the parameter's type
//  (e.g. bool, int, or string).  When the parameter is created, the value is
//  left unchanged from what was assigned during system initialization.  When
//  the parameter is registered, its value (the field) is updated to what (if
//  anything) was specified in the element configuration file.
//
class CfgParm : public Persistent
{
   friend class CfgParmRegistry;
public:
   //  Removes the parameter from CfgParmRegistry.  Virtual to allow
   //  subclassing.
   //
   virtual ~CfgParm();

   //  Deleted to prohibit copying.
   //
   CfgParm(const CfgParm& that) = delete;
   CfgParm& operator=(const CfgParm& that) = delete;

   //  Returns the parameter's name (its tuple's key).
   //
   c_string Key() const;

   //  Updates EXPL to explain the parmeter's purpose.
   //
   virtual void Explain(std::string& expl) const { expl = expl_; }

   //  Sets the parameter's value based on INPUT.  Returns false if INPUT
   //  was invalid; otherwise, returns true and updates LEVEL to the type
   //  of restart needed to make the change.  If LFVEL is RestartNil, the
   //  new value is already in effect, and no restart is required.
   //
   bool SetValue(const std::string& input, RestartLevel& level);

   //  Returns the offset to link_.
   //
   static ptrdiff_t LinkDiff();

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
protected:
   //  Searches CfgParmRegistry for a tuple with KEY.  If one doesn't exist,
   //  KEY and DEF (the default value) are used to create a new tuple and add
   //  it to the registry.  DEF and EXPL are also saved as default_ and expl_.
   //
   CfgParm(c_string key, c_string def, c_string expl);

   //  A subclass must override this to return a string that corresponds
   //  to the parameter's value.  If that string were passed to SetNext,
   //  it would set the parameter's next value to its current value.
   //
   virtual std::string GetCurr() const = 0;

   //  A subclass must override this to transfer the value of its next_ field
   //  into its subclass-specific curr_ field, which is usually a pointer to
   //  the parameter's current value.  After doing this, it must invoke this
   //  base class version to save the current value in the parameter's tuple.
   //
   virtual void SetCurr();
private:
   //  Returns a string that can be saved in the parameter's tuple so that the
   //  tuple can recreate the parameter.  The default version returns GetCurr()
   //  and is overridden by a parameter that uses its tuple as a raw input for
   //  constructing a different value.
   //
   virtual std::string GetInput() const;

   //  A subclass must override this to set the future value of the parameter
   //  parameter based on INPUT.  If INPUT is valid, it returns true and sets
   //  a subclass-specific next_ field to the future value.  If INPUT is not
   //  valid, it returns false and does nothing.
   //
   virtual bool SetNext(const std::string& input) = 0;

   //  A subclass may override this to specify the level of restart that is
   //  needed to invoke SetCurr (that is, to modify its parameter's value).
   //  The default, RestartNil, indicates that the value can be modified
   //  while the system is in service.
   //
   virtual RestartLevel RestartRequired() const { return RestartNil; }

   //  Sets the parameter to the value specified in its tuple.  If that value
   //  is invalid, the parameter's default value is retained and the value in
   //  the tuple is changed to the default.
   //
   bool SetFromTuple();

   //  The parameter's tuple (its key and the string used to set its value).
   //
   CfgTuple* tuple_;

   //  A string that sets the parameter to its default value.
   //
   fixed_string default_;

   //  A string that explains the parameter's type and purpose.
   //
   fixed_string expl_;

   //  The level of restart required to set the parameter to a pending value.
   //
   RestartLevel level_;

   //  The next parameter in CfgParmRegistry.
   //
   Q1Link link_;
};
}
#endif
