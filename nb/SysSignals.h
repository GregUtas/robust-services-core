//==============================================================================
//
//  SysSignals.h
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
   //  Deleted because this class only has static members.
   //
   SysSignals() = delete;

   //  Creates native signals during system initialization.
   //
   static void CreateNativeSignals();
private:
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

   class SigSegv : public PosixSignal
   {
      friend class Singleton< SigSegv >;
   private:
      SigSegv();
   };

   class SigTerm : public PosixSignal
   {
      friend class Singleton< SigTerm >;
   private:
      SigTerm();
   };
};
}
#endif
