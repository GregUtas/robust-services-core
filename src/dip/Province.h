//==============================================================================
//
//  Province.h
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
#ifndef PROVINCE_H_INCLUDED
#define PROVINCE_H_INCLUDED

#include <cstddef>
#include <map>
#include "DipTypes.h"
#include "Location.h"
#include "Token.h"

//------------------------------------------------------------------------------

namespace Diplomacy
{
//  For holding the locations adjacent to a given province.  The Token key
//  specifies how the adjacent locations can be reached (by an army, by any
//  fleet, or by a fleet on a specific coast).
//
typedef std::map<Token, LocationSet> AdjacentSet;

//  Information about a province.
//
struct Province
{
   bool is_valid;           // set if a province exists on map
   bool is_supply_centre;   // set if a supply centre
   bool is_land;            // whether land or sea
   Token token;             // token representation of province
   Token owner;             // power that currently owns this centre
   AdjacentSet neighbours;  // adjacent provinces, keyed by values that
                            //   are legal in Location.coast
   PowerSet home_powers;    // powers for which this is a home centre

   //  Initializes members to default values.
   //
   Province();

   //  Updates the province's neighbours when an MDF is received.  Returns
   //  NO_ERROR on success; other values indicate that an error occurred.
   //
   size_t process_adjacency_list(const TokenMessage& adjacency_list);
};
}
#endif
