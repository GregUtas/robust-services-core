//==============================================================================
//
//  CinThread.h
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
#ifndef CINTHREAD_H_INCLUDED
#define CINTHREAD_H_INCLUDED

#include "Thread.h"
#include <ios>
#include "NbTypes.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Thread for console input.  This will eventually evolve to support
//  a remote console that sends input via an IP port.
//
class CinThread : public Thread
{
   friend class Singleton< CinThread >;
public:
   //  Reads input from the console and places it in BUFF, which can hold up
   //  to CAPACITY characters.  Returns the number of characters read (N >= 1),
   //  excluding the trailing '\0' that is also placed in BUFF.  If N <= 0, see
   //  StreamRc.  The client thread is only scheduled out if input is not yet
   //  available, so it must not call EnterBlockingOperation before it invokes
   //  this function.
   //
   static std::streamsize GetLine(char* buff, std::streamsize capacity);

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
private:
   //> The size of the console input buffer.
   //
   static const std::streamsize BuffSize = 2 * COUT_LENGTH_MAX;

   //  Private because this singleton is not subclassed.
   //
   CinThread();

   //  Private because this singleton is not subclassed.
   //
   ~CinThread();

   //  Registers CLIENT as waiting for input.  Returns false if another
   //  client is currenty registered.
   //
   bool SetClient(Thread* client);

   //  Overridden to return a name for the thread.
   //
   const char* AbbrName() const override;

   //  Overridden to read input from the console and either buffer it
   //  or pass it to a waiting thread.
   //
   void Enter() override;

   //  Overridden to delete the singleton.
   //
   void Destroy() override;

   //  Buffer for input.
   //
   char buff_[BuffSize];

   //  The number of characters in buff_.
   //
   std::streamsize size_;

   //  The thread that is waiting for input.
   //
   Thread* client_;
};
}
#endif
