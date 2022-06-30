//==============================================================================
//
//  MainArgs.h
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
#ifndef MAINARGS_H_INCLUDED
#define MAINARGS_H_INCLUDED

#include "Immutable.h"
#include <cstddef>
#include <string>
#include <vector>
#include "Allocators.h"
#include "NbTypes.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  For saving and accessing the command line parameters to main().
//
class MainArgs : public Immutable
{
   friend class Singleton<MainArgs>;
public:
   //  Deleted to prohibit copying.
   //
   MainArgs(const MainArgs& that) = delete;

   //  Deleted to prohibit copy assignment.
   //
   MainArgs& operator=(const MainArgs& that) = delete;

   //  Echoes main()'s arguments to the console and saves them.
   //  ARGC and ARGV are the arguments to main().
   //
   static void EchoAndSaveArgs(int argc, char* argv[]);

   //  Returns the number of arguments that were passed to main().
   //
   static size_t Size();

   //  Returns the Nth argument that was passed to main().
   //
   static c_string At(size_t n);

   //  Adds the next argument that was passed to main().
   //
   static void PushBack(const std::string& arg);

   //  Looks for an argument that begins with TAG.  If one is found,
   //  returns the string that follows TAG, else returns EMPTY_STR.
   //
   static std::string Find(c_string tag);

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
private:
   //  Private because this is a singleton.
   //
   MainArgs();

   //  Private because this is a singleton.
   //
   ~MainArgs();

   //  The arguments to main().
   //
   std::vector<ImmutableStr, ImmutableAllocator<ImmutableStr>> args_;
};
}
#endif
