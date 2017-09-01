//==============================================================================
//
//  LocalAddress.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef LOCALADDRESS_H_INCLUDED
#define LOCALADDRESS_H_INCLUDED

#include <string>
#include "NbTypes.h"
#include "SbTypes.h"

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

//  Nil local address.
//
extern const LocalAddress NilLocalAddress;
}
#endif
