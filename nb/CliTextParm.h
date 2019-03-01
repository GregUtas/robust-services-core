//==============================================================================
//
//  CliTextParm.h
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
#ifndef CLITEXTPARM_H_INCLUDED
#define CLITEXTPARM_H_INCLUDED

#include "CliParm.h"
#include <cstdint>
#include "Registry.h"
#include "SysTypes.h"

namespace NodeBase
{
   class CliText;
}

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Defines a parameter that takes any string as a parameter, or a specific
//  string (a subclass of CliText) in a list created by BindText.
//
class CliTextParm : public CliParm
{
public:
   //  HELP, OPT, and TAG are passed to CliParm.  SIZE specifies the number
   //  of strings in the list of valid inputs.  A SIZE of zero means that an
   //  arbitrary string may be entered.
   //
   explicit CliTextParm(const char* help, bool opt = false,
      uint32_t size = 32, const char* tag = nullptr);

   //  Virtual to allow subclassing.
   //
   virtual ~CliTextParm();

   //  Adds TEXT as one of the acceptable strings for the text parameter.
   //  TEXT is added at [INDEX], which GetTextParmRc (see below) returns
   //  to identify the string.
   //
   bool BindText(CliText& text, id_t index);

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
protected:
   //  Overridden to access parameters associated with a specific string.
   //
   CliParm* AccessParm(CliCookie& cookie, uint32_t depth) const override;

   //  Overridden to show the strings that are acceptable inputs.
   //
   void Explain(std::ostream& stream, col_t indent) const override;

   //  Overridden to look for a string from strings_.
   //
   Rc GetTextParmRc(id_t& i, std::string& s, CliThread& cli) const override;

   //  Overridden to look for an arbitrary string.
   //
   Rc GetStringRc(std::string& s, CliThread& cli) const override;

   //  Overridden to look for a filename.
   //
   Rc GetFileNameRc(std::string& s, CliThread& cli) const override;

   //  Overridden to look for an identifier.
   //
   Rc GetIdentifierRc(std::string& s, CliThread& cli,
      const std::string& valid, const std::string& exclude) const override;
private:
   //  Overridden to show the strings that are acceptable inputs.
   //
   bool ShowValues(std::string& values) const override;

   //  Used while parsing a command.  INDEX is the offset within
   //  strings_ where a valid string was found.
   //
   void Descend(CliCookie& cookie, uint32_t index) const;

   //  The strings that are legal for the text parameter.
   //
   Registry< CliText > strings_;
};
}
#endif
