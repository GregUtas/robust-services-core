# Robust Services Core: POTS Application

The POTS (Plain Ordinary Telephone Service) application simulates basic telephone
services.

## Running Tests

Many of the scripts in the [`input`](/input) directory are tests for the POTS application.
When the [`test.cp.setup`](/input/test.cp.setup.txt) script is run, the following files
are generated for each subsequent test:
1. A detailed function and message trace of the scenario.
2. A [message sequence chart](http://en.wikipedia.org/wiki/Message_sequence_chart) of the
scenario, followed by a context trace (a summary of socket activity, incoming and outgoing
messages, internal events, and the event handlers that were invoked).

The [`traffic`](/input/traffic.txt) script (`>read traffic`) launches a
[thread](/an/PotsTrafficThread.h) that initiates, answers, and releases calls, initially
at a rate of 120 per minute.  The call rate can be increased to the point where the system
enters overload, which on my PC occurs when the rate exceeds about 18,000 calls per minute
(`>traffic rate 18000`).  Whatever the current call rate, you can observe the system's
behavior with commands such as `>status`, `>sched show`, and `>traffic profile`.

## Configuring User Profiles

Users (phone numbers) are created in the `>pots` CLI increment.  The CLI commands
available in that increment are described [here](/docs/output/help.cli.txt),
starting after the line `pots>help full`.

Phone numbers are five digits in length, in the range 20000-99999.  *Supplementary services*,
which alter the behavior of basic calls, can also be assigned to each user.

### Supplementary Services
The POTS application currently defines the following supplementary services:
* **Barring of Incoming Calls (BIC)**: Incoming calls are blocked.
* **Barring of Outgoing Calls (BOC)**: Outgoing calls are blocked.
* **Call Forward Unconditional/Busy/No Reply (CFU/CFB/CFN)**: Incoming calls are
forwarded to a pre-specified number
  1. immediately (CFU),
  2. if the user is already involved in a call (CFB), or
  3. if the user does not answer before a specified timeout (CFN).
* **Call Transfer (CXF)**: When a user who has established a consultation or conference
call disconnects, the two remaining users are connected.  *Not yet implemented, but can
be assigned to a user's profile.*
* **Call Waiting (CWT)**: A user in an established call can receive a second call and
then repeatedly flip between the two calls.  If the user disconnects, the inactive
call rerings the user.
* **Hot Line (HTL)**: When the user initiates a call, it always routes to a pre-specified
number.
* **Suspended Service (SUS)**: The user cannot initiate or receive calls.
* **Three-Way Calling (TWC)**: A user in an established call can place the call on hold
and initiate a consultation call, which can then be conferenced with the original call.
*Not yet implemented, but can be assigned to a user's profile.*
* **Warm Line (WML)**: When a call is initiated and no digits are dialed before the timeout
interval, the call is routed to a pre-specified number.

## Design Overview
The [`SessionBase`](/sb) component of RCS defines virtual base classes for implementing state
machines and protocols.  As a session-oriented application, POTS uses this framework.  The
documents [*RCS-Session-Processing*](/docs/RSC-Session-Processing.pdf) and [*A Pattern Language
of Call Processing*](/docs/PLCP.pdf) should prove helpful if studying the POTS software in
detail.

The protocol between the user (client/phone) and network (server) is defined
[here](/pb/PotsProtocol.h).
On the network side, the POTS basic call state machine is based on the states and events defined
[here](/cb/BcSessions.h).  Its concrete state subclasses are defined [here](/sn/PotsSessions.h),
and its event handlers are implemented [here](/sn/PotsBcHandlers.cpp).
