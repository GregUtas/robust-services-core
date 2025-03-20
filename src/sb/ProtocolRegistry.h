//==============================================================================
//
//  ProtocolRegistry.h
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
#ifndef PROTOCOLREGISTRY_H_INCLUDED
#define PROTOCOLREGISTRY_H_INCLUDED

#include "Immutable.h"
#include "NbTypes.h"
#include "Registry.h"

namespace SessionBase
{
   class Protocol;
}

//------------------------------------------------------------------------------

namespace SessionBase
{
//  Global registry for protocols.
//
class ProtocolRegistry : public NodeBase::Immutable
{
   friend class NodeBase::Singleton<ProtocolRegistry>;
   friend class Protocol;
public:
   //  Deleted to prohibit copying.
   //
   ProtocolRegistry(const ProtocolRegistry& that) = delete;

   //  Deleted to prohibit copy assignment.
   //
   ProtocolRegistry& operator=(const ProtocolRegistry& that) = delete;

   //  Returns the protocols in the registry.
   //
   const NodeBase::Registry<Protocol>& Protocols() const { return protocols_; }

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const NodeBase::Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;

   //  Overridden to display each protocol.
   //
   size_t Summarize(std::ostream& stream, uint32_t selector) const override;
private:
   //  Private because this is a singleton.
   //
   ProtocolRegistry();

   //  Private because this is a singleton.
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
   NodeBase::Registry<Protocol> protocols_;
};
}
#endif
