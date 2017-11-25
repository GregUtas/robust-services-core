//==============================================================================
//
//  ProtocolRegistry.h
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
#ifndef PROTOCOLREGISTRY_H_INCLUDED
#define PROTOCOLREGISTRY_H_INCLUDED

#include "Protected.h"
#include "NbTypes.h"
#include "Registry.h"
#include "SbTypes.h"

namespace SessionBase
{
   class Protocol;
}

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace SessionBase
{
//  Global registry for protocols.
//
class ProtocolRegistry : public Protected
{
   friend class Protocol;
   friend class Singleton< ProtocolRegistry >;
public:
   //  Returns the protocol registered against PRID.
   //
   Protocol* GetProtocol(ProtocolId prid) const;

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
   ProtocolRegistry();

   //  Private because this singleton is not subclassed.
   //
   ~ProtocolRegistry();

   //  Adds PROTOCOL to the registry.  Invoked by Protocol's base class
   //  constructor.
   //
   bool BindProtocol(Protocol& protocol);

   //  Removes PROTOCOL from the registry.  Invoked by Protocol's base
   //  class destructor.
   //
   void UnbindProtocol(Protocol& protocol);

   //  The global registry of protocols.
   //
   Registry< Protocol > protocols_;
};
}
#endif
