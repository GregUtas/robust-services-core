//==============================================================================
//
//  CfgTuple.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef CFGTUPLE_H_INCLUDED
#define CFGTUPLE_H_INCLUDED

#include "Protected.h"
#include <cstddef>
#include <string>
#include "Q1Link.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  A key-value pair for a configuration parameter.  Applications do not
//  use this class directly.  Instances of it are created when
//  o CfgParmRegistry::LoadTuples reads key-value pairs from the element
//    configuration file during system initialization;
//  o CfgParmRegistry::BindParm adds a parameter to the registry and no
//    tuple for that parameter existed in the element configuration file.
//    In this case, a tuple is created for the parameter, and its value
//    is set to the parameter's default.
//
class CfgTuple : public Protected
{
public:
   //  The character that prefixes comments in the file that contains
   //  element configuration parameters.  This character, and any that
   //  follow it on the same line, are ignored.
   //
   static const char CommentChar;

   //  Sets key_ and input_ from the arguments.
   //
   CfgTuple(const std::string& key, const std::string& input);

   //  Removes the tuple from CfgParmRegistry.  Not subclassed.
   //
   ~CfgTuple();

   //  Returns the tuple's key.
   //
   const std::string& Key() const { return key_; }

   //  Returns the string used to set the parameter's value.
   //
   const std::string& Input() const { return input_; }

   //  Saves the string that would set the parameter to its current value.
   //  Such a string must be available so that it can be written to a file
   //  that can later be read to restore the parameter's current value.
   //
   void SetInput(const std::string& input) { input_ = input; }

   //  Returns a string containing the characters that are valid in the
   //  name of a configuration parameter.
   //
   static const std::string& ValidNameChars();

   //  Returns a string containing the characters that are valid in an
   //  input string that sets the value of a configuration parameter.
   //
   static const std::string& ValidValueChars();

   //  Returns a string containing the characters that are valid blanks
   //  in the file that sets element configuration parameters.
   //
   static const std::string& ValidBlankChars();

   //  Returns the offset to link_.
   //
   static ptrdiff_t LinkDiff();

   //  Overridden to display member variables.
   //
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   virtual void Patch(sel_t selector, void* arguments) override;
private:
   //  Overridden to prohibit copying.
   //
   CfgTuple(const CfgTuple& that);
   void operator=(const CfgTuple& that);

   //  The name of the parameter associated with the tuple.
   //
   const std::string key_;  //r

   //  The string used to set the parameter's value.
   //
   std::string input_;  //r

   //  The next tuple in CfgParmRegistry.
   //
   Q1Link link_;
};
}
#endif
