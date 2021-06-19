//==============================================================================
//
//  MscBuilder.cpp
//
//  Copyright (C) 2013-2021  Greg Utas
//
//  This file is part of the Robust Services Core (RSC).
//
//  RSC is free software: you can redistribute it and/or modify it under the
//  terms of the GNU General Public License as published by the Free Software
//  Foundation, either version 3 of the License, or (at your option) any later
//  version.
//
//  RSC is distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
//  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
//  details.
//
//  You should have received a copy of the GNU General Public License along
//  with RSC.  If not, see <http://www.gnu.org/licenses/>.
//
#include "MscBuilder.h"
#include <algorithm>
#include <sstream>
#include "Algorithms.h"
#include "Debug.h"
#include "FactoryRegistry.h"
#include "Formatters.h"
#include "LocalAddress.h"
#include "MscAddress.h"
#include "MscContextPair.h"
#include "Protocol.h"
#include "ProtocolRegistry.h"
#include "SbTrace.h"
#include "Singleton.h"
#include "TraceBuffer.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace SessionTools
{
//  Formatting constants.
//
fixed_string MscHeader  = "MESSAGE SEQUENCE CHART";
fixed_string MscTrailer = "END OF MSC";

const MscColumn FirstCol = (ColWidth / 2);  // column for first vertical line
const size_t MinMsgLine = 5;                // minimum length of horizontal line
const size_t TimeGap = 3;               // spacing between event times
const size_t TimeLen = 9;               // length of an event time (mm:ss.msecs)

const char IdleCtx   = ':';
const char ActiveCtx = '|';
const char MsgLine   = '-';
const char MsgLeft   = '<';
const char MsgRight  = '>';
const char ErrorFlag = '?';

//------------------------------------------------------------------------------

MscBuilder::MscBuilder(bool debug) :
   debug_(debug),
   nextEvent_(0),
   group_(0),
   lines_(0),
   nextCol_(FirstCol),
   lastCol_(0),
   nextRow_(0),
   stream_(nullptr)
{
   Debug::ft("MscBuilder.ctor");

   contextq_.Init(MscContext::LinkDiff());
   addressq_.Init(MscAddress::LinkDiff());
   pairq_.Init(MscContextPair::LinkDiff());

   for(auto i = 0; i < MaxEvents; ++i) events_[i] = nullptr;
}

//------------------------------------------------------------------------------

MscBuilder::~MscBuilder()
{
   Debug::ftnt("MscBuilder.dtor");

   //  Delete all of the data that was allocated to build the MSC.
   //
   contextq_.Purge();
   addressq_.Purge();
   pairq_.Purge();
}

//------------------------------------------------------------------------------

void MscBuilder::AddEvent(const TraceRecord* rec)
{
   Debug::ft("MscBuilder.AddEvent");

   if(nextEvent_ < MaxEvents)
   {
      events_[nextEvent_++] = rec;
   }
}

//------------------------------------------------------------------------------

void MscBuilder::AddRow(const string& s)
{
   Debug::ft("MscBuilder.AddRow");

   if(nextRow_ < MaxRows)
   {
      rows_[nextRow_++] = s.c_str();
   }
}

//------------------------------------------------------------------------------

TraceRc MscBuilder::AnalyzeEvents()
{
   Debug::ft("MscBuilder.AnalyzeEvents");

   MscContext* ctx = nullptr;
   const TransTrace* tt;

   for(size_t i = 0; i < nextEvent_; ++i)
   {
      auto rec = events_[i];

      switch(rec->Owner())
      {
      case TransTracer:
         //
         //  Add this context to the MSC to build its set of vertical lines.
         //
         tt = static_cast< const TransTrace* >(rec);
         ctx = EnsureContext(*tt);
         break;

      case ContextTracer:
         //
         //  Only MsgTrace events have been extracted.
         //
         auto mt = static_cast< const MsgTrace* >(rec);

         if(mt->NoCtx())
         {
            //  If a message was not sent from a context, it could have been
            //  sent by the CLI thread (injected on behalf of a factory) or
            //  the timer thread (a timeout).  Timeouts are handled when they
            //  arrive.  For the former, create a context and add the factory
            //  to it if the message is internal.
            //
            if(!mt->Self() && (mt->Route() == Message::Internal))
            {
               auto reg = Singleton< FactoryRegistry >::Instance();
               auto fac = reg->GetFactory(mt->LocAddr().fid);

               if(fac != nullptr)
               {
                  ctx = EnsureContext(fac, mt->LocAddr().fid);
                  EnsureAddr(*mt, ctx);
                  ctx = nullptr;
               }
            }
         }
         else
         {
            //  There should be a context.  Add the PSM or factory
            //  that sent or received this message to that context.
            //
            if(ctx != nullptr) EnsureAddr(*mt, ctx);
         }

         //  Join the receiver and sender.
         //
         JoinAddrs(*mt);
         break;
      }
   }

   //  If we didn't find any contexts, there is nothing to display.
   //
   if(contextq_.Empty()) return NothingToDisplay;

   return TraceOk;
}

//------------------------------------------------------------------------------

MscContext* MscBuilder::ColumnToContext(MscColumn column) const
{
   Debug::ft("MscBuilder.ColumnToContext");

   for(auto c = contextq_.First(); c != nullptr; contextq_.Next(c))
   {
      if(c->Group() != group_) continue;
      if(c->Column() == column) return c;
   }

   return nullptr;
}

//------------------------------------------------------------------------------

void MscBuilder::Compress()
{
   Debug::ft("MscBuilder.Compress");

   MscColumn leftGap;   // columns to delete at left of message rows
   MscColumn midGap;    // columns to delete in middle of header rows
   MscColumn rightGap;  // columns to delete at right of message rows
   MscColumn start;     // starting position for string searches
   MscColumn end;       // ending position for string searches
   MscColumn blank0;    // first blank (after START) in header row 0
   MscColumn text0;     // first non-blank (after START) in header row 0
   MscColumn blank1;    // first blank (after START) in header row 1
   MscColumn text1;     // first non-blank (after START) in header row 1

   //  Delete blanks on the left side of the MSC.
   //
   blank0 = rows_[0].find_first_not_of(SPACE, 0);
   blank1 = rows_[1].find_first_not_of(SPACE, 0);
   leftGap = std::min(blank0, blank1);

   if(leftGap > 0)
   {
      for(size_t row = 0; row < nextRow_; ++row)
      {
         auto& str = rows_[row];
         if(str.front() == ErrorFlag) continue;
         rows_[row].erase(0, leftGap);
      }

      ReduceColumns(0, leftGap);
   }

   //  Most of the work involves compressing the space between contexts.
   //
   for(size_t ctx = 0; ctx < lines_ - 1; ++ctx)
   {
      start = columns_[ctx];
      end = columns_[ctx + 1];

      //  For the two header rows, find the number of blanks between
      //  START and END.  The midGap will be the smaller of these.
      //
      blank0 = rows_[0].find(SPACE, start);
      text0 = rows_[0].find_first_not_of(SPACE, blank0);
      blank1 = rows_[1].find(SPACE, start);
      text1 = rows_[1].find_first_not_of(SPACE, blank1);

      midGap = std::min(text0 - blank0, text1 - blank1) - 2;

      //  Determine how much compression can be done at the left and right
      //  ends of the message lines.  The leftGap and rightGap will be
      //  the length of the smallest lines that precede and follow the
      //  message labels.  These lengths are further reduced to maintain
      //  a "--" at the sending end and a "->" at the receiving end.
      //
      leftGap = (ColWidth - MinMsgLine) / 2;  // max value
      rightGap = (ColWidth - MinMsgLine) / 2;  // max value

      for(size_t row = 2; row < nextRow_; ++row)
      {
         auto& str = rows_[row];
         if(str.front() == ErrorFlag) continue;

         if((str[start + 1] != SPACE) && (str[end - 1] != SPACE))
         {
            text0 = str.find_first_not_of(MsgLine, start + 3);
            leftGap = std::min(leftGap, text0 - (start + 3));
            text1 = str.find_last_not_of(MsgLine, end - 3);
            rightGap = std::min(rightGap, (end - 3) - text1);
         }
      }

      //  If the size of the leftGap plus the rightGap exceeds the midGap,
      //  reduce them so that they add to the same size as the midGap.
      //
      auto count = (leftGap + rightGap) - midGap;

      if(count > 0)
      {
         leftGap -= ((count + 1) / 2);
         rightGap -= (count / 2);
      }

      //  Compress the space between the header rows and delete unneeded
      //  columns between the START and END columns of the remaining rows.
      //
      rows_[0].erase(blank0, leftGap + rightGap);
      rows_[1].erase(blank1, leftGap + rightGap);

      for(size_t row = 2; row < nextRow_; ++row)
      {
         auto& str = rows_[row];
         if(str.front() == ErrorFlag) continue;
         rows_[row].erase(end - (rightGap + 2), rightGap);
         rows_[row].erase(start + 3, leftGap);
      }

      ReduceColumns(start, leftGap + rightGap);
   }

   //  Finally, remove blanks to the right of the final context.  Originally
   //  lastCol_ was the end of the longest of the first two lines, which show
   //  context names.  Now everything has shifted left, so recalculate the
   //  new END.  Then find the first non-blank character to the right of END
   //  in row 2.  This is the beginning of the string "time", which we want
   //  to *finish* at TimeGap + TimeLen blanks to the right of END.
   //
   start = columns_[lines_ - 1];
   blank0 = rows_[0].find(SPACE, start);
   blank1 = rows_[1].find(SPACE, start);
   end = std::max(blank0, blank1);

   text1 = rows_[1].find_first_not_of(SPACE, end);
   rightGap = text1 - (end + TimeGap + TimeLen - 4);

   for(size_t row = 0; row < nextRow_; ++row)
   {
      if(rows_[row].size() > size_t(end))
      {
         rows_[row].erase(end, rightGap);
      }
   }
}

//------------------------------------------------------------------------------

size_t MscBuilder::CountContexts() const
{
   Debug::ft("MscBuilder.CountContexts");

   size_t count = 0;

   for(auto c = contextq_.First(); c != nullptr; contextq_.Next(c))
   {
      if(c->Group() == group_) ++count;
   }

   return count;
}

//------------------------------------------------------------------------------

void MscBuilder::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   stream << CRLF;

   Temporary::Display(stream, prefix, options);

   stream << prefix << "debug     : " << debug_ << CRLF;
   stream << prefix << "nextEvent : " << nextEvent_ << CRLF;
   stream << prefix << "group     : " << group_ << CRLF;
   stream << prefix << "lines     : " << lines_ << CRLF;
   stream << prefix << "nextCol   : " << nextCol_ << CRLF;
   stream << prefix << "lastCol   : " << lastCol_ << CRLF;
   stream << prefix << "nextRow   : " << nextRow_ << CRLF;

   stream << prefix << "columns   : ";
   for(size_t i = 0; i < lines_; ++i) stream << columns_[i] << SPACE;
   stream << CRLF;

   auto lead1 = prefix + spaces(2);
   auto lead2 = prefix + spaces(4);

   stream << prefix << "addressq : " << CRLF;
   for(auto a = addressq_.First(); a != nullptr; addressq_.Next(a))
   {
      stream << lead1 << ObjSeparatorStr << CRLF;
      a->Display(stream, lead2, NoFlags);
   }

   stream << prefix << "contextq : " << CRLF;
   for(auto c = contextq_.First(); c != nullptr; contextq_.Next(c))
   {
      stream << lead1 << ObjSeparatorStr << CRLF;
      c->Display(stream, lead2, NoFlags);
   }

   stream << prefix << "pairq : " << CRLF;
   for(auto p = pairq_.First(); p != nullptr; pairq_.Next(p))
   {
      stream << lead1 << ObjSeparatorStr << CRLF;
      p->Display(stream, lead2, NoFlags);
   }
}

//------------------------------------------------------------------------------

void MscBuilder::EnsureAddr(const MsgTrace& mt, MscContext* context)
{
   Debug::ft("MscBuilder.EnsureAddr");

   if(mt.LocAddr().fid == NIL_ID) return;

   auto loc = FindAddr(mt.LocAddr());

   if(loc == nullptr)
      addressq_.Enq(*new MscAddress(mt, context));
   else
      loc->SetPeer(mt, context);
}

//------------------------------------------------------------------------------

MscContext* MscBuilder::EnsureContext(const Factory* fac, FactoryId fid)
{
   Debug::ft("MscBuilder.EnsureContext(fac)");

   auto ctx = FindContext(fac, fid);
   if(ctx != nullptr) return ctx;

   ctx = new MscContext(fac, SingleMsg, fid);
   contextq_.Enq(*ctx);
   return ctx;
}

//------------------------------------------------------------------------------

MscContext* MscBuilder::EnsureContext(const TransTrace& trans)
{
   Debug::ft("MscBuilder.EnsureContext(trans)");

   //  When an SSM context is created, its root SSM is not created until the
   //  first transaction.  The root SSM's identifier must therefore be set
   //  later.
   //
   auto ctx = FindContext(trans.Rcvr(), trans.Cid());

   if(ctx != nullptr)
   {
      if((trans.Type() == MultiPort) && trans.Service())
      {
         ctx->SetCid(trans.Cid());
      }

      return ctx;
   }

   auto type = trans.Type();
   auto cid = trans.Cid();
   if((type == MultiPort) && !trans.Service()) cid = NIL_ID;
   ctx = new MscContext(trans.Rcvr(), type, cid);
   contextq_.Enq(*ctx);
   return ctx;
}

//------------------------------------------------------------------------------

void MscBuilder::EnsureFactories()
{
   Debug::ft("MscBuilder.EnsureFactories");

   //  There are three situations in which an address (PSM or factory) will
   //  not have a peer:
   //  1. The address was communicating internally with a factory whose context
   //     was not found in the trace.  If so, create a context here.
   //  2. The address was communicating internally with a PSM whose context was
   //     not found in the trace.  In this case the MSC will show the peer
   //     PSM's factory instead, so ensure that this factory has a context.
   //  3. The address was communicating externally.  Ensure that the external
   //     factory has a context.
   //
   auto reg = Singleton< FactoryRegistry >::Instance();

   for(auto addr = addressq_.First(); addr != nullptr; addressq_.Next(addr))
   {
      auto peer = addr->RemAddr();

      if(peer.fid != NIL_ID)
      {
         if(FindAddr(peer) == nullptr)
         {
            auto fac = reg->GetFactory(peer.fid);

            if(FindContext(fac, peer.fid) == nullptr)
            {
               auto ctx = EnsureContext(fac, peer.fid);
               JoinContexts(*addr->Context(), *ctx);
            }
         }
      }

      FactoryId fid;

      if(addr->ExternalFid(fid))
      {
         EnsureContext(nullptr, fid);
      }
   }
}

//------------------------------------------------------------------------------

fn_name MscBuilder_Error = "MscBuilder.Error";

bool MscBuilder::Error(const string& errstr, debug64_t errval)
{
   Debug::ft(MscBuilder_Error);

   Debug::SwLog(MscBuilder_Error, errstr, errval);
   *stream_ << ErrorFlag;
   *stream_ << SPACE << errstr;
   *stream_ << "; errval=" << strHex(errval);
   debug_ = true;
   return false;
}

//------------------------------------------------------------------------------

const Flags TTmask = Flags(1 << TransTracer);
const Flags CTmask = Flags(1 << ContextTracer);

TraceRc MscBuilder::ExtractEvents()
{
   Debug::ft("MscBuilder.ExtractEvents");

   auto buff = Singleton< TraceBuffer >::Instance();
   auto mask = (TTmask | CTmask);
   TraceRecord* rec = nullptr;

   //  Iterate through all trace records, selecting the following:
   //  o Transactions ("RXNET" and "TRANS") captured by TransTracer.
   //  o Messages ("rxmsg" and "txmsg") captured by ContextTracer.
   //
   for(buff->Next(rec, mask); rec != nullptr; buff->Next(rec, mask))
   {
      auto rid = rec->Rid();

      switch(rec->Owner())
      {
      case TransTracer:
         AddEvent(rec);
         break;

      case ContextTracer:
         switch(rid)
         {
         case MsgTrace::Transmission:
         case MsgTrace::Reception:
            AddEvent(rec);
         }
      }
   }

   if(nextEvent_ == 0) return NothingToDisplay;
   return TraceOk;
}

//------------------------------------------------------------------------------

bool MscBuilder::ExtractGroup() const
{
   Debug::ft("MscBuilder.ExtractGroup");

   bool found = false;

   //  If there is an internal context that does not yet belong to a group,
   //  make it the first member of the current group.
   //
   for(auto c = contextq_.First(); c != nullptr; contextq_.Next(c))
   {
      if(c->SetGroup(group_))
      {
         found = true;
         break;
      }
   }

   if(!found) return false;

   //  Iterate through the pairs of communicating contexts to find those
   //  that are transitively paired with this one.  Add these contexts to
   //  the current group: they will appear in the next MSC.
   //
   while(found)
   {
      found = false;

      for(auto p = pairq_.First(); p != nullptr; pairq_.Next(p))
      {
         MscContext* c1, *c2;

         p->Contexts(c1, c2);

         if(c1->Group() == group_)
         {
            if(c2->SetGroup(group_)) found = true;
         }

         if(c2->Group() == group_)
         {
            if(c1->SetGroup(group_)) found = true;
         }
      }
   }

   //  Clear the group for all external contexts.
   //
   for(auto c = contextq_.First(); c != nullptr; contextq_.Next(c))
   {
      c->ClearGroup();
   }

   //  Include, in the current group, all external contexts that communicate
   //  with any address in the group.
   //
   for(auto addr = addressq_.First(); addr != nullptr; addressq_.Next(addr))
   {
      FactoryId fid;

      if((addr->Context()->Group() == group_) && addr->ExternalFid(fid))
      {
         auto ctx = FindContext(nullptr, fid);
         ctx->SetGroup(group_);
      }
   }

   return true;
}

//------------------------------------------------------------------------------

MscAddress* MscBuilder::FindAddr(const LocalAddress& locAddr) const
{
   Debug::ft("MscBuilder.FindAddr");

   for(auto a = addressq_.First(); a != nullptr; addressq_.Next(a))
   {
      if(a->LocAddr() == locAddr) return a;
   }

   return nullptr;
}

//------------------------------------------------------------------------------

MscContext* MscBuilder::FindContext(const void* rcvr, id_t cid) const
{
   Debug::ft("MscBuilder.FindContext");

   for(auto c = contextq_.First(); c != nullptr; contextq_.Next(c))
   {
      if(c->IsEqualTo(rcvr, cid)) return c;
   }

   return nullptr;
}

//------------------------------------------------------------------------------

MscAddress* MscBuilder::FindPeer(const LocalAddress& locAddr) const
{
   Debug::ft("MscBuilder.FindPeer");

   auto addr = FindAddr(locAddr);
   if(addr == nullptr) return nullptr;
   return FindAddr(addr->RemAddr());
}

//------------------------------------------------------------------------------

const MsgTrace* MscBuilder::FindRxMsg(size_t index) const
{
   Debug::ft("MscBuilder.FindRxMsg");

   auto rxnet = static_cast< const TransTrace* >(events_[index]);
   auto trans = static_cast< const TransTrace* >(nullptr);

   for(size_t i = index + 1; i < nextEvent_; ++i)
   {
      auto rec = events_[i];

      switch(rec->Owner())
      {
      case TransTracer:
         //
         //  See if a new transaction has started and if its SbIpBuffer
         //  pointer matches the one for "RXNET".  If it does, the next
         //  "rxmsg" event is the one we're looking for.
         //
         if(rec->Rid() == TransTrace::Trans)
         {
            trans = static_cast< const TransTrace* >(rec);

            if(trans->Buff() != rxnet->Buff()) trans = nullptr;
         }
         break;

      case ContextTracer:
         //
         //  If TRANS is not nullptr, this is the event we're looking for.
         //
         if((trans != nullptr) && (rec->Rid() == MsgTrace::Reception))
         {
            return static_cast< const MsgTrace* >(rec);
         }
         break;
      }
   }

   return nullptr;
}

//------------------------------------------------------------------------------

const TransTrace* MscBuilder::FindTrans(size_t index) const
{
   Debug::ft("MscBuilder.FindTrans");

   auto txmsg = static_cast< const MsgTrace* >(events_[index]);

   //  If the message was not internal, don't bother to look for the
   //  transaction that processed it.
   //
   if(txmsg->Route() != Message::Internal) return nullptr;

   //  Track the most recent "TRANS" event and return it when we stumble on
   //  an "rxmsg" event whose remote address matches TXMSG's local address.
   //
   auto trans = static_cast< const TransTrace* >(nullptr);

   for(size_t i = index + 1; i < nextEvent_; ++i)
   {
      auto rec = events_[i];

      switch(rec->Owner())
      {
      case TransTracer:
         if(rec->Rid() == TransTrace::Trans)
         {
            trans = static_cast< const TransTrace* >(rec);
         }
         break;

      case ContextTracer:
         if(rec->Rid() == MsgTrace::Reception)
         {
            auto rxmsg = static_cast< const MsgTrace* >(rec);

            if(rxmsg->RemAddr() == txmsg->LocAddr()) return trans;
         }
         break;
      }
   }

   return nullptr;
}

//------------------------------------------------------------------------------

TraceRc MscBuilder::Generate(ostream& stream)
{
   Debug::ft("MscBuilder.Generate");

   //  Find the trace records needed to build an MSC.  These records
   //  are used until the MSC is completed, so make sure they don't
   //  get overwritten.
   //
   stream_ = &stream;

   auto buff = Singleton< TraceBuffer >::Instance();
   auto rc = TraceOk;

   buff->Lock();
   {
      do
      {
         rc = ExtractEvents();
         if(rc != TraceOk) break;

         //  Create the list of contexts, which correspond to vertical
         //  lines in an MSC.
         //
         rc = AnalyzeEvents();
         if(rc != TraceOk) break;

         //  If a PSM was communicating internally but doesn't have a peer
         //  PSM, ensure that the factory with which it was communicating
         //  has a context in the MSC.
         //
         EnsureFactories();

         //  Output a header, followed by one or more MSCs, and finally a
         //  trailer. More than one MSC results from displaying disjoint
         //  MSCs separately.
         //
         OutputHeader();

         for(group_ = 1; (ExtractGroup() == true); ++group_)
         {
            OutputChart();
         }

         OutputTrailer();
      }
      while(false);
   }
   buff->Unlock();

   if(debug_)
   {
      *stream_ << CRLF << "INTERNAL DATA:" << CRLF;
      Output(*stream_, 0, true);
   }

   return rc;
}

//------------------------------------------------------------------------------

void MscBuilder::JoinAddrs(const MsgTrace& mt)
{
   Debug::ft("MscBuilder.JoinAddrs");

   if(mt.Route() != Message::Internal) return;

   auto addr1 = FindAddr(mt.LocAddr());
   if(addr1 == nullptr) return;
   auto addr2 = FindAddr(mt.RemAddr());
   if(addr2 == nullptr) return;
   JoinContexts(*addr1->Context(), *addr2->Context());
}

//------------------------------------------------------------------------------

void MscBuilder::JoinContexts(MscContext& context1, MscContext& context2)
{
   Debug::ft("MscBuilder.JoinContexts");

   for(auto p = pairq_.First(); p != nullptr; pairq_.Next(p))
   {
      if(p->IsEqualTo(context1, context2)) return;
   }

   pairq_.Enq(*new MscContextPair(context1, context2));
}

//------------------------------------------------------------------------------

void MscBuilder::OutputChart()
{
   Debug::ft("MscBuilder.OutputChart");

   nextRow_ = 0;

   //  Determine the number of vertical lines in the MSC, which is the
   //  number of contexts in the current group plus a context for each
   //  external factory that participated in a dialog with a member of
   //  the group.
   //
   lines_ = CountContexts();

   //  The must be at least 2 lines_ (two communicating contexts, whether
   //  internal or external) and no more than the maximum.
   //
   if((lines_ <= 1) || (lines_ > MaxCols))
   {
      Error("invalid context count", lines_);
      AddRow(OutputFiller(nullptr));
      return;
   }

   //  Assign columns to all internal contexts and record the location of
   //  each column.
   //
   SetContextColumns();

   for(size_t i = 0, col = FirstCol; i < lines_; ++i, col += ColWidth)
   {
      columns_[i] = col;
   }

   //  Display the current group's contexts and one row in which no context
   //  is active.
   //
   OutputGroup();
   AddRow(OutputFiller(nullptr));

   const TransTrace* tt;
   const MsgTrace* mt;
   MscContext* ctx = nullptr;

   for(size_t i = 0; i < nextEvent_; ++i)
   {
      auto rec = events_[i];
      auto rid = rec->Rid();

      switch(rec->Owner())
      {
      case TransTracer:
         tt = static_cast< const TransTrace* >(rec);

         switch(rid)
         {
         case TransTrace::RxNet:
            //
            //  This event occurs when a message arrives over the IP stack,
            //  even if sent from within the same processor.  Look ahead in
            //  the trace buffer to find the "rxmsg" event that occurs in
            //  the same context, with the same protocol and signal.  This
            //  event contains the info required to display the message.
            //
            mt = FindRxMsg(i);
            if(mt != nullptr) OutputMessage(nullptr, *mt, tt);
            break;

         case TransTrace::Trans:
            //
            //  This simply changes the current context.
            //
            ctx = FindContext(tt->Rcvr(), 0);
            break;
         }
         break;

      case ContextTracer:
         //
         //  Display messages when they are sent.  An internal message
         //  bypasses the IP stack, so it has no RxNet event.  It must
         //  be displayed here.  Messages sent over the IP stack go to
         //  the "External Initiator" or "External Receiver", even if
         //  intraprocessor.  In the intraprocessor case, the message
         //  will also show up above, in an RxNet event.
         //
         if(rid == MsgTrace::Transmission)
         {
            mt = static_cast< const MsgTrace* >(rec);
            tt = FindTrans(i);
            OutputMessage(ctx, *mt, tt);
         }
         break;
      }
   }

   //  Generate a final row in which no context is active.  Output the rows
   //  after performing compression and inserting a blank line to separate
   //  the MSC from the header or the previous MSC.
   //
   AddRow(OutputFiller(nullptr));
   Compress();
   *stream_ << CRLF;
   for(size_t row = 0; row < nextRow_; ++row) *stream_ << rows_[row] << CRLF;
}

//------------------------------------------------------------------------------

string MscBuilder::OutputFiller(const MscContext* active) const
{
   Debug::ft("MscBuilder.OutputFiller");

   //  Generate a "blank" line in the MSC.  It contains a vertical line for
   //  each context, with a different type of line for the ACTIVE context.
   //
   string line(FirstCol + ((lines_ - 1) * ColWidth) + 1, SPACE);

   for(size_t col = FirstCol, n = lines_; n > 0; col += ColWidth, --n)
   {
      if((active != nullptr) && (active->Column() == col))
         line[col] = ActiveCtx;
      else
         line[col] = IdleCtx;
   }

   return line;
}

//------------------------------------------------------------------------------

void MscBuilder::OutputGroup()
{
   Debug::ft("MscBuilder.OutputGroup");

   MscColumn col = FirstCol;
   string text1, text2;
   std::ostringstream line1, line2;

   //  Display the context associated with each column.
   //
   for(auto c = ColumnToContext(col); c != nullptr; c = ColumnToContext(col))
   {
      c->Names(text1, text2);
      line1 << strCenter(text1, ColWidth, 1);
      line2 << strCenter(text2, ColWidth, 1);
      col += ColWidth;
   }

   auto right1 = line1.tellp();
   auto right2 = line2.tellp();
   lastCol_ = std::max(right1, right2);
   line1 << string(lastCol_ - right1 + TimeGap, SPACE);
   line1 << "    txmsg" << spaces(TimeGap);
   line1 << "    RXNET" << spaces(TimeGap);
   line1 << "    TRANS";
   line2 << string(lastCol_ - right2 + TimeGap, SPACE);
   line2 << "     time" << spaces(TimeGap);
   line2 << "     time" << spaces(TimeGap);
   line2 << "     time";

   AddRow(line1.str());
   AddRow(line2.str());
}

//------------------------------------------------------------------------------

void MscBuilder::OutputHeader() const
{
   Debug::ft("MscBuilder.OutputHeader");

   auto buff = Singleton< TraceBuffer >::Instance();
   *stream_ << MscHeader << buff->strTimePlace() << CRLF;
}

//------------------------------------------------------------------------------

bool MscBuilder::OutputMessage
   (const MscContext* active, const MsgTrace& mt, const TransTrace* tt)
{
   Debug::ft("MscBuilder.OutputMessage");

   string txmsgTime, rxnetTime, transTime;
   MscColumn start, end;

   if(mt.Rid() == MsgTrace::Transmission)
   {
      //  This is an outgoing message.  It starts at the sender's context.
      //
      auto txaddr = FindAddr(mt.LocAddr());
      if(txaddr == nullptr)
      {
         return Error("txaddr not found", pack2(mt.Prid(), mt.Sid()));
      }

      auto sender = txaddr->Context();
      if(sender->Group() != group_) return true;

      if(mt.NoCtx()) active = sender;
      start = sender->Column();
      txmsgTime = mt.GetTime(EMPTY_STR);
      if(tt != nullptr) transTime = tt->GetTime(EMPTY_STR);

      //  If the message was sent to self, the sender is also the receiver.
      //  The message will start one column to the sender's left and end at
      //  the sender's context.  (If the sender is the leftmost column, the
      //  message will start one column to its right).
      //
      if(mt.Self())
      {
         end = start;

         if(start >= ColWidth)
            start -= ColWidth;
         else
            start += ColWidth;
      }
      else
      {
         MscContext* receiver;

         if(mt.Route() == Message::Internal)
         {
            //  This is an intraprocessor message.  If the receiver is a PSM,
            //  find its context.  If the receiver is a factory, then either
            //  the receiver truly is a factory or this is an initial message
            //  to another PSM, in which case the message created that PSM and
            //  did not know its address at the time it was sent.  In either
            //  case, use the peer that we have recorded for the sender.
            //
            MscAddress* rxaddr;

            if(mt.RemAddr().bid != NIL_ID)
            {
               rxaddr = FindAddr(mt.RemAddr());
               if(rxaddr == nullptr)
               {
                  return Error("rxaddr not found", pack2(mt.Prid(), mt.Sid()));
               }
            }
            else
            {
               rxaddr = FindPeer(mt.LocAddr());
               if(rxaddr == nullptr)
               {
                  return Error("peer not found", pack2(mt.Prid(), mt.Sid()));
               }
            }

            receiver = rxaddr->Context();
         }
         else
         {
            //  An interprocessor message arrives at the remote factory.
            //
            receiver = FindContext(nullptr, mt.RemAddr().fid);
         }

         if(receiver == nullptr)
         {
            return Error("receiver not found", pack2(mt.Prid(), mt.Sid()));
         }

         if(receiver->Group() != group_)
         {
            return Error("receiver group invalid", pack2(mt.Prid(), mt.Sid()));
         }

         end = receiver->Column();
      }
   }
   else
   {
      //  This is a message arriving over the IP stack.  It starts at
      //  an external context and ends at the receiver's context.
      //
      auto rxaddr = FindAddr(mt.LocAddr());
      if(rxaddr == nullptr)
      {
         return Error("rxaddr not found", pack2(mt.Prid(), mt.Sid()));
      }

      auto receiver = rxaddr->Context();
      if(receiver == nullptr)
      {
         return Error("receiver not found", pack2(mt.Prid(), mt.Sid()));
      }

      if(receiver->Group() != group_) return true;

      auto sender = FindContext(nullptr, mt.RemAddr().fid);

      if(sender == nullptr)
      {
         return Error("sender not found", pack2(mt.Prid(), mt.Sid()));
      }

      if(sender->Group() != group_)
      {
         return Error("invalid sender group", pack2(mt.Prid(), mt.Sid()));
      }

      start = sender->Column();
      end = receiver->Column();
      rxnetTime = tt->GetTime(EMPTY_STR);
      transTime = mt.GetTime(EMPTY_STR);
   }

   //  Find the message's signal so that it can be displayed.  Strip out
   //  the word "Signal".
   //
   auto pro = Singleton< ProtocolRegistry >::Instance()->GetProtocol(mt.Prid());
   if(pro == nullptr)
      return Error("protocol not found", pack2(mt.Prid(), mt.Sid()));

   auto label = strClass(pro->GetSignal(mt.Sid()), false);
   auto index = label.find("Signal");
   if(index != string::npos) label.erase(index, 6);

   //  The size of the message name must be 5 less than the column width.
   //  This allows for a vertical line, a "<-" at the receiving end, and
   //  a "--" at the sending end (e.g. |--Label->: for width=10).
   //
   auto width = std::min(label.size(), ColWidth - MinMsgLine);

   //  Generate a "filler" line and overwrite part of it with the message.
   //
   auto line = OutputFiller(active);

   //  Determine the direction in which the message will travel.
   //
   if(start != end)
   {
      if(start > end)
      {
         line[end + 1] = MsgLeft;
         start -= 1;
         end += 2;
         for(auto col = end; col <= start; ++col)
         {
            line[col] = MsgLine;
         }

         auto pad = ((ColWidth - MinMsgLine) - width) / 2;
         start = (start - 1) - (width + pad);
         for(size_t i = 0, col = start; i < width; ++i, ++col)
         {
            line[col] = label[i];
         }

         //  A message to self comes from nowhere.  Erase most of the line
         //  to the right of the label so it won't look like it was sent by
         //  the context on the right.
         //
         if(mt.Self())
         {
            start += width;
            end = (start + 2) + pad;
            for(auto col = start; col <= end; ++col) line[col] = SPACE;
         }
      }
      else
      {
         line[end - 1] = MsgRight;
         start += 1;
         end -= 2;
         for(auto col = start; col <= end; ++col) line[col] = MsgLine;

         auto pad = ((ColWidth - MinMsgLine) - width + 1) / 2;
         start = (start + 2) + pad;
         for(size_t i = 0, col = start; i < width; ++i, ++col)
         {
            line[col] = label[i];
         }

         //  A message to self comes from nowhere.  Erase most of the line
         //  to the left of the label so it won't look like it was sent by
         //  the context on the left.
         //
         if(mt.Self())
         {
            end = start - 1;
            start -= (pad + 2);
            for(auto col = start; col <= end; ++col) line[col] = SPACE;
         }
      }
   }
   else
   {
      //  A message from a context to itself (start == end) causes problems.
      //
      return Error("message to self", pack2(mt.Prid(), mt.Sid()));
   }

   auto gap = spaces(TimeGap);
   auto fill = spaces(TimeLen);

   line += string(lastCol_ - line.size(), SPACE);
   line += gap;

   if(!txmsgTime.empty())
      line += txmsgTime;
   else
      line += fill;

   line += gap;

   if(!rxnetTime.empty())
      line += rxnetTime;
   else
      line += fill;

   if(!transTime.empty())
      line += gap + transTime;

   //  Output the message after adding event timestamps.
   //
   AddRow(line);
   return true;
}

//------------------------------------------------------------------------------

void MscBuilder::OutputTrailer() const
{
   Debug::ft("MscBuilder.OutputTrailer");

   //  Append TransTracer and ContextTracer events to the MSC.
   //  To filter out other trace records, disable other tools
   //  and reenable them afterwards.
   //
   auto buff = Singleton< TraceBuffer >::Instance();
   auto tools = buff->GetTools();
   buff->ClearTools();
   buff->SetTool(TransTracer, true);
   buff->SetTool(ContextTracer, true);
   *stream_ << CRLF;
   Singleton< TraceBuffer >::Instance()->DisplayTrace(stream_, EMPTY_STR);
   *stream_ << MscTrailer;
   buff->SetTools(tools);
}

//------------------------------------------------------------------------------

void MscBuilder::ReduceColumns(MscColumn start, MscColumn count)
{
   Debug::ft("MscBuilder.ReduceColumns");

   //  All columns beyond START have moved COUNT columns to the left.
   //
   for(size_t i = 0; i < lines_; ++i)
   {
      if(columns_[i] > start) columns_[i] -= count;
   }

   for(auto c = contextq_.First(); c != nullptr; contextq_.Next(c))
   {
      auto col = c->Column();

      if(col > start) c->SetColumn(col - count);
   }
}

//------------------------------------------------------------------------------

fn_name MscBuilder_SetContextColumns = "MscBuilder.SetContextColumns";

void MscBuilder::SetContextColumns()
{
   Debug::ft(MscBuilder_SetContextColumns);

   //  Start by removing all contexts.
   //
   for(auto c = contextq_.First(); c != nullptr; contextq_.Next(c))
   {
      c->SetColumn(NilMscColumn);
   }

   nextCol_ = FirstCol;

   //  If the external context without a known factory belongs to the current
   //  group, put it on the left of the MSC.
   //
   auto ctx = FindContext(nullptr, NIL_ID);

   if((ctx != nullptr) && (ctx->Group() == group_))
   {
      nextCol_ = ctx->SetColumn(nextCol_);
   }

   //  Next, add the other external contexts that belong to the current group.
   //
   for(auto c = contextq_.First(); c != nullptr; contextq_.Next(c))
   {
      if(c->Group() != group_) continue;
      if(!c->IsExternal()) continue;

      if(c->Column() == NilMscColumn)
      {
         nextCol_ = c->SetColumn(nextCol_);
      }
   }

   //  Finally, add the internal contexts that belong to the current group.
   //
   for(auto c = contextq_.First(); c != nullptr; contextq_.Next(c))
   {
      if(c->Group() != group_) continue;

      if(c->Column() == NilMscColumn)
      {
         nextCol_ = c->SetColumn(nextCol_);
      }

      //  Find the internal contexts that communicate with this one.  Those
      //  without a column will be assigned one now.  This helps to keep
      //  communicating contexts close together in the MSC.
      //
      SetNeighbourColumns(*c);
   }

   //  All contexts in the current group have been assigned columns.  Check
   //  that nextCol_ has the expected value.
   //
   if(nextCol_ != (FirstCol + (lines_ * ColWidth)))
   {
      Debug::SwLog(MscBuilder_SetContextColumns, "column invalid", nextCol_);
   }
}

//------------------------------------------------------------------------------

void MscBuilder::SetNeighbourColumns(const MscContext& context)
{
   Debug::ft("MscBuilder.SetNeighbourColumns");

   //  Find all pairs of communicating contexts that contain RCVR.  If the
   //  other context in the pair is not assigned a column, assign one to it.
   //
   for(auto p = pairq_.First(); p != nullptr; pairq_.Next(p))
   {
      auto peer = p->Peer(context);

      if((peer != nullptr) && (peer->Column() == NilMscColumn))
      {
         nextCol_ = peer->SetColumn(nextCol_);
      }
   }
}
}
