//==============================================================================
//
//  BaseBot.h
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
#ifndef BASEBOT_H_INCLUDED
#define BASEBOT_H_INCLUDED

#include "Base.h"
#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <list>
#include <set>
#include <string>
#include <vector>
#include "DipProtocol.h"
#include "DipTypes.h"
#include "StartupParameters.h"
#include "SysIpL3Addr.h"
#include "SysTypes.h"
#include "TokenMessage.h"

namespace Diplomacy
{
   class MapAndUnits;
   struct PowerCentres;
   struct PowerUnits;
}

using namespace NodeBase;

//------------------------------------------------------------------------------
//
namespace Diplomacy
{
//  Base class for Diplomacy bots.  See BotType.h for how to install your
//  subclass.
//
class BaseBot : public Base
{
public:
   //  Values returned from initialise().  Bot-specific values should start
   //  at FIRST_BOT_SPECIFIC_ERROR.
   //
   enum StartupResult
   {
      STARTUP_OK,
      SERVER_ADDRESS_LOOKUP_FAILED,
      SERVER_PROTOCOL_INCORRECT,
      FAILED_TO_ALLOCATE_PORT,
      FAILED_TO_ALLOCATE_SOCKET,
      FIRST_BOT_SPECIFIC_ERROR
   };

   //  Public to allow instantiation of a default bot.
   //
   BaseBot();

   //  Virtual to allow subclassing.
   //
   virtual ~BaseBot() = default;

   //  Returns the bot's singleton instance.
   //
   static BaseBot* instance();

   //  Initialises the bot.
   //
   StartupResult initialise();

   //  Processes the incoming MESSAGE.
   //
   void process_message(const DipMessage& message);

   //  Handles a command line parameter that is specific to this bot.  TOKEN
   //  is the character after the '-', and VALUE is the string that follows it.
   //  Return true if the parameter is recognized.  Returning false produces a
   //  log that includes the documentation in report_command_line_parameters
   //  (see below).  The default version returns false.
   //
   virtual bool process_command_line_parameter(char token, std::string& value);

   //  Returns a string containing the description of command line parameters
   //  specific to a bot.  Invoked when process_command_line_parameter returns
   //  false.  The default version returns an empty string.
   //
   virtual std::string report_command_line_parameters();

   //  Adds an endline to REPORT and displays it on the console.
   //
   static void send_to_console(std::ostringstream& report);

   //  Overridden to display the bot's data.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;
protected:
   //  Sets report_ when a bot wants to enable/disable the output of game
   //  updates to the console.
   //
   void set_report(bool onoff) { report_ = onoff; }

   //  Sends MESSAGE to the server, wrapped in a DM.  Returns true if the
   //  message was successfully sent.
   //
   bool send_to_server(const TokenMessage& message);

   //  Sends the orders to the server.  See set_..._order() in MapAndUnits.
   //
   void send_orders_to_server();

   //  Sends an NME (the bot's name and version) to the server.
   //
   void send_nme(c_string name, c_string version);

   //  Requests a copy of the map.  Gets a MAP and MDF, but doesn't YES(MAP())
   //  in response.  Done automatically if joining a game via HLO or OBS, but
   //  not if via IAM (since we may be rejoining following connection loss).
   //
   void request_map();

   //  Determines if an MAP or MDF was by request.  Only valid in
   //  process_map_message and process_mdf_message.
   //
   bool map_requested() const { return map_requested_; }

   //  Sends press to the server and adds it to a database of sent press.
   //  If RESEND_PARTIAL is set, or when broadcast press is sent, it is
   //  automatically retransmitted if we are notified that a power went
   //  into civil disorder or was eliminated before the server responded
   //  to the press.
   //
   void send_broadcast_to_server(const TokenMessage& broadcast_message);
   void send_press_to_server(const TokenMessage& press_to,
      const TokenMessage& press, bool resend_partial);

   //  Queues a BM message for the bot.  PAYLOAD[0 to LENGTH-1] is placed
   //  in the message immediately after the header.  LENGTH is in bytes.
   //
   void send_bm_message(const byte_t* payload, uint16_t length) const;

   //  Sends an FM or EM message to the server to close the connection,
   //  depending on whether ERROR is provided.
   //
   void disconnect_from_server(ProtocolError error = GRACEFUL_CLOSE);

   //  Returns a message containing the powers that are neither eliminated
   //  nor in civil disorder.  The message is not parenthesised, so operator&
   //  should be used to add it to a message.  Unless SELF is set, the power
   //  being played by the bot is not included in the result.
   //
   TokenMessage active_powers(bool self = false) const;

   //  Returns a message containing the powers that have not been eliminated.
   //  The message is not parenthesised, so operator& should be used to add it
   //  to a message.  Unless SELF is set, the power being played by the bot is
   //  not included in the result.
   //
   TokenMessage surviving_powers(bool self = false) const;

   //  Queues an EVENT that will arrive in SECS.
   //
   void queue_event(BotEvent event, int secs);

   //  Cancels EVENT if it exists.  If more than one such event is pending,
   //  only the one that would occur first is cancelled.
   //
   void cancel_event(BotEvent event);

   /////////////////////////////////////////////////////////////////////////////
   //
   //  Any of the virtual functions below may be overridden, although the base
   //  class default version may also be invoked if it does what the bot wants.
   //
   /////////////////////////////////////////////////////////////////////////////

   //  Invoked by initialise() to initialise the subclass.  The default simply
   //  returns STARTUP_OK.
   //
   virtual StartupResult initialise(const StartupParameters& parameters);

   //  Invoked to update the console's title when MSG is sent or acknowledged
   //  by the server.  This provides basic status information.
   //
   virtual void set_title(token_t msg, bool rcvd);

   //  Sends an OBS to the server.  A bot that wishes to play a power must
   //  override this by invoking send_nme instead.
   //
   virtual void send_nme_or_obs();

   //  Handles an incoming CCD.  The default does nothing but is invoked
   //  after cd_powers has been updated.
   //
   virtual void process_ccd_message
      (const TokenMessage& message, bool is_new_disconnection);

   //  Handles an incoming DRW.  The default does nothing but is invoked
   //  after the game is marked over.
   //
   virtual void process_drw_message(const TokenMessage& message);

   //  Lists the tokens that the bot understands.  The default returns
   //  an empty list, so a bot should override this.
   //
   virtual const std::vector<Token>& get_try_tokens() const;

   //  Handles an incoming FRM.  The default replies with HUH(message)
   //  and a TRY containing the tokens returned by get_try_tokens.
   //
   virtual void process_frm_message(const TokenMessage& message);

   //  Handles an incoming HUH.  The default logs the message.
   //
   virtual void process_huh_message(const TokenMessage& message);

   //  Handles an incoming LOD.  The default replies with REJ(LOD(...)).
   //
   virtual void process_lod_message(const TokenMessage& message);

   //  Handles an incoming MIS.  The default does nothing.
   //
   virtual void process_mis_message(const TokenMessage& message);

   //  Handles an incoming OFF.  The default disconnects from the server.
   //
   virtual void process_off_message(const TokenMessage& message);

   //  Handles an incoming OUT.  The default does nothing but is invoked
   //  after out_powers has been updated.
   //
   virtual void process_out_message(const TokenMessage& message);

   //  Handles an incoming PRN.  The default logs the message.
   //
   virtual void process_prn_message(const Token* message, size_t size);

   //  Handles an incoming SMR.  The default does nothing.
   //
   virtual void process_smr_message(const TokenMessage& message);

   //  Handles an incoming SVE.  The default replies with YES(SVE(...)).
   //
   virtual void process_sve_message(const TokenMessage& message);

   //  Handles an incoming THX.  If the result wasn't MBV, the default
   //  provides a Hold, Disband, or Waive order depending on the season.
   //
   virtual void process_thx_message(const TokenMessage& message);

   //  Handles an incoming TME.  The default does nothing.
   //
   virtual void process_tme_message(const TokenMessage& message);

   //  Handles an incoming ADM.  The default does nothing.
   //
   virtual void process_adm_message(const TokenMessage& message);

   //  Handles an incoming NOT(CCD()).  The default does nothing but is
   //  invoked after cd_powers has been updated.
   //
   virtual void process_not_ccd_message(const TokenMessage& message,
      const TokenMessage& parameters, bool is_new_reconnection);

   //  Handles an incoming NOT(TME()).  The default does nothing.
   //
   virtual void process_not_tme_message
      (const TokenMessage& message, const TokenMessage& parameters);

   //  Gets the details to reconnect to the game.  Returns true to try to
   //  reconnect, or false otherwise.  The default uses the reconnection
   //  parameters that were supplied on the command line.
   //
   virtual bool get_reconnect_details(Token& power, int& passcode) const;

   //  Invoked when the connection to the server either could not be
   //  established or was closed during the game.  Returns the number of
   //  seconds until reconnection should be attempted.  Returns 0 if the
   //  bot should exit instead of reconnecting, which occurs after many
   //  connection attempts have failed.
   //
   virtual uint8_t reconnection_delay();

   //  Handle an incoming REJ(NME()).  If get_reconnect_details supplies a
   //  valid power and passcode, the default sends an IAM to the server.
   //
   virtual void process_rej_nme_message
      (const TokenMessage& message, const TokenMessage& parameters);

   //  Handles an incoming REJ(IAM()).  The default logs the message.
   //
   virtual void process_rej_iam_message
      (const TokenMessage& message, const TokenMessage& parameters);

   //  Handles an incoming REJ(HLO()).  The default logs the message.
   //
   virtual void process_rej_hlo_message
      (const TokenMessage& message, const TokenMessage& parameters);

   //  Handles an incoming REJ(NOW()).  The default logs the message.
   //
   virtual void process_rej_now_message
      (const TokenMessage& message, const TokenMessage& parameters);

   //  Handles an incoming REJ(SCO()).  The default logs the message.
   //
   virtual void process_rej_sco_message
      (const TokenMessage& message, const TokenMessage& parameters);

   //  Handles an incoming REJ(HST()).  The default logs the message.
   //
   virtual void process_rej_hst_message
      (const TokenMessage& message, const TokenMessage& parameters);

   //  Handles an incoming REJ(SUB()).  The default logs the message.
   //
   virtual void process_rej_sub_message
      (const TokenMessage& message, const TokenMessage& parameters);

   //  Handles an incoming REJ(GOF()).  The default logs the message.
   //
   virtual void process_rej_gof_message
      (const TokenMessage& message, const TokenMessage& parameters);

   //  Handles an incoming REJ(ORD()).  The default logs the message.
   //
   virtual void process_rej_ord_message
      (const TokenMessage& message, const TokenMessage& parameters);

   //  Handles an incoming REJ(TME()).  The default logs the message.
   //
   virtual void process_rej_tme_message
      (const TokenMessage& message, const TokenMessage& parameters);

   //  Handles an incoming REJ(DRW()).  The default logs the message.
   //
   virtual void process_rej_drw_message
      (const TokenMessage& message, const TokenMessage& parameters);

   //  Handles an incoming REJ(SND()).  The default logs the message.
   //
   virtual void process_rej_snd_message
      (const TokenMessage& message, const TokenMessage& parameters);

   //  Handles an incoming REJ(ADM()).  The default does nothing.
   //
   virtual void process_rej_adm_message
      (const TokenMessage& message, const TokenMessage& parameters);

   //  Handles an incoming REJ(MIS()).  The default does nothing.
   //
   virtual void process_rej_mis_message
      (const TokenMessage& message, const TokenMessage& parameters);

   //  Handles an incoming REJ(NOT(GOF())).  The default logs the message.
   //
   virtual void process_rej_not_gof_message
      (const TokenMessage& message, const TokenMessage& parameters);

   //  Handles an incoming REJ(NOT(DRW())).  The default logs the message.
   //
   virtual void process_rej_not_drw_message
      (const TokenMessage& message, const TokenMessage& parameters);

   //  Handles an incoming YES(NME()).  The default does nothing.
   //
   virtual void process_yes_nme_message
      (const TokenMessage& message, const TokenMessage& parameters);

   //  Handles an incoming YES(OBS()).  The default does nothing.
   //
   virtual void process_yes_obs_message
      (const TokenMessage& message, const TokenMessage& parameters);

   //  Handles an incoming YES(IAM()).  The default does nothing but is
   //  invoked after requesting the map.
   //
   virtual void process_yes_iam_message
      (const TokenMessage& message, const TokenMessage& parameters);

   //  Handles an incoming YES(GOF()).  The default does nothing.
   //
   virtual void process_yes_gof_message
      (const TokenMessage& message, const TokenMessage& parameters);

   //  Handles an incoming YES(TME()).  The default does nothing.
   //
   virtual void process_yes_tme_message
      (const TokenMessage& message, const TokenMessage& parameters);

   //  Handles an incoming YES(DRW()).  The default does nothing.
   //
   virtual void process_yes_drw_message
      (const TokenMessage& message, const TokenMessage& parameters);

   //  Handles an incoming YES(SND()).  The default does nothing but is
   //  invoked after removing the acknowledged press from sent_press_.
   //
   virtual void process_yes_snd_message
      (const TokenMessage& message, const TokenMessage& parameters);

   //  Handles an incoming YES(NOT(GOF())).  The default does nothing.
   //
   virtual void process_yes_not_gof_message
      (const TokenMessage& message, const TokenMessage& parameters);

   //  Handles an incoming YES(NOT(DRW())).  The default does nothing.
   //
   virtual void process_yes_not_drw_message
      (const TokenMessage& message, const TokenMessage& parameters);

   //  Handles an incoming YES(NOT(SUB())).  The default does nothing.
   //
   virtual void process_yes_not_sub_message
      (const TokenMessage& message, const TokenMessage& parameters);

   //  Handles an incoming NOT that contains an unexpected parameter.  The
   //  default does nothing.
   //
   virtual void process_unexpected_not_message(const TokenMessage& message);

   //  Handles an incoming REJ that contains an unexpected parameter.  The
   //  default does nothing.
   //
   virtual void process_unexpected_rej_message(const TokenMessage& message);

   //  Handles an incoming REJ(NOT()) that contains an unexpected parameter.
   //  The default does nothing.
   //
   virtual void process_unexpected_rej_not_message(const TokenMessage& message);

   //  Handles an incoming YES that contains an unexpected parameter.  The
   //  default does nothing.
   //
   virtual void process_unexpected_yes_message(const TokenMessage& message);

   //  Handles an incoming YES(NOT()) that contains an unexpected parameter.
   //  The default does nothing.
   //
   virtual void process_unexpected_yes_not_message(const TokenMessage& message);

   //  Handles an incoming BM.  See send_bm_message().  The default logs the
   //  message.
   //
   virtual void process_bm_message(const DipMessage& message);

   /////////////////////////////////////////////////////////////////////////////
   //
   //  The following do nothing but are invoked after internal information has
   //  been updated for the bot's use.
   //
   /////////////////////////////////////////////////////////////////////////////

   //  Handles an incoming HLO after saving the power to be played.
   //
   virtual void process_hlo_message(const TokenMessage& message);

   //  Handles an incoming MAP after saving the map's name and sending
   //  an MDF to the server.
   //
   virtual void process_map_message(const TokenMessage& message);

   //  Handles a MDF after saving the map definition.
   //
   virtual void process_mdf_message(const TokenMessage& message);

   //  Handles an ORD after saving the order results.
   //
   virtual void process_ord_message(const TokenMessage& message);

   //  Handles an SCO after updating supply centre ownership.
   //
   virtual void process_sco_message(const TokenMessage& message);

   //  Handles a NOW after saving the new position.
   //
   virtual void process_now_message(const TokenMessage& message);

   //  Handles an incoming SLO after marking the game over.
   //
   virtual void process_slo_message(const TokenMessage& message);

   /////////////////////////////////////////////////////////////////////////////
   //
   //  The following are invoked to notify the bot of various events.
   //
   /////////////////////////////////////////////////////////////////////////////

   //  Notifies the bot that an attempt to send press has failed.
   //
   virtual void report_failed_press
      (bool is_broadcast, TokenMessage& receiving_powers, TokenMessage& press);

   //  Notifies the bot that the initial connection could not be established.
   //  Further attempts to connect will be made.
   //
   virtual void report_failed_connect();

   //  Notifies the bot that the connection has been closed because of ERROR.
   //  If ERROR is SOCKET_FAILED, attempts will be made to reconnect.
   //
   virtual void report_close(ProtocolError error);

   /////////////////////////////////////////////////////////////////////////////
   //
   //  The following are invoked by an observer bot to report on the game.
   //  They may also be invoked by a bot that is playing a power.
   //
   /////////////////////////////////////////////////////////////////////////////

   //  Reports that a power has entered/exited civil disorder.
   //
   static void report_ccd(const Token& power, bool disorder);

   //  Reports that the game ended (DRW, SLO, or OFF).
   //
   void report_end(const TokenMessage& message) const;

   //  Reports the map configuration.
   //
   void report_mdf() const;

   //  Reports the current position.
   //
   void report_now() const;

   //  Reports order results for the current turn.
   //
   void report_ords();

   //  Reports that a power has been eliminated from the game.
   //
   static void report_out(const Token& power);

   //  Reports supply centre ownership.
   //
   void report_sco() const;

   //  Reports the game summary.
   //
   void report_smr(const TokenMessage& message) const;

   //  Reports that the bot is exiting for REASON.
   //
   void report_exit(c_string reason);

   /////////////////////////////////////////////////////////////////////////////
   //
   //  Data accessible to a derived bot.
   //
   /////////////////////////////////////////////////////////////////////////////

   //  A pointer to the game map and position.
   //
   MapAndUnits* const map_and_units;

   //  The powers that are currently in civil disorder.  Updated when
   //  a CCD or NOT(CCD) arrives.
   //
   std::set<Token> cd_powers;

   //  The powers that have been eliminated.  Updated when an SCO or
   //  OUT arrives.
   //
   std::set<Token> out_powers;

   //  The centres owned by each power.  Updated when an SCO arrives.
   //
   std::vector<PowerCentres> centres;

   //  The units owned by each power.  Updated when a NOW arrives.
   //
   std::vector<PowerUnits> units;
private:
   //  The state of the Diplomacy AI protocol.
   //
   enum ProtocolState
   {
      CONNECTING,   // IM sent; waiting for RM
      CONNECTED,    // RM received; sending and receiving DMs
      DISCONNECTED  // startup / sent or received FM or EM /
   };               // received OFF / socket failed

   //  Updates the bot's protocol state and the console's title.
   //
   void set_state(ProtocolState state);

   //  Sends an IM to the server to connect for the first time.
   //
   void send_im_message();

   //  Handles an incoming RM.
   //
   void process_rm_message(const DipMessage& message);

   //  Handles an incoming DM.
   //
   void process_dm_message(const DipMessage& message);

   //  Handles an incoming FM.
   //
   void process_fm_message(const DipMessage& message);

   //  Handles an incoming EM.
   //
   void process_em_message(const DipMessage& message);

   //  Handles an incoming message before invoking the corresponding
   //  process_xxx_message function that the bot can override.  This
   //  ensures that internal data is updated before the bot sees the
   //  message.  In other cases, press is deleted after being accepted
   //  or rejected, or is retransmitted during race conditions where we
   //  are notified that a power has gone into civil disorder or been
   //  eliminated after we sent press that will presently be rejected
   //  for having been sent to an incommunicado power.
   //
   void process_mdf(const TokenMessage& message);
   void process_sco(const TokenMessage& message);
   void process_now(const TokenMessage& message);
   void process_ord(const TokenMessage& message);
   void process_ccd(const TokenMessage& message);
   void process_not_ccd
      (const TokenMessage& message, const TokenMessage& parameters);
   void process_out(const TokenMessage& message);
   void process_yes(const TokenMessage& message);
   void process_rej(const TokenMessage& message);
   void process_not(const TokenMessage& message);
   void process_yes_not
      (const TokenMessage& message, const TokenMessage& yes_not_parameters);
   void process_rej_not
      (const TokenMessage& message, const TokenMessage& rej_not_parameters);
   void process_yes_snd
      (const TokenMessage& message, const TokenMessage& parameters);
   void process_rej_snd
      (const TokenMessage& message, const TokenMessage& parameters);

   //  For saving an instance of sent press.
   //
   struct SentPressInfo
   {
      //  The press message itself.
      //
      TokenMessage message;

      //  The original recipients.
      //
      TokenMessage original_receiving_powers;

      //  The recipients the most recent time that the press was sent.
      //
      TokenMessage receiving_powers;

      //  Set to attempt a retransmission if the press is rejected.
      //
      bool resend_partial;

      //  Set if the press was broadcast to all active powers.
      //
      bool is_broadcast;

      //  Constructor.
      //
      SentPressInfo() : resend_partial(false), is_broadcast(false) { }
   };

   //  For maintaining a database of sent press.
   //
   typedef std::list<SentPressInfo> SentPress;

   //  If the database of unacknowledged press includes any press that was
   //  sent to INACTIVE_POWER (which has just gone into civil disorder or
   //  been eliminated), retransmit that message to the other recipients if
   //  originally requested to do so, or report that the press was rejected
   //  (which will happen presently).
   //
   void check_sent_press_for_inactive_power(const Token& inactive_power);

   //  Retransmits a press message after removing inactive_power from the
   //  list of recipients.
   //
   void send_to_reduced_powers(const SentPress::iterator& press_iter,
      const Token& inactive_power);

   //  Removes press from the database of pending press once the server
   //  has either acknowledged or rejected it.
   //
   void remove_sent_press(const TokenMessage& send_message);

   //  Updates out_powers when a SCO arrives.
   //
   void update_out_powers();

   //  Sets client_addr_ and server_addr_ from the startup parameters.
   //  Returns 0 on success; anything else is a failure.
   //
   StartupResult get_ipaddrs();

   //  Allocates the socket for communicating with the server.
   //
   StartupResult create_socket();

   //  Sends BUFF after recording it if tracing is on.  Returns false if
   //  BUFF could not be sent.
   //
   static bool send_buff(DipIpBuffer& buff);

   //  Releases the socket, enters the DISCONNECTED state, and invokes
   //  report_close.
   //
   void delete_socket(ProtocolError error);

   //  Attempts reconnection to the server.
   //
   void reconnect();

   //  Set once initialise() has been executed so that it will not be
   //  executed again if the bot is reentered after an exception.
   //
   bool initialised_;

   //  Startup parameters from the command line.
   //
   StartupParameters config_;

   //  The client's IP address and port number.
   //
   NetworkBase::SysIpL3Addr client_addr_;

   //  The client's IP address and port number, and the socket used to
   //  communicate with the server.
   //
   NetworkBase::SysIpL3Addr server_addr_;

   //  The current protocol state.
   //
   ProtocolState state_;

   //  How many reconnection attempts have been made, either initially
   //  or after losing the connection to the server.
   //
   uint8_t retries_;

   //  The title currently displayed on the console.
   //
   std::string title_;

   //  The bot's name.
   //
   std::string name_;

   //  The bot's version.
   //
   std::string version_;

   //  Set once a successful connection is established to the server,
   //  after which a connection failure causes a reconnection attempt.
   //
   bool reconnect_;

   //  Set if running as an observer.
   //
   bool observer_;

   //  Set to report results on the console.
   //
   bool report_;

   //  Set when an ORD is received.  Cleared when report_ords is
   //  invoked.
   //
   bool ord_received_;

   //  Set if a copy of the map has been requested.
   //
   bool map_requested_;

   //  The message containing the map name for the game.
   //
   TokenMessage map_message_;

   //  Press messages sent by the bot.
   //
   SentPress sent_press_;
};
}
#endif
