//==============================================================================
//
//  StartupParameters.cpp
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
#include "StartupParameters.h"
#include <cctype>
#include <cstddef>
#include <sstream>
#include "BaseBot.h"
#include "Debug.h"
#include "DipTypes.h"
#include "MainArgs.h"
#include "SysConsole.h"
#include "SysTypes.h"

using std::ostream;
using std::string;
using namespace NodeBase;

//------------------------------------------------------------------------------

namespace Diplomacy
{
StartupParameters::StartupParameters() :
   ip_specified(false),
   name_specified(false),
   server_name(EMPTY_STR),
   server_port(ServerIpPort),
   log_level(0),
   reconnect(false),
   power(EMPTY_STR),
   passcode(0)
{
   Debug::ft("StartupParameters.ctor");
}

//------------------------------------------------------------------------------

void StartupParameters::Display(ostream& stream, const string& prefix) const
{
   stream << prefix << "ip_specified   : " << ip_specified << CRLF;
   stream << prefix << "name_specified : " << name_specified << CRLF;
   stream << prefix << "server_name    : " << server_name << CRLF;
   stream << prefix << "server_port    : " << server_port << CRLF;
   stream << prefix << "log_level      : " << log_level << CRLF;
   stream << prefix << "reconnect      : " << reconnect << CRLF;
   stream << prefix << "power          : " << power << CRLF;
   stream << prefix << "passcode       : " << passcode << CRLF;
}

//------------------------------------------------------------------------------

fn_name StartupParameters_SetFromCommandLine =
   "StartupParameters.SetFromCommandLine";

void StartupParameters::SetFromCommandLine()
{
   Debug::ft(StartupParameters_SetFromCommandLine);

   auto bot = BaseBot::instance();
   auto size = MainArgs::Size();

   for(size_t p = 1; p < size; ++p)
   {
      string parm(MainArgs::At(p));

      if(parm.front() != '-') continue;

      if(parm.size() == 1)
      {
         Debug::SwLog(StartupParameters_SetFromCommandLine,
            "Empty '-' on command line ", 0);
         continue;
      }

      auto token = parm.at(1);
      auto value = parm.substr(2);

      switch(token)
      {
      case 'N':
      case 'n':
         name_specified = true;
         server_name = value;
         break;

      case 'I':
      case 'i':
         ip_specified = true;
         server_name = value;
         break;

      case 'S':
      case 's':
         server_port = std::stoi(value);
         break;

      case 'L':
      case 'l':
         log_level = std::stoi(value);
         break;

      case 'R':
      case 'r':
         if((value.size() == 8) && (value[3] == ':'))
         {
            reconnect = true;
            auto nation = value.substr(0, 3);
            for(auto i = 0; i < 3; ++i) nation[i] = toupper(nation[i]);
            power = nation;
            passcode = std::stoi(value.substr(4, 4));
         }
         else
         {
            Debug::SwLog(StartupParameters_SetFromCommandLine,
               "-r must precede 'POW:code' (3 alphabetic, 4 numeric", 1);
         }
         break;

      case 'H':
      case 'h':
         SysConsole::Minimize(true);
         break;

      default:
         if(!bot->process_command_line_parameter(token, value))
         {
            std::ostringstream report;
            report << "COMMAND LINE PARAMETERS:" << CRLF;
            report << "[-nServerName |  (string)" << CRLF;
            report << " -iServerIPAddr] (n:n:n:n)" << CRLF;
            report << "[-sServerIPPort] (1024-65535)" << CRLF;
            report << "[-lLogLevel]     (0-3)" << CRLF;
            report << "[-rPOW:code]     (AAA:0-9999)" << CRLF;
            report << "[-h]             (hides window)" << CRLF;

            auto expl = bot->report_command_line_parameters();
            if(!expl.empty()) report << expl << CRLF;
            BaseBot::send_to_console(report);
         }
      }
   }

   if(ip_specified && name_specified)
   {
      Debug::SwLog(StartupParameters_SetFromCommandLine,
         "shouldn't specify both ServerName and ServerIPAddr", 2);
   }
}
}
