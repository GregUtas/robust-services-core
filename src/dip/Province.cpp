//==============================================================================
//
//  Province.cpp
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
#include "Province.h"
#include "Debug.h"
#include "TokenMessage.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace Diplomacy
{
Province::Province() :
   is_valid(false),
   is_supply_centre(false),
   is_land(false)
{
}

//------------------------------------------------------------------------------

size_t Province::process_adjacency_list(const TokenMessage& adjacency_list)
{
   Debug::ft("Province.process_adjacency_list");

   Token coast;
   Token adjacent_coast_token;
   auto adjacency_token = adjacency_list.get_parm(0);

   if(adjacency_token.is_single_token())
   {
      coast = adjacency_token.front();
      adjacent_coast_token = coast;

      if(coast == TOKEN_UNIT_AMY)
      {
         is_land = true;
      }
   }
   else
   {
      coast = adjacency_token.at(1);
      adjacent_coast_token = TOKEN_UNIT_FLT;
   }

   if(!neighbours[coast].empty())
   {
      return 0;
   }

   for(size_t count = 1; count < adjacency_list.parm_count(); ++count)
   {
      Location adjacent_location;

      adjacency_token = adjacency_list.get_parm(count);

      if(adjacency_token.is_single_token())
      {
         adjacent_location.province = adjacency_token.front().province_id();
         adjacent_location.coast = adjacent_coast_token;
      }
      else
      {
         adjacent_location.province = adjacency_token.front().province_id();
         adjacent_location.coast = adjacency_token.at(1);
      }

      neighbours[coast].insert(adjacent_location);
   }

   return NO_ERROR;
}
}
