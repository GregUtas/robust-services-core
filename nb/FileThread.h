//==============================================================================
//
//  FileThread.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef FILETHREAD_H_INCLUDED
#define FILETHREAD_H_INCLUDED

#include "Thread.h"
#include <string>
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
   friend class Singleton< FileThread >;
public:
   //  Creates a stream where output can be directed and eventually passed
   //  to our Spool function.
   //
   static ostringstreamPtr CreateStream();

   //  Queues STREAM for output to the file identified by NAME.  If TRUNC
   //  is set, an existing file with NAME is overwritten; otherwise, STREAM
   //  is appended to it.  FileThread assumes ownership of STREAM, which is
   //  is set to nullptr before returning.
   //
   static void Spool(const std::string& name,
      ostringstreamPtr& stream, bool trunc = false);

   //  Outputs STR to the file identified by NAME.  Adds an "CRLF" if EOL
   //  is set.
   //
   static void Spool(const std::string& name,
      const std::string& s, bool eol = false);

   //  Clears the contents of the file identified by NAME.
   //
   static void Truncate(const std::string& name);

   //  Overridden for patching.
   //
   virtual void Patch(sel_t selector, void* arguments) override;
private:
   //  Private because this singleton is not subclassed.
   //
   FileThread();

   //  Private because this singleton is not subclassed.
   //
   ~FileThread();

   //  Overridden to return a name for the thread.
   //
   virtual const char* AbbrName() const override;

   //  Overridden to dequeue file output requests.
   //
   virtual void Enter() override;

   //  Overridden to delete the singleton.
   //
   virtual void Destroy() override;
};
}
#endif
