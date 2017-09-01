//==============================================================================
//
//  CliText.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef CLITEXT_H_INCLUDED
#define CLITEXT_H_INCLUDED

#include "CliParm.h"
#include <cstddef>
#include "Registry.h"

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
   //  HELP and OPTIONAL are passed to CliParm.  TEXT is the string that can
   //  be followed by parameters.  SIZE specifies the number of parameters
   //  that can follow the string.  If TEXT is EMPTY_STR, then any string
   //  acts as a match (which means that any subsequent instance of CliText
   //  bound against the same CliTextParm cannot be reached and is therefore
   //  useless).  For an example of a CliText instance that uses EMPTY_STR,
   //  see FileText (used by SendWhereParm).
   //
   CliText(const char* help, const char* text,
      bool opt = false, size_t size = 32);

   //  Virtual to allow subclassing.
   //
   virtual ~CliText();

   //  Returns the string.
   //
   const char* Text() const { return text_; }

   //  Returns the string as displayed by the >help command.
   //
   const char* HelpText() const;

   //  Returns the registry of parameters.
   //
   const Registry< CliParm >& Parms() const { return parms_; }

   //  Adds PARM to the list of parameters that can follow the string.
   //
   virtual bool BindParm(CliParm& parm);

   //  Overridden to display member variables.
   //
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   virtual void Patch(sel_t selector, void* arguments) override;
protected:
   //  Returns the registry of parameters.
   //
   Registry< CliParm >& Parms() { return parms_; }

   //  Overridden to access parameters that follow the string.
   //
   virtual CliParm* AccessParm(CliCookie& cookie, size_t depth) const override;

   //  Overridden to display parameters that can follow the string.
   //
   virtual void Explain(std::ostream& stream, col_t indent) const override;

   //  Overridden to look for a text parameter in parms_.
   //
   virtual Rc GetTextParmRc
      (id_t& i, std::string& s, CliThread& cli) const override;
private:
   //  Overridden to display the string as the acceptable input.
   //
   virtual bool ShowValues(std::string& values) const override;

   //  After matching a text string, this function prepares to look
   //  for parameters associated with the string.  If the string has
   //  no parameters, it prepares to look for the next parameter at
   //  the same parse depth.
   //
   void Descend(CliCookie& cookie) const;

   //  The string that that may be followed by parameters.
   //
   const char* text_;

   //  The parameters that may follow the string.
   //
   Registry< CliParm > parms_;
};
}
#endif
