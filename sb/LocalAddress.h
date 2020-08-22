//==============================================================================
//
//  LocalAddress.h
//
//  Copyright (C) 2013-2020  Greg Utas
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

//------------------------------------------------------------------------------

namespace SessionBase
{
//  Address for a SessionBase intraprocessor message, which specifies the
//  Factory and, if allocated and known, the object that will receive the
//  message.
//
struct LocalAddress
{
   NodeBase::PooledObjectId bid : 32;    // object sending or receiving message
   NodeBase::PooledObjectSeqNo seq : 8;  // object's incarnation number
   NodeBase::ObjectPoolId pid : 8;       // object pool associated with object
   FactoryId fid : 16;             // the factory sending/receiving the message

   //  Constructor.
   //
   LocalAddress();

   //  Copy/move constructors.
   //
   LocalAddress(const LocalAddress& that) = default;
   LocalAddress(LocalAddress&& that) = default;

   //  Copy/move operators.
   //
   LocalAddress& operator=(const LocalAddress& that) = default;
   LocalAddress& operator=(LocalAddress&& that) = default;

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
