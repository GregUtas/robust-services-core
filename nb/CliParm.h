//==============================================================================
//
//  CliParm.h
//
//  Copyright (C) 2013-2022  Greg Utas
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
#ifndef CLIPARM_H_INCLUDED
#define CLIPARM_H_INCLUDED

#include "Immutable.h"
#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <string>
#include "RegCell.h"
#include "SysTypes.h"

namespace NodeBase
{
   class CliCookie;
   class CliThread;
}

//------------------------------------------------------------------------------
//
//  The Command Line Interface (CLI) allows "increments" (applications) to
//  register so that they can be invoked from an input stream.  The CLI reads
//  input from the stream and passes it to the appropriate increment.  Each
//  increment defines commands that may also take parameters.  Because each
//  increment, command, and parameter derives from a common set of CLI base
//  classes, all CLI applications behave in a similar way.
//
//  When a user inputs an increment's name, the CLI enters that increment.  The
//  increment's commands then become available in the foreground.  For example:
//
//    >sb         // enters the SessionBase increment
//    >protocols  // lists all protocols supported by SessionBase applications
//
//  If an increment has not been entered, its commands can nevertheless be
//  accessed by prefixing them with the increment's name.  For example:
//
//    >sb protocols  // does not remain in the SessionBase increment
//
//  The CLI itself provides some basic commands in the NodeBase increment,
//  whose commands always remain available:
//
//    >help                // lists commands supported by the CLI
//    >help <incr>         // lists commands supported by an increment
//    >help <incr> <comm>  // provides help on an increment's command
//    >help <comm>         // same as help <incr> <comm> if already in <incr>
//    >incrs               // lists all registered increments
//    >send <file>         // sends output to file.txt ("cout" = to console)
//    >quit                // exits the most recently entered increment
//    >quit all            // exits all increments
//
//  To match a command name or parameter value, CLI input must generally use
//  the same case (upper or lower).
//
//  In the following list, classes marked with a '*' are subclassed (or used
//  directly) to create application-specific CLI objects.
//
// *CliIncrement:    Subclassed to define an increment
// *CliCommand:      Subclassed by each of an increment's commands
// *CliCommandSet:   Subclassed to group commands under an "umbrella" command
//  CliParm:         A command takes zero or more parameters.  Subclasses of
//                   CliParm define actual parameter types:
//    *CliText:      A string that is followed by zero or more parameters.
//                   Subclassed to define CliCommand and the specific strings
//                   that a CliTextParm accepts.
//    *CliIntParm:   An integer parameter
//    *CliBoolParm:  A boolean parameter
//    *CliCharParm:  A character parameter
//    *CliPtrParm:   A pointer parameter
//    *CliTextParm:  A text parameter (any string, or one specified in a list)
//  CliRegistry:     Registry for increments
//  CliStack:        Tracks active increments
//  CliBuffer:       Reads and parses user input
//  CliCookie:       Tracks the current location in the "tree" of parameters
//  CliThread:       Reads user input and invokes the appropriate increment
// *CliAppData:      Provides application-specific storage on CliThread
//  Symbol:          A string that may be used instead of a CLI parameter's
//                   value; often a mnemonic for an integer constant
//  SymbolRegistry:  Registry for symbols
//
namespace NodeBase
{
//  Virtual base class for CLI parameters.
//
class CliParm : public Immutable
{
public:
   //  The outcome of looking for a parameter when parsing the command line.
   //
   enum Rc
   {
      Ok,     // parameter found
      None,   // optional parameter not found
      Error,  // mandatory parameter not found
      Skip    // used internally only: skip optional parameter
   };

   //  Deleted to prohibit copying.
   //
   CliParm(const CliParm& that) = delete;

   //  Deleted to prohibit copy assignment.
   //
   CliParm& operator=(const CliParm& that) = delete;

   //  Precedes explanatory text.
   //
   static fixed_string ParmExplPrefix;

   //  Indicates that any string will match a parameter.
   //
   static fixed_string AnyStringParm;

   //  Virtual to allow subclassing.
   //
   virtual ~CliParm();

   //  Used by the CLI to access parameters when parsing a command.  Only
   //  strings can take parameters; booleans, integers, characters, and
   //  pointers are leaves in a parse tree.  The default version returns
   //  nullptr and must be overridden by subclasses associated with strings.
   //  COOKIE and DEPTH identify the current location in the parse tree.
   //
   virtual CliParm* AccessParm(CliCookie& cookie, uint32_t depth) const;

   //  Used by the >help command to provide information about this object.
   //  STREAM is where to output the information, and INDENT is how far to
   //  indent it.  The default version invokes ShowValues to obtain the
   //  parameter's legal values, which it displays as mandatory or optional,
   //  followed by the string the explains the parameter.
   //
   virtual void Explain(std::ostream& stream, col_t indent) const;

   //  Returns the next parameter in N if it is an integer.  Used when
   //  an integer parameter is mandatory.
   //
   bool GetIntParm(word& n, CliThread& cli) const;

   //  Returns the next parameter in N if it is an integer.  Used when
   //  an integer parameter is optional.
   //
   virtual Rc GetIntParmRc(word& n, CliThread& cli) const;

   //  Returns the next parameter in B if it is a boolean.  Used when a
   //  boolean parameter is mandatory.
   //
   bool GetBoolParm(bool& b, CliThread& cli) const;

   //  Returns the next parameter in B if it is a boolean.  Used when a
   //  boolean parameter is optional.
   //
   virtual Rc GetBoolParmRc(bool& b, CliThread& cli) const;

   //  Returns the next parameter in C if it is character in a specified
   //  list.  Used when a character parameter is mandatory.
   //
   bool GetCharParm(char& c, CliThread& cli) const;

   //  Returns the next parameter in C if it is character in a specified
   //  list.  Used when a character parameter is optional.
   //
   virtual Rc GetCharParmRc(char& c, CliThread& cli) const;

   //  Returns the next parameter in P if it is a pointer.  Used when a
   //  pointer parameter is mandatory.
   //
   bool GetPtrParm(void*& p, CliThread& cli) const;

   //  Returns the next parameter in P if it is a pointer.  Used when a
   //  pointer parameter is optional.
   //
   virtual Rc GetPtrParmRc(void*& p, CliThread& cli) const;

   //  Returns the next parameter if it is a string in a specified list.
   //  The string itself is not returned, but its index (into the list)
   //  is returned in I.  Used when the string parameter is mandatory
   //  and must match one in a set of fixed strings.
   //
   bool GetTextIndex(id_t& i, CliThread& cli) const;

   //  Returns the next parameter if it is a string in a specified list.
   //  The string itself is not returned, but its index (into the list)
   //  is returned in I.  Used when the string parameter is optional but
   //  must match one in a set of fixed strings.
   //
   Rc GetTextIndexRc(id_t& i, CliThread& cli) const;

   //  Returns the next parameter if it is a string in a specified list.
   //  The string is also returned in S, and its index (into the list)
   //  is provided in I.  Used when the string parameter is mandatory
   //  and can match either a fixed string or be an arbitrary string.
   //
   bool GetTextParm(id_t& i, std::string& s, CliThread& cli) const;

   //  Returns the next parameter if it is a string in a specified list.
   //  The string is returned in S, and its index (into the list) is
   //  returned in I.  Used when the string parameter is optional and
   //  can match either a fixed string or be an arbitrary string.
   //
   virtual Rc GetTextParmRc(id_t& i, std::string& s, CliThread& cli) const;

   //  Returns the next parameter in S if it is an arbitrary string.
   //  Used when a string parameter is mandatory.
   //
   bool GetString(std::string& s, CliThread& cli) const;

   //  Returns the next parameter in S if it is an arbitrary string.
   //  Used when a string parameter is optional.
   //
   virtual Rc GetStringRc(std::string& s, CliThread& cli) const;

   //  Returns the next parameter in S if it is a valid filename.
   //  Used when a filename is mandatory.
   //
   bool GetFileName(std::string& s, CliThread& cli) const;

   //  Returns the next parameter in S if it is a valid filename.
   //  Used when a filename is optional.
   //
   virtual Rc GetFileNameRc(std::string& s, CliThread& cli) const;

   //  Returns the next parameter in S if it is an identifier.  VALID
   //  specifies legal characters for the identifier, which must also
   //  not begin with one of the characters in EXCLUDE.  Used when an
   //  identifier is mandatory.
   //
   bool GetIdentifier(std::string& s, CliThread& cli,
      const std::string& valid, const std::string& exclude = EMPTY_STR) const;

   //  Returns the next parameter in S if it is an identifier.  Used
   //  when an identifier is optional.
   //
   virtual Rc GetIdentifierRc(std::string& s, CliThread& cli,
      const std::string& valid, const std::string& exclude) const;

   //  Returns the string that explains a parameter.
   //
   c_string Help() const { return help_; }

   //  Indicates whether the parameter is optional or mandatory.
   //
   bool IsOptional() const { return opt_; }

   //  Returns the tag (if any) for an optional parameter.
   //
   c_string Tag() const { return tag_; }

   //  Sets the registry index where the parameter was placed (or where
   //  it *must* be placed, if specified before it is registered).
   //
   void SetId(id_t id) { pid_.SetId(id); }

   //  Returns the registry index assigned to the parameter.
   //
   id_t GetId() const { return pid_.GetId(); }

   //  Returns the offset to pid_.
   //
   static ptrdiff_t CellDiff();

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
protected:
   //  Returns true if it is OK to invoke CliCookie.Ascend, which is
   //  equivalent to backing up to this item and continuing down the
   //  parse tree when looking for the next parameter.
   //
   virtual bool Ascend() const { return true; }

   //> Field width for parameters in help text.
   //
   static const col_t ParmWidth;

   //  Prefix for a mandatory parameter in help text.
   //
   static const char MandParmBegin;

   //  Suffix for a mandatory parameter in help text.
   //
   static const char MandParmEnd;

   //  Prefix for an optional parameter in help text.
   //
   static const char OptParmBegin;

   //  Suffix for an optional parameter in help text.
   //
   static const char OptParmEnd;

   //  HELP explains the parameter's purpose.  OPT indicates whether it is
   //  mandatory or optional.  TAG may be used for an optional parameter:
   //  it allows the parameter to appear in any order, tagged with "<tag>="
   //  (unquoted).  Protected because this class is virtual.
   //
   CliParm(c_string help, bool opt, c_string tag);
private:
   //  Updates VALUES to show legal inputs for the parameter.  Returns true
   //  if ParmMandBegin and ParmMandEnd should be included before and after
   //  VALUES when they are displayed in help text.
   //
   virtual bool ShowValues(std::string& values) const = 0;

   //  The parameter's index in the instance of CliText.parms_ where it
   //  is registered.
   //
   RegCell pid_;

   //  A string that describes the parameter's purpose.
   //
   fixed_string help_;

   //  Whether the parameter is mandatory or optional.
   //
   const bool opt_;

   //  The tag, if any, for an optional parameter.
   //
   fixed_string tag_;
};
}
#endif
