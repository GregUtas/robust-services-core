//==============================================================================
//
//  LocalAddress.h
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
#ifndef LOCALADDRESS_H_INCLUDED
#define LOCALADDRESS_H_INCLUDED

#include <string>
#include "NbTypes.h"
#include "SbTypes.h"
#include "SysTypes.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace SessionBase
{
//  Address for a SessionBase intraprocessor message, which specifies the
//  Factory and, if allocated and known, the object that will receive the
//  message.
//
struct LocalAddress
{
   PooledObjectId bid : 32;    // the object sending or receiving the message
   PooledObjectSeqNo seq : 8;  // the object's incarnation number
   ObjectPoolId pid : 8;       // the object pool associated with the object
   FactoryId fid : 16;         // the factory sending/receiving the message

   //  Constructor.
   //
   LocalAddress() : bid(NIL_ID), seq(0), pid(NIL_ID), fid(NIL_ID) { }

   //  Copy constructor.
   //
   LocalAddress(const LocalAddress& that) = default;

   //  Copy operator.
   //
   LocalAddress& operator=(const LocalAddress& that) = default;

   //  Returns true if both addresses match.  FIDs only have to match if BID
   //  is NIL_ID.
   //
   bool operator==(const LocalAddress& that) const;

   //  Returns the inverse of the == operator.
   //
   bool operator!=(const LocalAddress& that) const;

   //  Returns a string for displaying the address.
   //
   std::string to_str() const;
};
}
#endif
