# Robust Services Core: POTS Application

The POTS (Plain Ordinary Telephone Service) application simulates basic telephone
services.

## Tests

Many of the scripts in the [`input/`](/input) directory are tests for the POTS application.
When the `test.cp.setup` script is run before tests, the following files will be generated
for each test:
1. A detailed function and message trace of the scenario.
2. A [message sequence chart](http://en.wikipedia.org/wiki/Message_sequence_chart) of the
scenario, followed by a context trace (a summary of socket activity, incoming and outgoing
messages, internal events, and the event handlers that were invoked).

## Configuring Users

Users (phone numbers) are created in the `>pots` CLI increment.  The CLI commands
available in the POTS increment are described in [RSC-CLI-Commands](/docs/RSC-CLI-Commands.pdf),
starting after the line `pots>help full`.

Phone numbers are five digits in length, in the range 20000-99999.  *Supplementary services*,
which alter the behavior of basic calls, can also be assigned to each user.

### Supplementary Services
The POTS application currently defines the following supplementary services:
* **Barring of Incoming Calls (BIC)**: Incoming calls are blocked.
* **Barring of Outgoing Calls (BOC)**: Outgoing calls are blocked.
* **Call Forward Unconditional/Busy/No Reply (CFU, CFB, CFN)**: Incoming calls are
forwarded to a pre-specified number
  1. immediately (CFU),
  2. if the user is already involved in a call (CFB), or
  3. if the user does not answer before a specified timeout (CFN).
* **Call Transfer (CXF)** (*not yet implemented*): When a user who has established
a consultation or conference call disconnects, the two remaining users are connected.
* **Call Waiting (CWT)**: A user in an established call can receive a second call and
then repeatedly flip between the two calls.  If the user disconnects, the inactive
call rerings him.
* **Hot Line (HTL)**: When the user initiates a call, it always routes to a pre-specified
number.
* **Suspended Service (SUS)**: The user cannot initiate or receive calls.
* **Three-Way Calling (TWC)** (*not yet implemented*): A user in an established call can
place the call on hold and initiate a consultation call, which can then be conferenced with
the original call.
* **Warm Line (WML)**: When a call is initiated and no digits are dialed before the timeout
interval, the call is routed to a pre-specified number.
