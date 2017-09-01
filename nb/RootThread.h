//==============================================================================
//
//  RootThread.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef ROOTTHREAD_H_INCLUDED
#define ROOTTHREAD_H_INCLUDED

#include "Thread.h"
#include "NbTypes.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  The root thread--the one created to invoke main()--is responsible for
//  o creating InitThread and the minimal set of objects required for
//    InitThread to finish initializing the system,
//  o ensuring that InitThread finishes initializing the system, and
//  o ensuring that InitThread is running while the system is in service.
//
class RootThread : public Thread
{
   friend class Singleton< RootThread >;
   friend class InitThread;
public:
   //  Invoked as the only line of code in main().
   //
   static main_t Main();

   //  Overridden to display member variables.
   //
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   virtual void Patch(sel_t selector, void* arguments) override;
private:
   //  Private because this singleton is not subclassed.
   //
   RootThread();

   //  Private because this singleton is not subclassed.
   //
   ~RootThread();

   //  States for the root thread.
   //
   enum State
   {
      Initializing,  // system being initialized
      Running        // system is in service
   };

   //  Overridden to return a name for the thread.
   //
   virtual const char* AbbrName() const override;

   //  Overridden to create InitThread, to ensure that InitThread finishes
   //  initializing the system, and to ensure that InitThread subsequently
   //  runs periodically.  This is indirectly invoked by our Main function,
   //  via Thread::Start.
   //
   virtual void Enter() override;

   //  Overridden to delete the singleton.
   //
   virtual void Destroy() override;

   //  The thread's current state.
   //
   State state_;
};
}
#endif
