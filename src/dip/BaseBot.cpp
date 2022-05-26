//==============================================================================
//
//  BaseBot.cpp
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
#include "BaseBot.h"
#include <cstring>
#include <iterator>
#include <map>
#include <sstream>
#include <utility>
#include "BotThread.h"
#include "BotTrace.h"
#include "BotType.h"
#include "CliThread.h"
#include "Debug.h"
#include "Formatters.h"
#include "FunctionGuard.h"
#include "IoThread.h"
#include "IpPort.h"
#include "IpPortRegistry.h"
#include "Location.h"
#include "MainArgs.h"
#include "MapAndUnits.h"
#include "NbTracer.h"
#include "NbTypes.h"
#include "NwTracer.h"
#include "NwTypes.h"
#include "Province.h"
#include "Singleton.h"
#include "SysConsole.h"
#include "SysTcpSocket.h"
#include "ThisThread.h"
#include "Token.h"
#include "TokenTextMap.h"
#include "Tool.h"
#include "ToolTypes.h"
#include "TraceBuffer.h"
#include "UnitOrder.h"
#include "WinterOrders.h"

using std::ostream;
using std::string;
using namespace NodeBase;
using namespace NetworkBase;

//------------------------------------------------------------------------------

namespace Diplomacy
{
BaseBot::BaseBot() :
   map_and_units(MapAndUnits::instance()),
   initialised_(false),
   state_(DISCONNECTED),
   retry_delay_(1),
   name_("BaseBot"),
   version_("1.0"),
   reconnect_(false),
   observer_(false),
   report_(false),
   ord_received_(false),
   map_requested_(false)
{
   Debug::ft("BaseBot.ctor");

   title_ = MainArgs::At(0);
}

//------------------------------------------------------------------------------

TokenMessage BaseBot::active_powers(bool self) const
{
   Debug::ft("BaseBot.active_powers");

   TokenMessage result;

   for(PowerId p = 0; p < map_and_units->number_of_powers; ++p)
   {
      Token power(power_token(p));

      if((self || (power != map_and_units->our_power)) &&
         (out_powers.find(power) == out_powers.end()) &&
         (cd_powers.find(power) == cd_powers.end()))
      {
         result = result + power;
      }
   }

   return result;
}

//------------------------------------------------------------------------------

void BaseBot::cancel_event(BotEvent event)
{
   Debug::ft("BaseBot.cancel_event");

   Singleton<BotThread>::Instance()->CancelEvent(event);
}

//------------------------------------------------------------------------------

void BaseBot::check_sent_press_for_inactive_power(const Token& inactive_power)
{
   Debug::ft("BaseBot.check_sent_press_for_inactive_power");

   for(auto press = sent_press_.begin(); press != sent_press_.end(); ++press)
   {
      auto receiving_powers = press->receiving_powers;

      for(size_t p = 0; p < receiving_powers.size(); ++p)
      {
         if(receiving_powers.at(p) == inactive_power)
         {
            if(press->resend_partial)
            {
               send_to_reduced_powers(press, inactive_power);
               break;
            }
            else
            {
               report_failed_press(press->is_broadcast,
                  press->original_receiving_powers, press->message);
               break;
            }
         }
      }
   }
}

//------------------------------------------------------------------------------

BaseBot::StartupResult BaseBot::create_socket()
{
   Debug::ft("BaseBot.create_socket");

   //  Release any existing socket, allocate a new one, register our use
   //  of it, and save it against the server's IP address.
   //
   server_addr_.ReleaseSocket();

   auto reg = Singleton<IpPortRegistry>::Instance();
   auto port = reg->GetPort(ClientIpPort);
   auto socket = port->CreateAppSocket();
   if(socket == nullptr) return FAILED_TO_ALLOCATE_SOCKET;
   socket->Acquire();
   server_addr_.SetSocket(socket);
   return STARTUP_OK;
}

//------------------------------------------------------------------------------

void BaseBot::delete_socket(ProtocolError error)
{
   Debug::ft("BaseBot.delete_socket");

   set_state(DISCONNECTED);
   server_addr_.ReleaseSocket();

   if(!reconnect_)
      report_failed_connect();
   else
      report_close(error);

   //  If ERROR is SOCKET_FAILED, try to reconnect after a delay.
   //  All other errors mean that we should exit.
   //
   if(error == SOCKET_FAILED)
   {
      queue_event(RECONNECT_EVENT, reconnection_delay());
      return;
   }

   string reason("connection closed [error=");
   reason += std::to_string(error) + ']';
   report_exit(reason.c_str());
}

//------------------------------------------------------------------------------

fn_name BaseBot_disconnect_from_server = "BaseBot.disconnect_from_server";

void BaseBot::disconnect_from_server(ProtocolError error)
{
   Debug::ft(BaseBot_disconnect_from_server);

   if(state_ == DISCONNECTED)
   {
      Debug::SwLog(BaseBot_disconnect_from_server, "already disconnected", 0);
      return;
   }

   auto signal = FM_MESSAGE;
   uint16_t length = sizeof(FM_Message);

   if(error != GRACEFUL_CLOSE)
   {
      signal = EM_MESSAGE;
      length = sizeof(EM_Message);
   }

   DipIpBufferPtr buff(new DipIpBuffer(MsgOutgoing, length));
   buff->SetTxAddr(client_addr_);
   buff->SetRxAddr(server_addr_);

   auto em = reinterpret_cast<EM_Message*>(buff->PayloadPtr());
   em->header.signal = signal;
   em->header.spare = 0;
   em->header.length = length - DipHeaderSize;

   if(signal == EM_MESSAGE)
   {
      em->error = error;
   }

   if(!send_buff(*buff))
   {
      std::ostringstream stream;
      stream << "Failed to send ";
      stream << (signal == FM_MESSAGE ? "FM" : "EM") << CRLF;
      send_to_console(stream);
   }

   buff.reset();
   delete_socket(error);
}

//------------------------------------------------------------------------------

void BaseBot::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   stream << prefix << "cd_powers :";
   if(cd_powers.empty()) stream << " none";
   else
   {
      for(auto p = cd_powers.cbegin(); p != cd_powers.end(); ++p)
         stream << SPACE << *p;
   }
   stream << CRLF;

   stream << prefix << "out_powers :";
   if(out_powers.empty()) stream << " none";
   else
   {
      for(auto p = out_powers.cbegin(); p != out_powers.end(); ++p)
         stream << SPACE << *p;
   }
   stream << CRLF;

   auto lead = prefix + spaces(2);
   stream << prefix << "config :" << CRLF;
   config_.Display(stream, lead);

   stream << prefix << "client_addr    : " << client_addr_.to_str(true) << CRLF;
   stream << prefix << "server_addr    : " << server_addr_.to_str(true) << CRLF;
   stream << prefix << "state          : " << state_ << CRLF;
   stream << prefix << "retry_delay    : " << retry_delay_ << CRLF;
   stream << prefix << "title          : " << title_ << CRLF;
   stream << prefix << "name           : " << name_ << CRLF;
   stream << prefix << "version        : " << version_ << CRLF;
   stream << prefix << "reconnect      : " << reconnect_ << CRLF;
   stream << prefix << "observer       : " << observer_ << CRLF;
   stream << prefix << "report         : " << report_ << CRLF;
   stream << prefix << "ord_received   : " << ord_received_ << CRLF;
   stream << prefix << "map_requested  : " << map_requested_ << CRLF;
   stream << prefix << "map_message    : " << map_message_.to_str() << CRLF;
   stream << prefix << "sent_press (#) : " << sent_press_.size() << CRLF;
}

//------------------------------------------------------------------------------

BaseBot::StartupResult BaseBot::get_ipaddrs()
{
   Debug::ft("BaseBot.get_ipaddrs");

   std::ostringstream stream;

   client_addr_ = SysIpL3Addr(IpPortRegistry::LocalAddr(), ClientIpPort);

   if(config_.ip_specified)
   {
      SysIpL2Addr addr(config_.server_name);

      if(!addr.IsValid())
      {
         stream << "Server's IP address is ill-formed" << CRLF;
         stream << "address=" << config_.server_name << CRLF;
         send_to_console(stream);
         return SERVER_ADDRESS_LOOKUP_FAILED;
      }

      server_addr_ = SysIpL3Addr(addr, config_.server_port);
   }
   else if(config_.name_specified)
   {
      IpProtocol proto;
      server_addr_ = SysIpL3Addr
         (config_.server_name, std::to_string(config_.server_port), proto);

      if(!server_addr_.IsValid())
      {
         stream << "Server's name lookup failed" << CRLF;
         stream << "name=" << config_.server_name << CRLF;
         send_to_console(stream);
         return SERVER_ADDRESS_LOOKUP_FAILED;
      }

      if(proto != IpTcp)
      {
         stream << "Server's protocol is not TCP" << CRLF;
         stream << "protocol=" << proto << CRLF;
         send_to_console(stream);
         return SERVER_PROTOCOL_INCORRECT;
      }
   }
   else
   {
      config_.name_specified = SysIpL2Addr::LocalName(config_.server_name);
      auto& addr = SysIpL2Addr::LoopbackIpAddr();
      server_addr_ = SysIpL3Addr(addr, config_.server_port);
   }

   //  Create our IP port.
   //
   auto reg = Singleton<IpPortRegistry>::Instance();
   auto port = reg->GetPort(ClientIpPort);

   if(port == nullptr)
   {
      auto service = Singleton<BotTcpService>::Instance();
      port = service->Provision(ClientIpPort);
      if(port == nullptr) return FAILED_TO_ALLOCATE_PORT;
   }

   if(config_.log_level >= 2)
   {
      auto iot = port->GetThread();
      NbTracer::SelectThread(iot->Tid(), TraceIncluded);
      auto nwt = Singleton<NwTracer>::Instance();
      nwt->SelectPeer(server_addr_, TraceIncluded);
   }

   return STARTUP_OK;
}

//------------------------------------------------------------------------------

bool BaseBot::get_reconnect_details(Token& power, int& passcode) const
{
   Debug::ft("BaseBot.get_reconnect_details");

   //  If reconnection information was provided during startup,
   //  supply it and request that reconnection be attempted.
   //
   if(config_.reconnect)
   {
      auto& map = TokenTextMap::instance()->text_to_token_map();
      power = map.at(config_.power);
      passcode = config_.passcode;
   }

   return config_.reconnect;
}

//------------------------------------------------------------------------------

const std::vector<Token>& BaseBot::get_try_tokens() const
{
   Debug::ft("BaseBot.get_try_tokens");

   static std::vector<Token> no_tokens;

   return no_tokens;
}

//------------------------------------------------------------------------------

BaseBot::StartupResult BaseBot::initialise()
{
   Debug::ft("BaseBot.initialise");

   if(initialised_) return STARTUP_OK;

   config_.SetFromCommandLine();

   if(config_.log_level > 0)
   {
      auto buff = Singleton<TraceBuffer>::Instance();
      buff->StopTracing();
      buff->Clear();
      buff->ClearTools();
      buff->SetTool(DipTracer, true);
      if(config_.log_level >= 2) buff->SetTool(NetworkTracer, true);
      if(config_.log_level >= 3) buff->SetTool(FunctionTracer, true);

      auto nbt = Singleton<NbTracer>::Instance();
      nbt->ClearSelections(TraceAll);
      ThisThread::IncludeInTrace();
      ThisThread::StartTracing(EMPTY_STR);
   }

   auto rc = get_ipaddrs();
   if(rc != STARTUP_OK) return rc;

   rc = create_socket();
   if(rc != STARTUP_OK) return rc;

   rc = initialise(config_);
   if(rc != STARTUP_OK) return rc;

   initialised_ = true;

   //  Send an IM message to connect to the server.  If this fails, we will
   //  retry, so just report success for now.
   //
   send_im_message();
   return STARTUP_OK;
}

//------------------------------------------------------------------------------

BaseBot::StartupResult BaseBot::initialise(const StartupParameters& parameters)
{
   return STARTUP_OK;
}

//------------------------------------------------------------------------------

BaseBot* BaseBot::instance()
{
   static BotType bot;

   return &bot;
}

//------------------------------------------------------------------------------

void BaseBot::process_adm_message(const TokenMessage& message)
{
   Debug::ft("BaseBot.process_adm_message");

   if(report_) message.log("ADM received");
}

//------------------------------------------------------------------------------

void BaseBot::process_bm_message(const DipMessage& message)
{
   Debug::ft("BaseBot.process_bm_message");

   std::ostringstream stream;
   stream << "Unprocessed BM: event=" << int(message.header.spare) << CRLF;
   send_to_console(stream);
}

//------------------------------------------------------------------------------

void BaseBot::process_ccd(const TokenMessage& message)
{
   Debug::ft("BaseBot.process_ccd");

   auto cd_power = message.get_parm(1).front();
   auto is_new_disconnection = false;

   check_sent_press_for_inactive_power(cd_power);

   if(cd_powers.find(cd_power) == cd_powers.end())
   {
      cd_powers.insert(cd_power);
      is_new_disconnection = true;
   }

   process_ccd_message(message, is_new_disconnection);
}

//------------------------------------------------------------------------------

void BaseBot::process_ccd_message
   (const TokenMessage& message, bool is_new_disconnection)
{
   Debug::ft("BaseBot.process_ccd_message");

   if(report_ && is_new_disconnection)
   {
      report_ccd(message.get_parm(1).front(), true);
   }
}

//------------------------------------------------------------------------------

bool BaseBot::process_command_line_parameter(char token, string& value)
{
   Debug::ft("BaseBot.process_command_line_parameter");

   return false;
}

//------------------------------------------------------------------------------

void BaseBot::process_dm_message(const DipMessage& message)
{
   Debug::ft("BaseBot.process_dm_message");

   auto& dm = reinterpret_cast<const DM_Message&>(message);
   auto tokens = reinterpret_cast<const Token*>(&dm.tokens);

   if(tokens[0] == TOKEN_COMMAND_PRN)
   {
      //  A PRN cannot be converted to a TokenMessage, because its
      //  constructor checks for balanced parentheses.
      //
      process_prn_message(tokens, message.header.length);
      return;
   }

   TokenMessage icmsg(tokens, message.header.length / sizeof(Token));

   if(!icmsg.parm_is_single_token(0))
   {
      std::ostringstream stream;
      stream << "Ill-formed DM received:" << CRLF;
      dm.Display(stream);
      send_to_console(stream);
      return;
   }

   auto signal = icmsg.front().all();

   switch(signal)
   {
   case TOKEN_COMMAND_HLO:
      map_and_units->process_hlo(icmsg);
      set_title(TOKEN_COMMAND_HLO, true);
      process_hlo_message(icmsg);
      break;
   case TOKEN_COMMAND_MAP:
      map_message_ = icmsg;
      map_and_units->process_map(icmsg);
      send_to_server(TOKEN_COMMAND_MDF);
      process_map_message(icmsg);
      break;
   case TOKEN_COMMAND_MDF:
      process_mdf(icmsg);
      break;
   case TOKEN_COMMAND_NOW:
      process_now(icmsg);
      break;
   case TOKEN_COMMAND_ORD:
      process_ord(icmsg);
      break;
   case TOKEN_COMMAND_SCO:
      process_sco(icmsg);
      break;
   case TOKEN_COMMAND_YES:
      process_yes(icmsg);
      break;
   case TOKEN_COMMAND_REJ:
      process_rej(icmsg);
      break;
   case TOKEN_COMMAND_NOT:
      process_not(icmsg);
      break;
   case TOKEN_COMMAND_CCD:
      process_ccd(icmsg);
      break;
   case TOKEN_COMMAND_OUT:
      process_out(icmsg);
      break;
   case TOKEN_COMMAND_DRW:
      map_and_units->game_over = true;
      process_drw_message(icmsg);
      break;
   case TOKEN_COMMAND_SLO:
      map_and_units->game_over = true;
      process_slo_message(icmsg);
      break;
   case TOKEN_COMMAND_FRM:
      process_frm_message(icmsg);
      break;
   case TOKEN_COMMAND_HUH:
      process_huh_message(icmsg);
      break;
   case TOKEN_COMMAND_LOD:
      process_lod_message(icmsg);
      break;
   case TOKEN_COMMAND_MIS:
      process_mis_message(icmsg);
      break;
   case TOKEN_COMMAND_OFF:
      process_off_message(icmsg);
      break;
   case TOKEN_COMMAND_SMR:
      process_smr_message(icmsg);
      break;
   case TOKEN_COMMAND_SVE:
      process_sve_message(icmsg);
      break;
   case TOKEN_COMMAND_THX:
      process_thx_message(icmsg);
      break;
   case TOKEN_COMMAND_TME:
      process_tme_message(icmsg);
      break;
   case TOKEN_COMMAND_ADM:
      process_adm_message(icmsg);
      break;
   default:
      std::ostringstream stream;
      stream << "Unexpected DM command token received: " << signal << CRLF;
      send_to_console(stream);
   }
}

//------------------------------------------------------------------------------

void BaseBot::process_drw_message(const TokenMessage& message)
{
   Debug::ft("BaseBot.process_drw_message");

   if(report_) report_end(message);
}

//------------------------------------------------------------------------------

void BaseBot::process_em_message(const DipMessage& message)
{
   Debug::ft("BaseBot.process_em_message");

   //  The server has closed the connection because of an error.
   //
   auto& em = reinterpret_cast<const EM_Message&>(message);
   delete_socket(em.error);
}

//------------------------------------------------------------------------------

void BaseBot::process_fm_message(const DipMessage& message)
{
   Debug::ft("BaseBot.process_fm_message");

   //  The server has closed the connection gracefully.
   //
   delete_socket(GRACEFUL_CLOSE);
}

//------------------------------------------------------------------------------

void BaseBot::process_frm_message(const TokenMessage& message)
{
   Debug::ft("BaseBot.process_frm_message");

   auto message_id = message.get_parm(1);
   auto from_power = message_id.front();
   auto press = message.get_parm(3);

   switch(press.front().all())
   {
   case TOKEN_COMMAND_HUH:
   case TOKEN_PRESS_TRY:
      //
      //  Replying to either of these with a HUH-TRY pair could cause a
      //  messaging loop.
      //
      break;

   default:
      auto huh_message = Token(TOKEN_COMMAND_SND) & from_power &
         (Token(TOKEN_COMMAND_HUH) & (Token(TOKEN_PARAMETER_ERR) + press));
      send_to_server(huh_message);

      auto& tokens = get_try_tokens();
      TokenMessage token_msg;

      for(auto token = tokens.cbegin(); token != tokens.cend(); ++token)
      {
         token_msg = token_msg + *token;
      }

      auto try_message = Token(TOKEN_COMMAND_SND) & from_power &
         (Token(TOKEN_PRESS_TRY) & token_msg);

      send_to_server(try_message);
   }
}

//------------------------------------------------------------------------------

void BaseBot::process_hlo_message(const TokenMessage& message)
{
   Debug::ft("BaseBot.process_hlo_message");

   if(report_)
   {
      message.log("HLO received");

      std::ostringstream stream;
      stream << "The game is starting." << CRLF;
      send_to_console(stream);
   }
}

//------------------------------------------------------------------------------

void BaseBot::process_huh_message(const TokenMessage& message)
{
   Debug::ft("BaseBot.process_huh_message");

   message.log("HUH received");
}

//------------------------------------------------------------------------------

void BaseBot::process_lod_message(const TokenMessage& message)
{
   Debug::ft("BaseBot.process_lod_message");

   send_to_server(Token(TOKEN_COMMAND_REJ) & message);
}

//------------------------------------------------------------------------------

void BaseBot::process_map_message(const TokenMessage& message)
{
   Debug::ft("BaseBot.process_map_message");

   if(report_) message.log("MAP received");
}

//------------------------------------------------------------------------------

void BaseBot::process_mdf(const TokenMessage& message)
{
   Debug::ft("BaseBot.process_mdf");

   auto rc = map_and_units->process_mdf(message);

   if(rc != NO_ERROR)
   {
      std::ostringstream stream;
      stream << "Failed to process MDF: err=" << rc << CRLF;
      send_to_console(stream);
   }

   process_mdf_message(message);

   //  If we didn't request the map, accept it explicitly.  If we requested
   //  it after an IAM (reconnect), also request an HLO, ORD, SCO and NOW to
   //  ensure that we have the game's current state.
   //
   if(!map_requested_)
   {
      send_to_server(Token(TOKEN_COMMAND_YES) & map_message_);
   }
   else
   {
      send_to_server(TokenMessage(TOKEN_COMMAND_HLO));
      send_to_server(TokenMessage(TOKEN_COMMAND_ORD));
      send_to_server(TokenMessage(TOKEN_COMMAND_SCO));
      send_to_server(TokenMessage(TOKEN_COMMAND_NOW));
      map_requested_ = false;
   }
}

//------------------------------------------------------------------------------

void BaseBot::process_mdf_message(const TokenMessage& message)
{
   Debug::ft("BaseBot.process_mdf_message");

   if(report_) report_mdf();
}

//------------------------------------------------------------------------------

void BaseBot::process_message(const DipMessage& message)
{
   Debug::ft("BaseBot.process_message");

   switch(message.header.signal)
   {
   case RM_MESSAGE:
      process_rm_message(message);
      break;

   case DM_MESSAGE:
      process_dm_message(message);
      break;

   case FM_MESSAGE:
      process_fm_message(message);
      break;

   case EM_MESSAGE:
      process_em_message(message);
      break;

   case BM_MESSAGE:
      switch(message.header.spare)
      {
      case SOCKET_FAILURE_EVENT:
         delete_socket(SOCKET_FAILED);
         break;
      case RECONNECT_EVENT:
         reconnect();
         break;
      default:
         process_bm_message(message);
      }
      break;

   default:
      //
      //  An IM_MESSAGE or something bizarre.
      //
      std::ostringstream stream;
      stream << "Unexpected message received: signal=";
      stream << int(message.header.signal) << CRLF;
      send_to_console(stream);
   }
}

//------------------------------------------------------------------------------

void BaseBot::process_mis_message(const TokenMessage& message) { }

//------------------------------------------------------------------------------

void BaseBot::process_not(const TokenMessage& message)
{
   Debug::ft("BaseBot.process_not");

   auto inner = message.get_parm(1);
   auto signal = inner.front().all();

   switch(signal)
   {
   case TOKEN_COMMAND_CCD:
      process_not_ccd(message, inner.get_parm(1));
      break;

   case TOKEN_COMMAND_TME:
      process_not_tme_message(message, inner.get_parm(1));
      break;

   default:
      process_unexpected_not_message(message);
   }
}

//------------------------------------------------------------------------------

void BaseBot::process_not_ccd
   (const TokenMessage& message, const TokenMessage& parameters)
{
   Debug::ft("BaseBot.process_not_ccd");

   auto cd_power = parameters.front();
   auto is_new_reconnection = (cd_powers.erase(cd_power) > 0);
   process_not_ccd_message(message, parameters, is_new_reconnection);
}

//------------------------------------------------------------------------------

void BaseBot::process_not_ccd_message(const TokenMessage& message,
   const TokenMessage& parameters, bool is_new_reconnection)
{
   Debug::ft("BaseBot.process_not_ccd_message");

   if(report_ && is_new_reconnection)
   {
      report_ccd(parameters.front(), false);
   }
}

//------------------------------------------------------------------------------

void BaseBot::process_not_tme_message
   (const TokenMessage& message, const TokenMessage& parameters) { }

//------------------------------------------------------------------------------

void BaseBot::process_now(const TokenMessage& message)
{
   Debug::ft("BaseBot.process_now");

   if(report_) report_ords();
   map_and_units->process_now(message);
   units = map_and_units->get_units();
   process_now_message(message);
}

//------------------------------------------------------------------------------

void BaseBot::process_now_message(const TokenMessage& message)
{
   Debug::ft("BaseBot.process_now_message");

   if(report_) report_now();
}

//------------------------------------------------------------------------------

void BaseBot::process_off_message(const TokenMessage& message)
{
   Debug::ft("BaseBot.process_off_message");

   if(report_) report_end(message);
   delete_socket(SERVER_OFF);
}

//------------------------------------------------------------------------------

void BaseBot::process_ord(const TokenMessage& message)
{
   Debug::ft("BaseBot.process_ord");

   map_and_units->process_ord(message);
   ord_received_ = true;
   process_ord_message(message);
}

//------------------------------------------------------------------------------

void BaseBot::process_ord_message(const TokenMessage& message) { }

//------------------------------------------------------------------------------

void BaseBot::process_out(const TokenMessage& message)
{
   Debug::ft("BaseBot.process_out");

   //  Update the set of eliminated powers.  If any unacknowledged press
   //  included the eliminated power as a recipient, the server is about
   //  to reject it, so retransmit it to the other recipients.
   //
   auto out_power = message.get_parm(1).front();
   out_powers.insert(out_power);
   check_sent_press_for_inactive_power(out_power);
   process_out_message(message);
}

//------------------------------------------------------------------------------

void BaseBot::process_out_message(const TokenMessage& message)
{
   Debug::ft("BaseBot.process_out_message");

   if(report_) report_out(message.get_parm(1).front());
}

//------------------------------------------------------------------------------

void BaseBot::process_prn_message(const Token* message, size_t size)
{
   Debug::ft("BaseBot.process_prn_message");

   std::ostringstream stream;
   stream << "PRN received" << CRLF;
   auto count = size / sizeof(Token);

   for(size_t i = 0; i < count; ++i)
   {
      stream << message[i].to_str();

      if(message[i].category() != CATEGORY_ASCII)
      {
         stream << (i % 16 == 15 ? CRLF : SPACE);
      }
   }

   if(count % 16 != 0) stream << CRLF;
   send_to_console(stream);
}

//------------------------------------------------------------------------------

void BaseBot::process_rej(const TokenMessage& message)
{
   Debug::ft("BaseBot.process_rej");

   auto inner = message.get_parm(1);
   auto signal = inner.front().all();

   switch(signal)
   {
   case TOKEN_COMMAND_NME:
      process_rej_nme_message(message, inner.get_parm(1));
      break;

   case TOKEN_COMMAND_IAM:
      process_rej_iam_message(message, inner.get_parm(1));
      break;

   case TOKEN_COMMAND_HLO:
      process_rej_hlo_message(message, inner.get_parm(1));
      break;

   case TOKEN_COMMAND_NOW:
      process_rej_now_message(message, inner.get_parm(1));
      break;

   case TOKEN_COMMAND_SCO:
      process_rej_sco_message(message, inner.get_parm(1));
      break;

   case TOKEN_COMMAND_HST:
      process_rej_hst_message(message, inner.get_parm(1));
      break;

   case TOKEN_COMMAND_SUB:
      process_rej_sub_message(message, inner.get_parm(1));
      break;

   case TOKEN_COMMAND_NOT:
      process_rej_not(message, inner.get_parm(1));
      break;

   case TOKEN_COMMAND_GOF:
      process_rej_gof_message(message, inner.get_parm(1));
      break;

   case TOKEN_COMMAND_ORD:
      process_rej_ord_message(message, inner.get_parm(1));
      break;

   case TOKEN_COMMAND_TME:
      process_rej_tme_message(message, inner.get_parm(1));
      break;

   case TOKEN_COMMAND_DRW:
      process_rej_drw_message(message, inner.get_parm(1));
      break;

   case TOKEN_COMMAND_SND:
      process_rej_snd(message, inner.get_parm(1));
      break;

   case TOKEN_COMMAND_ADM:
      process_rej_adm_message(message, inner.get_parm(1));
      break;

   case TOKEN_COMMAND_MIS:
      process_rej_mis_message(message, inner.get_parm(1));
      break;

   default:
      process_unexpected_rej_message(message);
   }
}

//------------------------------------------------------------------------------

void BaseBot::process_rej_adm_message
   (const TokenMessage& message, const TokenMessage& parameters) { }

//------------------------------------------------------------------------------

void BaseBot::process_rej_drw_message
   (const TokenMessage& message, const TokenMessage& parameters)
{
   Debug::ft("BaseBot.process_rej_drw_message");

   message.log("Unprocessed message");
}

//------------------------------------------------------------------------------

void BaseBot::process_rej_gof_message
   (const TokenMessage& message, const TokenMessage& parameters)
{
   Debug::ft("BaseBot.process_rej_gof_message");

   message.log("Unprocessed message");
}

//------------------------------------------------------------------------------

void BaseBot::process_rej_hlo_message
   (const TokenMessage& message, const TokenMessage& parameters)
{
   Debug::ft("BaseBot.process_rej_hlo_message");

   message.log("Unprocessed message");
}

//------------------------------------------------------------------------------

void BaseBot::process_rej_hst_message
   (const TokenMessage& message, const TokenMessage& parameters)
{
   Debug::ft("BaseBot.process_rej_hst_message");

   message.log("Unprocessed message");
}

//------------------------------------------------------------------------------

void BaseBot::process_rej_iam_message
   (const TokenMessage& message, const TokenMessage& parameters)
{
   Debug::ft("BaseBot.process_rej_iam_message");

   message.log("Unprocessed message");
}

//------------------------------------------------------------------------------

void BaseBot::process_rej_mis_message
   (const TokenMessage& message, const TokenMessage& parameters) { }

//------------------------------------------------------------------------------

void BaseBot::process_rej_nme_message
   (const TokenMessage& message, const TokenMessage& parameters)
{
   Debug::ft("BaseBot.process_rej_nme_message");

   Token power;
   Token passcode;
   int code = 0;

   //  If reconnection is to be attempted, verify that POWER was updated
   //  to a valid power token before sending an IAM.
   //
   if(get_reconnect_details(power, code) && power.is_power())
   {
      passcode.set_number(code);
      send_to_server(Token(TOKEN_COMMAND_IAM) & power & passcode);
      set_title(TOKEN_COMMAND_IAM, false);
   }
   else
   {
      disconnect_from_server(GRACEFUL_CLOSE);
   }
}

//------------------------------------------------------------------------------

void BaseBot::process_rej_not
   (const TokenMessage& message, const TokenMessage& rej_not_parameters)
{
   Debug::ft("BaseBot.process_rej_not");

   switch(rej_not_parameters.front().all())
   {
   case TOKEN_COMMAND_GOF:
      process_rej_not_gof_message(message, rej_not_parameters.get_parm(1));
      break;

   case TOKEN_COMMAND_DRW:
      process_rej_not_drw_message(message, rej_not_parameters.get_parm(1));
      break;

   default:
      process_unexpected_rej_not_message(message);
   }
}

//------------------------------------------------------------------------------

void BaseBot::process_rej_not_drw_message
   (const TokenMessage& message, const TokenMessage& parameters)
{
   Debug::ft("BaseBot.process_rej_not_drw_message");

   message.log("Unprocessed message");
}

//------------------------------------------------------------------------------

void BaseBot::process_rej_not_gof_message
   (const TokenMessage& message, const TokenMessage& parameters)
{
   Debug::ft("BaseBot.process_rej_not_gof_message");

   message.log("Unprocessed message");
}

//------------------------------------------------------------------------------

void BaseBot::process_rej_now_message
   (const TokenMessage& message, const TokenMessage& parameters)
{
   Debug::ft("BaseBot.process_rej_now_message");

   message.log("Unprocessed message");
}

//------------------------------------------------------------------------------

void BaseBot::process_rej_ord_message
   (const TokenMessage& message, const TokenMessage& parameters)
{
   Debug::ft("BaseBot.process_rej_ord_message");

   message.log("Unprocessed message");
}

//------------------------------------------------------------------------------

void BaseBot::process_rej_sco_message
   (const TokenMessage& message, const TokenMessage& parameters)
{
   Debug::ft("BaseBot.process_rej_sco_message");

   message.log("Unprocessed message");
}

//------------------------------------------------------------------------------

void BaseBot::process_rej_snd
   (const TokenMessage& message, const TokenMessage& parameters)
{
   Debug::ft("BaseBot.process_rej_snd");

   auto press = message.get_parm(1);
   remove_sent_press(press);
   process_rej_snd_message(message, parameters);
}

//------------------------------------------------------------------------------

void BaseBot::process_rej_snd_message
   (const TokenMessage& message, const TokenMessage& parameters)
{
   Debug::ft("BaseBot.process_rej_snd_message");

   message.log("Unprocessed message");
}

//------------------------------------------------------------------------------

void BaseBot::process_rej_sub_message
   (const TokenMessage& message, const TokenMessage& parameters)
{
   Debug::ft("BaseBot.process_rej_sub_message");

   message.log("Unprocessed message");
}

//------------------------------------------------------------------------------

void BaseBot::process_rej_tme_message
   (const TokenMessage& message, const TokenMessage& parameters)
{
   Debug::ft("BaseBot.process_rej_tme_message");

   message.log("Unprocessed message");
}

//------------------------------------------------------------------------------

void BaseBot::process_rm_message(const DipMessage& message)
{
   Debug::ft("BaseBot.process_rm_message");

   //  We've just connected to the server, so cancel the reconnection
   //  event and reset the initial timeout for reconnection attempts.
   //
   reconnect_ = true;
   cancel_event(RECONNECT_EVENT);
   retry_delay_ = 1;

   if(state_ != CONNECTING)
   {
      std::ostringstream stream;
      stream << "Unexpected RM received" << CRLF;
      send_to_console(stream);
   }

   set_state(CONNECTED);

   if(message.header.length > 0)
   {
      //  Remove the existing power and province names.
      //
      auto tokens = TokenTextMap::instance();
      tokens->erase_powers_and_provinces();

      //  Insert the new power and province names.  Each name is defined
      //  by a 6-byte parameter in which the first 2 bytes are the token
      //  and the last 4 bytes are its null-terminated name.
      //
      auto& rm = reinterpret_cast<const RM_Message&>(message);
      auto count = message.header.length / 6;

      for(auto i = 0; i < count; ++i)
      {
         tokens->insert(rm.pairs[i].token, rm.pairs[i].name);
      }
   }

   //  Send the NME or OBS.  If we're actually reconnecting, the server will
   //  reject this message and process_rej_nme_message will send an IAM.
   //
   send_nme_or_obs();
}

//------------------------------------------------------------------------------

void BaseBot::process_sco(const TokenMessage& message)
{
   Debug::ft("BaseBot.process_sco");

   if(report_) report_ords();
   map_and_units->process_sco(message);
   centres = map_and_units->get_centres();
   update_out_powers();
   process_sco_message(message);
}

//------------------------------------------------------------------------------

void BaseBot::process_sco_message(const TokenMessage& message)
{
   Debug::ft("BaseBot.process_sco_message");

   if(report_) report_sco();
}

//------------------------------------------------------------------------------

void BaseBot::process_slo_message(const TokenMessage& message)
{
   Debug::ft("BaseBot.process_slo_message");

   if(report_) report_end(message);
}

//------------------------------------------------------------------------------

void BaseBot::process_smr_message(const TokenMessage& message)
{
   Debug::ft("BaseBot.process_smr_message");

   if(report_) report_smr(message);
}

//------------------------------------------------------------------------------

void BaseBot::process_sve_message(const TokenMessage& message)
{
   Debug::ft("BaseBot.process_sve_message");

   send_to_server(Token(TOKEN_COMMAND_YES) & message);
}

//------------------------------------------------------------------------------

void BaseBot::process_thx_message(const TokenMessage& message)
{
   Debug::ft("BaseBot.process_thx_message");

   TokenMessage new_order;
   auto send_new_order = false;

   auto order = message.get_parm(1);
   auto unit = order.get_parm(0).enclose();
   auto note = message.get_parm(2).front();

   switch(note.all())
   {
   case TOKEN_ORDER_NOTE_MBV:
      //
      //  The order was OK, so there is nothing to do.
      //
      break;

   case TOKEN_ORDER_NOTE_FAR:
   case TOKEN_ORDER_NOTE_NSP:
   case TOKEN_ORDER_NOTE_NSU:
   case TOKEN_ORDER_NOTE_NAS:
   case TOKEN_ORDER_NOTE_NSF:
   case TOKEN_ORDER_NOTE_NSA:
      //
      //  Illegal movement order.  Replace with a hold order.
      //
      new_order = unit + Token(TOKEN_ORDER_HLD);
      send_new_order = true;
      break;

   case TOKEN_ORDER_NOTE_NVR:
      //
      //  Illegal retreat order.  Replace with a disband order.
      //
      new_order = unit + Token(TOKEN_ORDER_DSB);
      send_new_order = true;
      break;

   case TOKEN_ORDER_NOTE_YSC:
   case TOKEN_ORDER_NOTE_ESC:
   case TOKEN_ORDER_NOTE_HSC:
   case TOKEN_ORDER_NOTE_NSC:
   case TOKEN_ORDER_NOTE_CST:
      //
      //  Illegal build order.  Replace with a waive order.
      //
      new_order = unit.get_parm(0) + Token(TOKEN_ORDER_WVE);
      send_new_order = true;
      break;

   case TOKEN_ORDER_NOTE_NYU:
   case TOKEN_ORDER_NOTE_NRS:
      //
      //  An illegal order that we can't do anything about.
      //
      break;

   case TOKEN_ORDER_NOTE_NRN:
   case TOKEN_ORDER_NOTE_NMB:
   case TOKEN_ORDER_NOTE_NMR:
      //
      //  This order wasn't needed in the first place!
      //
      break;
   }

   std::ostringstream stream;

   if(send_new_order && (new_order != order))
   {
      stream << "Illegal order replaced:" << CRLF;
      stream << "reason=" << message.get_parm(2).to_str() << CRLF;
      stream << "old order=" << order.to_str() << CRLF;
      stream << "new order=" << new_order.to_str() << CRLF;
      send_to_console(stream);
      send_to_server(new_order);
   }
   else if(note != TOKEN_ORDER_NOTE_MBV)
   {
      stream << "Illegal order not replaced:" << CRLF;
      stream << "reason=" << message.get_parm(2).to_str() << CRLF;
      stream << "order=" << order.to_str() << CRLF;
      send_to_console(stream);
   }
}

//------------------------------------------------------------------------------

void BaseBot::process_tme_message(const TokenMessage& message) { }

//------------------------------------------------------------------------------

void BaseBot::process_unexpected_not_message(const TokenMessage& message) { }

//------------------------------------------------------------------------------

void BaseBot::process_unexpected_rej_message(const TokenMessage& message) { }

//------------------------------------------------------------------------------

void BaseBot::process_unexpected_rej_not_message
   (const TokenMessage& message) { }

//------------------------------------------------------------------------------

void BaseBot::process_unexpected_yes_message(const TokenMessage& message) { }

//------------------------------------------------------------------------------

void BaseBot::process_unexpected_yes_not_message
   (const TokenMessage& message) { }

//------------------------------------------------------------------------------

void BaseBot::process_yes(const TokenMessage& message)
{
   Debug::ft("BaseBot.process_yes");

   auto inner = message.get_parm(1);

   switch(message.get_parm(1).front().all())
   {
   case TOKEN_COMMAND_NME:
      set_title(TOKEN_COMMAND_NME, true);
      process_yes_nme_message(message, inner.get_parm(1));
      break;

   case TOKEN_COMMAND_OBS:
      set_title(TOKEN_COMMAND_OBS, true);
      process_yes_obs_message(message, inner.get_parm(1));
      break;

   case TOKEN_COMMAND_IAM:
      set_title(TOKEN_COMMAND_IAM, true);
      request_map();
      process_yes_iam_message(message, inner.get_parm(1));
      break;

   case TOKEN_COMMAND_NOT:
      process_yes_not(message, inner.get_parm(1));
      break;

   case TOKEN_COMMAND_GOF:
      process_yes_gof_message(message, inner.get_parm(1));
      break;

   case TOKEN_COMMAND_TME:
      process_yes_tme_message(message, inner.get_parm(1));
      break;

   case TOKEN_COMMAND_DRW:
      process_yes_drw_message(message, inner.get_parm(1));
      break;

   case TOKEN_COMMAND_SND:
      process_yes_snd(message, inner.get_parm(1));
      break;

   default:
      process_unexpected_yes_message(message);
   }
}

//------------------------------------------------------------------------------

void BaseBot::process_yes_drw_message
   (const TokenMessage& message, const TokenMessage& parameters) { }

//------------------------------------------------------------------------------

void BaseBot::process_yes_gof_message
   (const TokenMessage& message, const TokenMessage& parameters) { }

//------------------------------------------------------------------------------

void BaseBot::process_yes_iam_message
   (const TokenMessage& message, const TokenMessage& parameters) { }

//------------------------------------------------------------------------------

void BaseBot::process_yes_nme_message
   (const TokenMessage& message, const TokenMessage& parameters) { }

//------------------------------------------------------------------------------

void BaseBot::process_yes_not
   (const TokenMessage& message, const TokenMessage& yes_not_parameters)
{
   Debug::ft("BaseBot.process_yes_not");

   switch(yes_not_parameters.front().all())
   {
   case TOKEN_COMMAND_GOF:
      process_yes_not_gof_message(message, yes_not_parameters.get_parm(1));
      break;

   case TOKEN_COMMAND_DRW:
      process_yes_not_drw_message(message, yes_not_parameters.get_parm(1));
      break;

   case TOKEN_COMMAND_SUB:
      process_yes_not_sub_message(message, yes_not_parameters.get_parm(1));
      break;

   default:
      process_unexpected_yes_not_message(message);
   }
}

//------------------------------------------------------------------------------

void BaseBot::process_yes_not_drw_message
   (const TokenMessage& message, const TokenMessage& parameters) { }

//------------------------------------------------------------------------------

void BaseBot::process_yes_not_gof_message
   (const TokenMessage& message, const TokenMessage& parameters) { }

//------------------------------------------------------------------------------

void BaseBot::process_yes_not_sub_message
   (const TokenMessage& message, const TokenMessage& parameters) { }

//------------------------------------------------------------------------------

void BaseBot::process_yes_obs_message
   (const TokenMessage& message, const TokenMessage& parameters)
{
   Debug::ft("BaseBot.process_yes_obs_message");

   observer_ = true;
   report_ = true;
}

//------------------------------------------------------------------------------

void BaseBot::process_yes_snd
   (const TokenMessage& message, const TokenMessage& parameters)
{
   Debug::ft("BaseBot.process_yes_snd");

   auto press = message.get_parm(1);
   remove_sent_press(press);
   process_yes_snd_message(message, parameters);
}

//------------------------------------------------------------------------------

void BaseBot::process_yes_snd_message
   (const TokenMessage& message, const TokenMessage& parameters) { }

//------------------------------------------------------------------------------

void BaseBot::process_yes_tme_message
   (const TokenMessage& message, const TokenMessage& parameters) { }

//------------------------------------------------------------------------------

bool BaseBot::queue_event(BotEvent event, int secs)
{
   Debug::ft("BaseBot.queue_event");

   auto thread = Singleton<BotThread>::Instance();

   if(event == RECONNECT_EVENT)
   {
      if(secs == 0)
      {
         //  It's time to give up trying to connect to the server.
         //
         report_exit("cannot connect to server");
         return true;
      }
      else if(reconnect_ || (secs >= 4))
      {
         //  Provide a status update.
         //
         std::ostringstream stream;
         stream << "No connection to server" << CRLF;
         stream << "Will try again in " << secs << " seconds" << CRLF;
         send_to_console(stream);
      }
   }

   return thread->QueueEvent(event, secs);
}

//------------------------------------------------------------------------------

void BaseBot::reconnect()
{
   Debug::ft("BaseBot.reconnect");

   set_state(DISCONNECTED);
   auto rc = create_socket();

   if(rc != STARTUP_OK)
   {
      string reason("could not create socket [err=");
      reason += std::to_string(rc) + ']';
      report_exit(reason.c_str());
      return;
   }

   if(!config_.reconnect && (map_and_units->our_power != INVALID_TOKEN))
   {
      //  We already know what power we're playing, so save it and our
      //  passcode for the IAM that we hope to send next.
      //
      config_.power = map_and_units->our_power.to_str();
      config_.passcode = map_and_units->passcode;
   }

   send_im_message();
}

//------------------------------------------------------------------------------

uint8_t BaseBot::reconnection_delay()
{
   Debug::ft("BaseBot.reconnection_delay");

   //  The initial delay is 1 second.  Each time we try to reconnect, double
   //  it (which means that we wait 2 seconds the first time).  After we wait
   //  for 128 seconds, give up the next time.  That will allow 7 reconnect
   //  attempts during the course of just over 4 minutes.
   //
   if(retry_delay_ > 128) return 0;
   retry_delay_ <<= 1;
   return retry_delay_;
}

//------------------------------------------------------------------------------

void BaseBot::remove_sent_press(const TokenMessage& send_message)
{
   Debug::ft("BaseBot.remove_sent_press");

   auto recipients = send_message.get_parm(1);
   auto contents = send_message.get_parm(2);
   auto press = sent_press_.begin();

   while(press != sent_press_.end())
   {
      if((press->receiving_powers == recipients) &&
         (press->message == contents))
         press = sent_press_.erase(press);
      else
         ++press;
   }
}

//------------------------------------------------------------------------------

void BaseBot::report_ccd(const Token& power, bool disorder)
{
   Debug::ft("BaseBot.report_ccd");

   std::ostringstream stream;
   stream << power << " is ";
   stream << (disorder ? "now" : "no longer");
   stream << " in civil disorder." << CRLF;
   send_to_console(stream);
}

//------------------------------------------------------------------------------

void BaseBot::report_close(ProtocolError error) { }

//------------------------------------------------------------------------------

string BaseBot::report_command_line_parameters()
{
   Debug::ft("BaseBot.report_command_line_parameters");

   return string(EMPTY_STR);
}

//------------------------------------------------------------------------------

fn_name BaseBot_report_end = "BaseBot.report_end";

void BaseBot::report_end(const TokenMessage& message) const
{
   Debug::ft(BaseBot_report_end);

   std::ostringstream stream;

   auto signal = message.front().all();

   switch(signal)
   {
   case TOKEN_COMMAND_DRW:
   {
      TokenMessage powers;
      stream << "The game is over." << CRLF;
      stream << "It ended in a draw between ";

      if(message.has_nested_parms())
         powers = message.get_parm(1);
      else
         powers = surviving_powers(true);

      auto last = powers.size() - 1;

      for(size_t i = 0; i < last; ++i)
      {
         stream << powers.at(i);
         if(last > 1) stream << ", ";
      }

      stream << "and " << powers.at(last) << '.';
      break;
   }

   case TOKEN_COMMAND_OFF:
      if(!map_and_units->game_over)
      {
         stream << "The game is over." << CRLF;
         stream << "The server disconnected before a result was reached.";
      }
      break;

   case TOKEN_COMMAND_SLO:
      stream << "The game is over." << CRLF;
      stream << "It ended in a win for ";
      stream << message.get_parm(1).front() << '.';
      break;

   default:
      Debug::SwLog(BaseBot_report_end, "unexpected signal", signal);
      return;
   }

   stream << CRLF;
   send_to_console(stream);
}

//------------------------------------------------------------------------------

void BaseBot::report_exit(c_string reason)
{
   Debug::ft("BaseBot.report_exit");

   std::ostringstream stream;
   stream << "EXITING: " << reason << CRLF;
   send_to_console(stream);
   Singleton<BotThread>::Instance()->SetExit();
}

//------------------------------------------------------------------------------

void BaseBot::report_failed_connect() { }

//------------------------------------------------------------------------------

void BaseBot::report_failed_press
   (bool is_broadcast, TokenMessage& receiving_powers, TokenMessage& press) { }

//------------------------------------------------------------------------------

void BaseBot::report_mdf() const
{
   Debug::ft("BaseBot.report_mdf");

   std::ostringstream stream;
   stream << "The provinces (";
   stream << map_and_units->number_of_provinces;
   stream << ") and their neighbours are" << CRLF;

   for(ProvinceId p = 0; p < map_and_units->number_of_provinces; ++p)
   {
      auto& province = map_and_units->game_map[p];
      stream << province.token << SPACE;
      stream << (province.is_land ? "(land)" : "(sea)") << CRLF;

      auto& neighbours = province.neighbours;

      for(auto n = neighbours.cbegin(); n != neighbours.cend(); ++n)
      {
         stream << spaces(2) << n->first << " can move to:";

         for(auto loc = n->second.cbegin(); loc != n->second.cend(); ++loc)
         {
            stream << SPACE << *loc;
         }

         stream << CRLF;
      }
   }

   send_to_console(stream);
}

//------------------------------------------------------------------------------

void BaseBot::report_now() const
{
   Debug::ft("BaseBot.report_now");

   std::ostringstream stream;
   stream << "The current location of units is" << CRLF;

   for(size_t p = 0; p < units.size(); ++p)
   {
      auto& entry = units.at(p);

      if(!entry.units.empty())
      {
         stream << entry.power << " (";
         stream << entry.units.size() << "): ";
         auto last = entry.units.size() - 1;

         for(size_t u = 0; u <= last; ++u)
         {
            stream << entry.units.at(u).unit << SPACE;
            stream << entry.units.at(u).loc;
            if(u != last)
            {
               stream << ", ";
               if((u == 7) && (last > 8)) stream << CRLF << spaces(3);
               if((u == 17) && (last > 18)) stream << CRLF << spaces(3);
            }
         }

         stream << CRLF;
      }
   }

   send_to_console(stream);
}

//------------------------------------------------------------------------------

void BaseBot::report_ords()
{
   Debug::ft("BaseBot.report_ords");

   //  This is invoked when an SCO or NOW is received.  However, we don't
   //  want to report order results before the first move or twice during
   //  the same season.  So we only report them once, when a series of
   //  unreported ORDs have been received.
   //
   if(!ord_received_) return;
   ord_received_ = false;

   auto season = map_and_units->curr_season;
   auto year = map_and_units->curr_year;

   std::ostringstream stream;
   stream << "The adjudicated orders for the " << season;
   stream << " of " << year << " are" << CRLF;

   switch(season.all())
   {
   case TOKEN_SEASON_SPR:
   case TOKEN_SEASON_FAL:
   {
      auto powers = map_and_units->get_orders(season);

      for(auto p = powers.cbegin(); p != powers.cend(); ++p)
      {
         if(p->orders.empty()) continue;

         stream << p->power << ':' << CRLF;

         for(auto o = p->orders.cbegin(); o < p->orders.cend(); ++o)
         {
            stream << spaces(2) << map_and_units->display_movement_result(*o);
            stream << CRLF;
         }
      }
      break;
   }

   case TOKEN_SEASON_SUM:
   case TOKEN_SEASON_AUT:
   {
      auto powers = map_and_units->get_orders(season);

      for(auto p = powers.cbegin(); p != powers.cend(); ++p)
      {
         if(p->orders.empty()) continue;

         stream << p->power << ": ";

         for(auto o = p->orders.cbegin(); o < p->orders.cend(); ++o)
         {
            stream << map_and_units->display_retreat_result(*o);
            if(std::next(o) != p->orders.cend()) stream << ", ";
         }

         stream << CRLF;
      }
      break;
   }

   case TOKEN_SEASON_WIN:
   {
      for(PowerId p = 0; p < map_and_units->number_of_powers; ++p)
      {
         auto orders = map_and_units->prev_adjustments.find(p);

         if(orders != map_and_units->prev_adjustments.cend())
         {
            stream << power_token(p) << ": ";
            stream << orders->second;
            stream << CRLF;
         }
      }
      break;
   }

   default:
      stream << "Ill-formed ORD received: " << season.to_str();
      stream << "is not a season" << CRLF;
      send_to_console(stream);
      return;
   }

   send_to_console(stream);
}

//------------------------------------------------------------------------------

void BaseBot::report_out(const Token& power)
{
   Debug::ft("BaseBot.report_out");

   if(!power.is_power()) return;  // for UNO

   std::ostringstream stream;
   stream << power << " has been eliminated." << CRLF;
   send_to_console(stream);
}

//------------------------------------------------------------------------------

void BaseBot::report_sco() const
{
   Debug::ft("BaseBot.report_sco");

   std::ostringstream stream;
   stream << "The current ownership of supply centres is" << CRLF;

   for(size_t p = 0; p < centres.size(); ++p)
   {
      auto& entry = centres.at(p);

      if(!entry.centres.empty())
      {
         stream << entry.power << " (";
         stream << entry.centres.size() << "): ";
         auto last = entry.centres.size() - 1;

         for(size_t c = 0; c <= last; ++c)
         {
            stream << map_and_units->display_province(entry.centres.at(c));
            if(c != last) stream << ", ";
            if((c == 11) && (last > 12)) stream << CRLF << spaces(5);
            if((c == 22) && (last > 23)) stream << CRLF << spaces(5);
         }

         stream << CRLF;
      }
   }

   send_to_console(stream);
}

//------------------------------------------------------------------------------

void BaseBot::report_smr(const TokenMessage& message) const
{
   Debug::ft("BaseBot.report_smr");

   std::ostringstream stream;
   stream << "Game summary:" << CRLF;

   auto turn = message.get_parm(1);
   stream << "The game ended in the ";
   stream << turn.at(0) << " of " << turn.at(1) << '.' << CRLF;
   stream << "The participants were" << CRLF;

   for(PowerId p = 0; p < map_and_units->number_of_powers; ++p)
   {
      auto player = message.get_parm(p + 2);
      stream << player.front() << ": ";
      stream << "name: " << player.get_parm(1).to_str() << ", ";
      stream << "version: " << player.get_parm(2).to_str() << ", ";
      if(player.parm_count() == 4)
         stream << "centres: " << player.get_parm(3).to_str();
      else
         stream << "year eliminated: " << player.get_parm(4).to_str();
      stream << CRLF;
   }

   send_to_console(stream);
}

//------------------------------------------------------------------------------

void BaseBot::request_map()
{
   Debug::ft("BaseBot.request_map");

   map_requested_ = true;
   send_to_server(TokenMessage(TOKEN_COMMAND_MAP));
}

//------------------------------------------------------------------------------

void BaseBot::send_bm_message(const byte_t* payload, uint16_t length) const
{
   Debug::ft("BaseBot.send_bm_message");

   DipIpBufferPtr buff(new DipIpBuffer(MsgOutgoing, DipHeaderSize + length));
   buff->SetTxAddr(client_addr_);
   buff->SetRxAddr(client_addr_);

   auto bm = reinterpret_cast<DipMessage*>(buff->PayloadPtr());
   bm->header.signal = BM_MESSAGE;
   bm->header.spare = 0;
   bm->header.length = length;
   if(length > 0) memcpy(&bm->first_payload_byte, payload, length);

   FunctionGuard guard(Guard_MakeUnpreemptable);
   Singleton<BotThread>::Instance()->QueueMsg(buff);
}

//------------------------------------------------------------------------------

void BaseBot::send_broadcast_to_server(const TokenMessage& broadcast_message)
{
   Debug::ft("BaseBot.send_broadcast_to_server");

   TokenMessage receiving_powers;
   SentPressInfo press_record;

   //  Before sending the press message, save its details so that it can
   //  be retransmitted if necessary.
   //
   press_record.original_receiving_powers = receiving_powers;
   press_record.receiving_powers = active_powers();
   press_record.message = broadcast_message;
   press_record.resend_partial = true;
   press_record.is_broadcast = true;
   sent_press_.push_back(press_record);

   send_to_server(Token(TOKEN_COMMAND_SND) &
      receiving_powers & broadcast_message);
}

//------------------------------------------------------------------------------

bool BaseBot::send_buff(DipIpBuffer& buff)
{
   Debug::ft("BaseBot.send_buff");

   if(Debug::TraceOn())
   {
      auto tbuff = Singleton<TraceBuffer>::Instance();

      if(tbuff->ToolIsOn(DipTracer))
      {
         auto rec = new BotTrace(BotTrace::OgMsg, buff);
         tbuff->Insert(rec);
      }
   }

   return buff.Send(false);
}

//------------------------------------------------------------------------------

fn_name BaseBot_send_im_message = "BaseBot.send_im_message";

void BaseBot::send_im_message()
{
   Debug::ft(BaseBot_send_im_message);

   if(state_ != DISCONNECTED)
   {
      Debug::SwLog(BaseBot_send_im_message, "already connected", state_);
      return;
   }

   uint16_t length = sizeof(IM_Message);
   DipIpBufferPtr buff(new DipIpBuffer(MsgOutgoing, length));
   buff->SetTxAddr(client_addr_);
   buff->SetRxAddr(server_addr_);

   auto im = reinterpret_cast<IM_Message*>(buff->PayloadPtr());
   im->header.signal = IM_MESSAGE;
   im->header.spare = 0;
   im->header.length = length - DipHeaderSize;
   im->magic_number = 0xda10;
   im->version = 1;

   if(!send_buff(*buff))
   {
      std::ostringstream stream;
      stream << "Failed to send IM" << CRLF;
      send_to_console(stream);
      delete_socket(SOCKET_FAILED);
      buff.reset();
      return;
   }

   buff.reset();
   queue_event(RECONNECT_EVENT, reconnection_delay());
   set_state(CONNECTING);
}

//------------------------------------------------------------------------------

void BaseBot::send_nme(c_string name, c_string version)
{
   Debug::ft("BaseBot.send_nme");

   TokenMessage name_tokens;
   TokenMessage version_tokens;

   name_ = name;
   version_ = version;

   auto name_in_quotes = "'" + string(name) + "'";
   auto version_in_quotes = "'" + string(version) + "'";

   name_tokens.set_from(name_in_quotes);
   version_tokens.set_from(version_in_quotes);

   auto nme = Token(TOKEN_COMMAND_NME) & name_tokens & version_tokens;
   send_to_server(nme);
   set_title(TOKEN_COMMAND_NME, false);
}

//------------------------------------------------------------------------------

void BaseBot::send_nme_or_obs()
{
   Debug::ft("BaseBot.send_nme_or_obs");

   send_to_server(TOKEN_COMMAND_OBS);
   set_title(TOKEN_COMMAND_OBS, false);
}

//------------------------------------------------------------------------------

void BaseBot::send_orders_to_server()
{
   Debug::ft("BaseBot.send_orders_to_server");

   auto sub = map_and_units->build_sub();

   if(sub.size() > 1)
   {
      send_to_server(sub);
   }
}

//------------------------------------------------------------------------------

void BaseBot::send_press_to_server(const TokenMessage& press_to,
   const TokenMessage& press, bool resend_partial)
{
   Debug::ft("BaseBot.send_press_to_server");

   SentPressInfo press_record;

   //  Before sending the press message, save its details so that it can
   //  be retransmitted if necessary.
   //
   press_record.original_receiving_powers = press_to;
   press_record.receiving_powers = press_to;
   press_record.message = press;
   press_record.resend_partial = resend_partial;
   sent_press_.push_back(press_record);

   send_to_server(Token(TOKEN_COMMAND_SND) & press_to & press);
}

//------------------------------------------------------------------------------

void BaseBot::send_to_console(std::ostringstream& report)
{
   Debug::ft("BaseBot.send_to_console");

   report << CRLF;
   Singleton<CliThread>::Instance()->Inform(report);
}

//------------------------------------------------------------------------------

void BaseBot::send_to_reduced_powers
   (const SentPress::iterator& press_iter, const Token& inactive_power)
{
   Debug::ft("BaseBot.send_to_reduced_powers");

   TokenMessage reduced_powers;
   auto receiving_powers = press_iter->receiving_powers;

   for(size_t p = 0; p < receiving_powers.size(); ++p)
   {
      if(receiving_powers.at(p) != inactive_power)
      {
         reduced_powers = reduced_powers + receiving_powers.at(p);
      }
   }

   press_iter->receiving_powers = reduced_powers;
   send_to_server(Token(TOKEN_COMMAND_SND) &
      reduced_powers & press_iter->message);
}

//------------------------------------------------------------------------------

bool BaseBot::send_to_server(const TokenMessage& message)
{
   Debug::ft("BaseBot.send_to_server");

   if(state_ != CONNECTED)
   {
      message.log("Message discarded: not yet connected");
      return false;
   }

   auto count = message.size();
   auto length = count * sizeof(Token);
   DipIpBufferPtr buff(new DipIpBuffer(MsgOutgoing, DipHeaderSize + length));
   buff->SetTxAddr(client_addr_);
   buff->SetRxAddr(server_addr_);

   auto dm = reinterpret_cast<DM_Message*>(buff->PayloadPtr());
   dm->header.signal = DM_MESSAGE;
   dm->header.spare = 0;
   dm->header.length = length;
   message.get_tokens(reinterpret_cast<Token*>(&dm->tokens), count);

   if(!send_buff(*buff))
   {
      message.log("Failed to send DM");
      delete_socket(SOCKET_FAILED);
      buff.reset();
      return false;
   }

   buff.reset();
   return true;
}

//------------------------------------------------------------------------------

void BaseBot::set_state(ProtocolState state)
{
   Debug::ft("BaseBot.set_state");

   state_ = state;

   auto pos = title_.find_first_of(':');
   if(pos != string::npos) title_.erase(0, pos + 1);

   switch(state_)
   {
   case CONNECTING:
      title_.insert(0, "<-IM: ");
      break;
   case CONNECTED:
      title_.insert(0, "->RM: ");
      break;
   case DISCONNECTED:
      title_.insert(0, "DISCONNECTED: ");
      break;
   }

   SysConsole::SetTitle(title_);
}

//------------------------------------------------------------------------------

void BaseBot::set_title(token_t msg, bool rcvd)
{
   Debug::ft("BaseBot.set_title");

   //  Update the console's title to provide status information.
   //
   switch(msg)
   {
   case TOKEN_COMMAND_OBS:
      if(!rcvd)
         title_ = "<-OBS: " + name_ + SPACE + version_;
      else
         title_ = "OBS: " + name_ + SPACE + version_;
      break;

   case TOKEN_COMMAND_NME:
      if(!rcvd)
         title_ = "<-NME: " + name_ + SPACE + version_;
      else
         title_ = "NME: " + name_ + SPACE + version_;
      break;

   case TOKEN_COMMAND_IAM:
      if(!rcvd)
         title_ = "<-IAM: " + name_ + SPACE + version_;
      else
         title_ = "IAM: " + name_ + SPACE + version_;
      break;

   case TOKEN_COMMAND_HLO:
      if(!rcvd)
      {
         title_ = "<-HLO: " + name_ + SPACE + version_;
      }
      else if(!observer_)
      {
         title_ = map_and_units->our_power.to_str() + "(";
         title_ += std::to_string(map_and_units->passcode) + "): ";
         title_ += name_ + SPACE + version_;
      }
      break;

   default:
      return;
   }

   SysConsole::SetTitle(title_);
}

//------------------------------------------------------------------------------

TokenMessage BaseBot::surviving_powers(bool self) const
{
   Debug::ft("BaseBot.surviving_powers");

   TokenMessage result;

   for(PowerId p = 0; p < map_and_units->number_of_powers; ++p)
   {
      Token power(power_token(p));

      if((self || (power != map_and_units->our_power)) &&
         (out_powers.find(power) == out_powers.end()))
      {
         result = result + power;
      }
   }

   return result;
}

//------------------------------------------------------------------------------

void BaseBot::update_out_powers()
{
   Debug::ft("BaseBot.update_out_powers");

   //  If a power owns no centres, add it to out_powers.  If it
   //  wasn't already in the set, report its elimination.
   //
   for(auto c = centres.cbegin(); c != centres.cend(); ++c)
   {
      if(c->centres.empty())
      {
         auto item = out_powers.insert(c->power);

         if(item.second)
         {
            report_out(c->power);
         }
      }
   }
}
}
