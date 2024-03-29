# Robust Services Core: POTS Application

The POTS (Plain Ordinary Telephone Service) application simulates basic telephone
services.

## Running tests

Many of the scripts in the [_input_](/input) directory are tests for the POTS application.
When the [_test.cp.setup_](/input/test.cp.setup.txt) script is read, the following files
are generated during each test (see the files in the [_output_](/output) directory):

* A detailed function and message trace (_*.trace.txt_).
* A function profile (_*.funcs.txt_), as described in the [**Testing**](/README.md) section.
* A [message sequence chart](http://en.wikipedia.org/wiki/Message_sequence_chart) of the
scenario (_*.msc.txt_).  This is followed by an event trace (a summary of socket activity,
object creations/deletions, incoming and outgoing messages, and internal states and events).
The items in this event trace also appear in the function trace.
* A console file (_*.cli.txt_).

The [_traffic_](/input/traffic.start.txt) script (`>read traffic.start`) launches a
[thread](/src/an/PotsTrafficThread.h) that initiates, answers, and releases calls, initially
at a rate of 600 per minute.  The call rate can be increased to the point where the system
enters overload.  Whatever the current call rate, you can observe the system's behavior with
commands such as `>status`, `>sched show`, and `>traffic profile`.  A console file of a
traffic run appears [here](/output/traffic.console.txt), and a log file appears
[here](/output/traffic.logs.txt).  During the run, the call rate is suddenly
increased from 600 to 20,000 calls per minute.  Once this rate is reached, it is increased
to 24,000 calls per minute to create an overload situation.  After overload has persisted
for a while, the call rate is dropped to 0, which gradually causes all calls to be released.

## Configuring user profiles

Users (phone numbers) are created in the `>pots` CLI increment.  The CLI commands
available in that increment are described [here](/help/cli.txt),
starting after the line `pots>help full`.

Phone numbers are five digits in length, in the range 20000-99999.  _Supplementary services_,
which alter the behavior of basic calls, can also be assigned to each user.

### Supplementary services
The POTS application currently defines the following supplementary services:
* **Barring of Incoming Calls (BIC)**: Incoming calls are blocked.
* **Barring of Outgoing Calls (BOC)**: Outgoing calls are blocked.
* **Call Forward Unconditional/Busy/No Reply (CFU/CFB/CFN)**: Incoming calls are
forwarded to a pre-specified number
  1. immediately (CFU),
  2. if the user is already involved in a call (CFB), or
  3. if the user does not answer before a specified timeout (CFN).
* **Call Transfer (CXF)**: When a user who has established a consultation or conference
call disconnects, the two remaining users are connected.  _Not yet implemented, but can
be assigned to a user's profile._
* **Call Waiting (CWT)**: A user in an established call can receive a second call and
then repeatedly flip between the two calls.  If the user disconnects, the inactive
call rerings the user.
* **Hot Line (HTL)**: When the user initiates a call, it always routes to a pre-specified
number.
* **Suspended Service (SUS)**: The user cannot initiate or receive calls.
* **Three-Way Calling (TWC)**: A user in an established call can place the call on hold
and initiate a consultation call, which can then be conferenced with the original call.
_Not yet implemented, but can be assigned to a user's profile._
* **Warm Line (WML)**: When a call is initiated and no digits are dialed before the timeout
interval, the call is routed to a pre-specified number.

## Design overview
The [`SessionBase`](/src/sb) component of RCS defines virtual base classes for implementing state
machines and protocols.  As a session-oriented application, POTS uses this framework.  The
documents [_RSC Session Processing_](/docs/RSC-Session-Processing.pdf) and [_A Pattern Language
of Call Processing_](/docs/PLCP.pdf) should prove helpful if studying the POTS software in
detail.

The protocol between the user (client/phone) and network (server) is defined
[here](/src/pb/PotsProtocol.h).
On the network side, the POTS basic call state machine is based on the states and events defined
[here](/src/cb/BcSessions.h).  Its concrete state subclasses are defined [here](/src/sn/PotsSessions.h),
and its event handlers are implemented [here](/src/sn/PotsBcHandlers.cpp).
