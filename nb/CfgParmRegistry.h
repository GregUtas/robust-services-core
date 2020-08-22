//==============================================================================
//
//  CfgParmRegistry.h
//
//  Copyright (C) 2013-2020  Greg Utas
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
#ifndef CFGPARMREGISTRY_H_INCLUDED
#define CFGPARMREGISTRY_H_INCLUDED

#include "Protected.h"
#include <iosfwd>
#include <string>
#include "NbTypes.h"
#include "Q1Way.h"

namespace NodeBase
{
   class CfgParm;
   class CfgTuple;
}

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Global registry for configuration parameters.
//
class CfgParmRegistry : public Protected
{
   friend class Singleton< CfgParmRegistry >;
   friend class CfgParm;
   friend class CfgTuple;
public:
   //  Deleted to prohibit copying.
   //
   CfgParmRegistry(const CfgParmRegistry& that) = delete;
   CfgParmRegistry& operator=(const CfgParmRegistry& that) = delete;

   //  Searches the registry and returns a tuple that matches KEY.
   //  Returns nullptr if no such tuple exists.
   //
   CfgTuple* FindTuple(const std::string& key) const;

   //  Adds PARM to the registry and sets its value from the tuple
   //  that is associated with it.
   //
   bool BindParm(CfgParm& parm);

   //  Searches the registry and returns a configuration parameter
   //  that matches KEY.  Returns nullptr if no such parameter exists.
   //
   CfgParm* FindParm(const std::string& key) const;

   //  Searches the registry for a configuration parameter that
   //  matches KEY.  If such a parameter exists, updates VALUE to
   //  the string associated with the parameter's current value.
   //
   bool GetValue(const std::string& key, std::string& value) const;

   //  Displays each parameter in the registry, along with its value.
   //
   void ListParms(std::ostream& stream, const std::string& prefix) const;

   //  Overridden for restarts.
   //
   void Startup(RestartLevel level) override;

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
private:
   //  Private because this singleton is not subclassed.
   //
   CfgParmRegistry();

   //  Private because this singleton is not subclassed.
   //
   ~CfgParmRegistry();

   //  Adds TUPLE to the registry.
   //
   bool BindTuple(CfgTuple& tuple);

   //  Removes TUPLE from the registry.
   //
   void UnbindTuple(CfgTuple& tuple);

   //  Removes PARM from the registry.
   //
   void UnbindParm(CfgParm& parm);

   //  Reads configuration tuples (key-value pairs) from the configuration
   //  file during system initialization.  Creates a CfgTuple instance for
   //  each valid tuple and adds it to the registry.
   //
   void LoadTuples();

   //  Called by LoadTuples to read the next tuple from the configuration
   //  file.    Returns true if another valid tuple exists; updates KEY
   //  and VALUE accordingly.  Returns false on EOF.  Calls BadLine to log
   //  invalid entries, but continues to look for tuples.
   //
   bool LoadNextTuple(std::string& key, std::string& value);

   //  The file from which tuples are read during system initialization.
   //
   ProtectedStr configFileName_;

   //  The tuples (key-value pairs) in the registry.  They are kept
   //  in a queue that is sorted in alphabetical order, by key.
   //
   Q1Way< CfgTuple > tupleq_;

   //  The configuration parameters in the registry.  They are kept
   //  in a queue that is sorted in alphabetical order.
   //
   Q1Way< CfgParm > parmq_;
};
}
#endif
