//==============================================================================
//
//  Location.cpp
//
//  Diplomacy AI Client - Part of the DAIDE project (www.daide.org.uk).
//
//  (C) David Norman 2002 david@ellought.demon.co.uk
//  (C) Greg Utas 2019-2025 greg@pentennea.com
//
//  This software may be reused for non-commercial purposes without charge,
//  and without notifying the authors.  Use of any part of this software for
//  commercial purposes without permission from the authors is prohibited.
//
#include "Location.h"
#include <ostream>
#include "TokenMessage.h"

using std::ostream;

//------------------------------------------------------------------------------

namespace Diplomacy
{
Location::Location() : province(NIL_PROVINCE), coast(INVALID_TOKEN) { }

//------------------------------------------------------------------------------

Location::Location(ProvinceId p, const Token& c) : province(p), coast(c) { }

//------------------------------------------------------------------------------

Location::Location(const TokenMessage& where, const Token& unit_type) :
   province(where.front().province_id()),
   coast(where.is_single_token() ? unit_type : where.at(1))
{
}

//------------------------------------------------------------------------------

Location::Location(const TokenMessage& unit) :
   province(unit.get_parm(2).front().province_id()),
   coast(unit.get_parm(1).front())
{
}

//------------------------------------------------------------------------------

bool Location::operator<(const Location& that) const
{
   if(this->province < that.province) return true;
   if(that.province < this->province) return false;
   if(this->coast < that.coast) return true;
   if(that.coast < this->coast) return false;
   return (this < &that);
}

//------------------------------------------------------------------------------

ostream& operator<<(ostream& stream, const Location& loc)
{
   stream << province_token(loc.province);
   if(loc.coast.category() == CATEGORY_COAST) stream << loc.coast;
   return stream;
}
}
