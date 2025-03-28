//==============================================================================
//
//  FileThread.h
//
//  Copyright (C) 2013-2025  Greg Utas
//
//  This file is part of the Robust Services Core (RSC).
//
//  RSC is free software: you can redistribute it and/or modify it under the
//  terms of the Lesser GNU General Public License as published by the Free
//  Software Foundation, either version 3 of the License, or (at your option)
//  any later version.
//
//  RSC is distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
//  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
//  details.
//
//  You should have received a copy of the Lesser GNU General Public License
//  along with RSC.  If not, see <http://www.gnu.org/licenses/>.
//
#ifndef FILETHREAD_H_INCLUDED
#define FILETHREAD_H_INCLUDED

#include "Thread.h"
#include <string>
#include "CallbackRequest.h"
#include "NbTypes.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Thread for file output.  All threads use this, as it will eventually
//  support sending files to a remote location.
//
class FileThread : public Thread
{
   friend class Singleton<FileThread>;
public:
   //  Creates a stream where output can be directed and eventually passed
   //  to our Spool function.
   //
   static ostringstreamPtr CreateStream();

   //  Queues STREAM for output to the file identified by NAME.  If TRUNC
   //  is set, an existing file with NAME is overwritten; otherwise, STREAM
   //  is appended to it.  When STREAM has been written, WRITTEN is invoked.
   //  FileThread assumes ownership of STREAM and WRITTEN, which are set to
   //  nullptr before returning.
   //
   static void Spool(const std::string& name, ostringstreamPtr& stream,
      CallbackRequestPtr& written, bool trunc = false);

   //  The same as Spool (above), but without a callback.
   //
   static void Spool(const std::string& name, ostringstreamPtr& stream,
      bool trunc = false);

   //  Outputs STR to the file identified by NAME.  Adds a CRLF if EOL is set.
   //
   static void Spool(const std::string& name,
      const std::string& str, bool eol = false);

   //  Outputs S to the console transcript file.  Adds a CRLF if EOL is set.
   //
   static void Record(const std::string& s, bool eol = false);

   //  Clears the contents of the file identified by NAME.
   //
   static void Truncate(const std::string& name);

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
private:
   //  Private because this is a singleton.
   //
   FileThread();

   //  Private because this is a singleton.
   //
   ~FileThread();

   //  Overridden to return a name for the thread.
   //
   c_string AbbrName() const override;

   //  Overridden to delete the singleton.
   //
   void Destroy() override;

   //  Overridden to dequeue file output requests.
   //
   void Enter() override;
};
}
#endif
