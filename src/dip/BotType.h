//==============================================================================
//
//  BotType.h
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
#ifndef BOTTYPE_H_INCLUDED
#define BOTTYPE_H_INCLUDED

//  A Bot writer must change both of the following lines.
//  o Change the #include to include the header file for your Bot.
//  o Change the typedef to make BotType be the class name for your Bot.
//
#include "BaseBot.h"

namespace Diplomacy
{
   typedef BaseBot BotType;
}
#endif
