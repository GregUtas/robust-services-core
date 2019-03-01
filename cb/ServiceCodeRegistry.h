//==============================================================================
//
//  ServiceCodeRegistry.h
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
#ifndef SERVICECODEREGISTRY_H_INCLUDED
#define SERVICECODEREGISTRY_H_INCLUDED

#include "Protected.h"
#include "BcAddress.h"
#include "NbTypes.h"
#include "SbTypes.h"

using namespace NodeBase;
using namespace SessionBase;

//------------------------------------------------------------------------------

namespace CallBase
{
//  Registry for service codes (*nn digit strings used to control services).
//
class ServiceCodeRegistry : public Protected
{
   friend class Singleton< ServiceCodeRegistry >;
public:
   //  Associates the service identified by SID when the service code
   //  identified by SC.
   //
   void SetService(Address::SC sc, ServiceId sid);

   //  Returns the service associated with SC.
   //
   ServiceId GetService(Address::SC sc) const;

   //  Overridden for restarts.
   //
   void Startup(RestartLevel level) override;

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;
private:
   //  Private because this singleton is not subclassed.
   //
   ServiceCodeRegistry();

   //  Private because this singleton is not subclassed.
   //
   ~ServiceCodeRegistry();

   //  The table that maps service codes to service identifiers.
   //
   ServiceId codeToService_[Address::LastSC + 1];
};
}
#endif
