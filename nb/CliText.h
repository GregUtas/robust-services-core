//==============================================================================
//
//  CliText.h
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
#ifndef CLITEXT_H_INCLUDED
#define CLITEXT_H_INCLUDED

#include "CliParm.h"
#include <cstdint>
#include "Registry.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Defines a string (such as a command) that may take additional parameters.
//  Each instance is added to a CliTextParm by invoking CliTextParm.BindText.
//
class CliText : public CliParm
{
   friend class CliTextParm;
public:
   //  HELP and OPT are passed to CliParm.  TEXT is the string that can
   //  be followed by parameters.  SIZE specifies the number of parameters
   //  that can follow the string.  If TEXT is EMPTY_STR, then any string
   //  acts as a match (which means that any subsequent instance of CliText
   //  bound against the same CliTextParm cannot be reached and is therefore
   //  useless).  For an example of a CliText instance that uses EMPTY_STR,
   //  see FileText (used by SendWhereParm).
   //
   CliText(c_string help, c_string text, bool opt = false, uint32_t size = 32);

   //  Virtual to allow subclassing.
   //
   virtual ~CliText();

   //  Returns the string.
   //
   c_string Text() const { return text_; }

   //  Returns the string as displayed by the >help command.
   //
   c_string HelpText() const;

   //  Returns the registry of parameters.
   //
   const Registry< CliParm >& Parms() const { return parms_; }

   //  Adds PARM to the list of parameters that can follow the string.
   //
   virtual bool BindParm(CliParm& parm);

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
protected:
   //  Returns the registry of parameters.
   //
   Registry< CliParm >& Parms() { return parms_; }

   //  Overridden to access parameters that follow the string.
   //
   CliParm* AccessParm(CliCookie& cookie, uint32_t depth) const override;

   //  Overridden to display parameters that can follow the string.
   //
   void Explain(std::ostream& stream, col_t indent) const override;

   //  Overridden to look for a text parameter in parms_.
   //
   Rc GetTextParmRc(id_t& i, std::string& s, CliThread& cli) const override;
private:
   //  Overridden to display the string as the acceptable input.
   //
   bool ShowValues(std::string& values) const override;

   //  After matching a text string, this function prepares to look
   //  for parameters associated with the string.  If the string has
   //  no parameters, it prepares to look for the next parameter at
   //  the same parse depth.
   //
   void Descend(CliCookie& cookie) const;

   //  The string that that may be followed by parameters.
   //
   fixed_string text_;

   //  The parameters that may follow the string.
   //
   Registry< CliParm > parms_;
};
}
#endif
