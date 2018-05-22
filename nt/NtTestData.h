//==============================================================================
//
//  NtTestData.h
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
#ifndef NTTESTDATA_H_INCLUDED
#define NTTESTDATA_H_INCLUDED

#include "CliAppData.h"
#include <cstddef>
#include <string>
#include "NbTypes.h"
#include "SysTypes.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace NodeTools
{
//  Data used by the CLI Testcase command and related commands.
//
class NtTestData : public CliAppData
{
public:
   //  Returns the test data registered against CLI.  If the data does not
   //  exist, it is created.
   //
   static NtTestData* Access(CliThread& cli);

   //  Sets the file to be read before executing the >testcase command.
   //
   void SetProlog(const std::string& prolog) { prolog_ = prolog.c_str(); }

   //  Sets the file to be read after a testcase passes.
   //
   void SetEpilog(const std::string& epilog) { epilog_ = epilog.c_str(); }

   //  Sets the file to be read after a testcase fails.
   //
   void SetRecover(const std::string& recover) { recover_ = recover.c_str(); }

   //  Initiates the testcase named TEST.  This name is saved in the symbol
   //  "testcase.name" for use in prolog and epilog command files (see below).
   //  Returns 0.
   //
   word Initiate(const std::string& test);

   //  Concludes a testcase by invoking the script defined by SetEpilog or
   //  SetRecover.
   //
   void Conclude();

   //  Invoked to report a testcase failure.  Invokes CliThread.Report with
   //  RC and a string that includes EXPL.  Returns RC.
   //
   word SetFailed(word rc, const std::string& expl);

   //  Displays testcase statistics.
   //
   void Query() const;

   //  Resets the test environment by deleting the test data, which is
   //  recreated by Access before running another series of testcases.
   //
   void Reset();

   //  Overridden to display member variables.
   //
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;
private:
   //  Private to restrict creation to the Access function.
   //
   explicit NtTestData(CliThread& cli);

   //  Private to restrict deletion.  Not subclassed.
   //
   ~NtTestData();

   //  The file to be read before executing the >testcase command.
   //
   TempString prolog_;

   //  The file to be read after a testcase passes.
   //
   TempString epilog_;

   //  The file to be read after a testcase fails.
   //
   TempString recover_;

   //  The test currently being executed.
   //
   TempString name_;

   //  Set if the current testcase failed.
   //
   bool failed_;

   //  The number of testcases that passed.
   //
   size_t passCount_;

   //  The number of testcases that failed.
   //
   size_t failCount_;
};
}
#endif
