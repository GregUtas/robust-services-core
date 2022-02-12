//==============================================================================
//
//  StartupParameters.h
//
//  Diplomacy AI Client - Part of the DAIDE project (www.daide.org.uk).
//
//  (C) David Norman 2002 david@ellought.demon.co.uk
//  (C) Greg Utas 2019-2022 greg@pentennea.com
//
//  This software may be reused for non-commercial purposes without charge,
//  and without notifying the authors.  Use of any part of this software for
//  commercial purposes without permission from the authors is prohibited.
//
#ifndef STARTUPPARAMETERS_H_INCLUDED
#define STARTUPPARAMETERS_H_INCLUDED

#include <iosfwd>
#include <string>
#include "NwTypes.h"

using namespace NetworkBase;

//------------------------------------------------------------------------------

namespace Diplomacy
{
//  Command line parameters supported by BaseBot.
//
struct StartupParameters
{
   bool ip_specified;        // set if the server's IP address was provided
   bool name_specified;      // set if the server's name was provided
   std::string server_name;  // the server's name or IP address
   ipport_t server_port;     // the server's port number (in host order)
   int log_level;            // the level at which to generate logs
   bool reconnect;           // set if reconnection parameters were provided
   std::string power;        // the power for reconnection attempts
   int passcode;             // the passcode for reconnection attempts

   //  Initializes fields to default values.
   //
   StartupParameters();

   //  Sets the parameters from the command line that launched the program.
   //
   void SetFromCommandLine();

   //  Displays the parameters in STREAM, with each one preceded by PREFIX.
   //
   void Display(std::ostream& stream, const std::string& prefix) const;
};
}
#endif
