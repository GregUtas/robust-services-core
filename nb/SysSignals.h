//==============================================================================
//
//  SysSignals.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef SYSSIGNALS_H_INCLUDED
#define SYSSIGNALS_H_INCLUDED

#include "PosixSignal.h"
#include "NbTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
class SysSignals
{
public:
   //  Creates native signals during system initialization.
   //
   static void CreateNativeSignals();
private:
   //  Private because this class only has static members.
   //
   SysSignals();

   //  Standard signals.  CreateNativeSignals instantiates a singleton for each
   //  one that this platform supports.  Other signals also exist, but their use
   //  in a server is either dubious or unlikely to be required.
   //
   class SigAbort : public PosixSignal
   {
      friend class Singleton< SigAbort >;
   private:
      SigAbort();
   };

   class SigAlrm : public PosixSignal
   {
      friend class Singleton< SigAlrm >;
   private:
      SigAlrm();
   };

   class SigBreak : public PosixSignal
   {
      friend class Singleton< SigBreak >;
   private:
      SigBreak();
   };

   class SigBus : public PosixSignal
   {
      friend class Singleton< SigBus >;
   private:
      SigBus();
   };

   class SigFpe : public PosixSignal
   {
      friend class Singleton< SigFpe >;
   private:
      SigFpe();
   };

   class SigIll : public PosixSignal
   {
      friend class Singleton< SigIll >;
   private:
      SigIll();
   };

   class SigInt : public PosixSignal
   {
      friend class Singleton< SigInt >;
   private:
      SigInt();
   };

   class SigQuit : public PosixSignal
   {
      friend class Singleton< SigQuit >;
   private:
      SigQuit();
   };

   class SigSegv : public PosixSignal
   {
      friend class Singleton< SigSegv >;
   private:
      SigSegv();
   };

   class SigSys : public PosixSignal
   {
      friend class Singleton< SigSys >;
   private:
      SigSys();
   };

   class SigTerm : public PosixSignal
   {
      friend class Singleton< SigTerm >;
   private:
      SigTerm();
   };

   class SigVtAlrm : public PosixSignal
   {
      friend class Singleton< SigVtAlrm >;
   private:
      SigVtAlrm();
   };
};
}
#endif
