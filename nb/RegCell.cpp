//==============================================================================
//
//  RegCell.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "RegCell.h"
#include <iosfwd>
#include <sstream>
#include "Debug.h"

using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
RegCell::RegCell() : id(NIL_ID), bound(false) { }

//------------------------------------------------------------------------------

fn_name RegCell_dtor = "RegCell.dtor";

RegCell::~RegCell()
{
   if(bound)
   {
      Debug::SwErr(RegCell_dtor, id, 0);
   }
}

//------------------------------------------------------------------------------

fn_name RegCell_SetId = "RegCell.SetId";

void RegCell::SetId(id_t cid)
{
   if(bound)
      Debug::SwErr(RegCell_SetId, id, cid);
   else
      id = cid;
}

//------------------------------------------------------------------------------

string RegCell::to_str() const
{
   std::ostringstream stream;
   stream << id;
   if(!bound) stream << " (not bound)";
   return stream.str();
}
}
