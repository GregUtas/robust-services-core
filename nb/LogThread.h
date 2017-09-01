//==============================================================================
//
//  LogThread.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef LOGTHREAD_H_INCLUDED
#define LOGTHREAD_H_INCLUDED

#include "Thread.h"
#include "NbTypes.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Thread for spooling logs.
//
class LogThread : public Thread
{
   friend class Singleton< LogThread >;
   friend class Log;
public:
   //  Overridden for patching.
   //
   virtual void Patch(sel_t selector, void* arguments) override;
private:
   //  Private because this singleton is not subclassed.
   //
   LogThread();

   //  Private because this singleton is not subclassed.
   //
   ~LogThread();

   //  Writes LOG to the log file and, if appropriate, the console.  LogThread
   //  takes ownership of LOG, which is set to nullptr before returning.
   //
   static void Spool(ostringstreamPtr& log);

   //  Overridden to return a name for the thread.
   //
   virtual const char* AbbrName() const override;

   //  Overridden to dequeue log requests.
   //
   virtual void Enter() override;

   //  Overridden to delete the singleton.
   //
   virtual void Destroy() override;
};
}
#endif
