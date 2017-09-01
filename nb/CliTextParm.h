//==============================================================================
//
//  CliTextParm.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef CLITEXTPARM_H_INCLUDED
#define CLITEXTPARM_H_INCLUDED

#include "CliParm.h"
#include <cstddef>
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
   //  HELP and OPT are passed to CliParm.  SIZE specifies the number of
   //  strings in the list of valid inputs.  A SIZE of zero means that an
   //  arbitrary string may be entered.
   //
   explicit CliTextParm(const char* help, bool opt = false,
      size_t size = 32, const char* tag = nullptr);

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
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   virtual void Patch(sel_t selector, void* arguments) override;
protected:
   //  Overridden to access parameters associated with a specific string.
   //
   virtual CliParm* AccessParm(CliCookie& cookie, size_t depth) const override;

   //  Overridden to show the strings that are acceptable inputs.
   //
   virtual void Explain(std::ostream& stream, col_t indent) const override;

   //  Overridden to look for a string from strings_.
   //
   virtual Rc GetTextParmRc
      (id_t& i, std::string& s, CliThread& cli) const override;

   //  Overridden to look for an arbitrary string.
   //
   virtual Rc GetStringRc(std::string& s, CliThread& cli) const override;

   //  Overridden to look for a filename.
   //
   virtual Rc GetFileNameRc(std::string& s, CliThread& cli) const override;

   //  Overridden to look for an identifier.
   //
   virtual Rc GetIdentifierRc(std::string& s, CliThread& cli,
      const std::string& valid, const std::string& exclude) const override;
private:
   //  Overridden to show the strings that are acceptable inputs.
   //
   virtual bool ShowValues(std::string& values) const override;

   //  Used while parsing a command.  INDEX is the offset within
   //  strings_ where a valid string was found.
   //
   void Descend(CliCookie& cookie, size_t index) const;

   //  The strings that are legal for the text parameter.
   //
   Registry< CliText > strings_;
};
}
#endif
