//==============================================================================
//
//  MscBuilder.h
//
//  Copyright (C) 2017  Greg Utas
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
#ifndef MSCBUILDER_H_INCLUDED
#define MSCBUILDER_H_INCLUDED

#include "Temporary.h"
#include <cstddef>
#include <iosfwd>
#include <string>
#include "Factory.h"
#include "MscContext.h"
#include "NbTypes.h"
#include "Q1Way.h"
#include "SbTypes.h"
#include "SysTypes.h"
#include "ToolTypes.h"

namespace NodeBase
{
   class TraceRecord;
}

namespace SessionBase
{
   struct LocalAddress;
   class MsgTrace;
}

namespace SessionTools
{
   class MscAddress;
   class MscContextPair;
}

using namespace NodeBase;
using namespace SessionBase;

//------------------------------------------------------------------------------

namespace SessionTools
{
//  Constructs message sequence charts (MSCs) from the trace records
//  captured by TransTracer and ContextTracer.
//
class MscBuilder : public Temporary
{
public:
   //> The maximum number of columns supported in an MSC.
   //
   static const MscColumn MaxCols = 14;

   //> The maximum number of rows supported in an MSC.
   //
   static const int MaxRows = 512;

   //> The maximum number of trace records used to generate MSCs.
   //
   static const int MaxEvents = 3 * MaxRows;

   //  Prepares to build MSCs from the trace records.  If DEBUG is
   //  set, internal data structures are output before the MSCs.
   //
   explicit MscBuilder(bool debug);

   //  Deletes the data used to build MSCs.  Not subclassed.
   //
   ~MscBuilder();

   //  Builds the MSCs.
   //
   TraceRc Generate(std::ostream& stream);

   //  Overridden to display member variables.
   //
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;
private:
   //  Finds the trace records that are relevant to building MSCs.
   //
   TraceRc ExtractEvents();

   //  Adds REC to the set of trace records used to build MSCs.
   //
   void AddEvent(const TraceRecord* rec);

   //  Creates the set of addresses, contexts, and context pairs that
   //  are associated with the trace records used to build MSCs.
   //
   TraceRc AnalyzeEvents();

   //  Creates an address identified by mt.locAddr if such an address
   //  has yet to be noted.  Returns a pointer to the address, whether
   //  it existed or was just created.
   //
   void EnsureAddr(const MsgTrace& mt, MscContext* context);

   //  Finds the address identified by locAddr in the set of addresses.
   //
   MscAddress* FindAddr(const LocalAddress& locAddr) const;

   //  Records that the local and remote addresses captured by MT are
   //  communicating.
   //
   void JoinAddrs(const MsgTrace& mt);

   //  Finds the address that communicates with one identified by LOCADDR.
   //
   MscAddress* FindPeer(const LocalAddress& locAddr) const;

   //  Finds the context associated with RCVR in the set of contexts.
   //  If RCVR is nullptr (an external context), the search uses CID.
   //
   MscContext* FindContext(const void* rcvr, id_t cid) const;

   //  Creates a context for a factory identified by FID, if such a context
   //  has yet to be noted.  FAC is the actual instance, if known.  Returns
   //  a pointer to the context, whether it existed or was just created.
   //
   MscContext* EnsureContext(const Factory* fac, FactoryId fid);

   //  Creates a context from TRANS if such a context has yet to be noted.
   //  Returns a pointer to the context, whether it existed or was just
   //  created.
   //
   MscContext* EnsureContext(const TransTrace& trans);

   //  Records that context1 and context2 are communicating.
   //
   void JoinContexts(MscContext& context1, MscContext& context2);

   //  If a context is communicating with a factory, this function ensures
   //  that the factory is also noted as a context in the MSC.
   //
   void EnsureFactories();

   //  Generates an MSC's header.
   //
   void OutputHeader() const;

   //  Finds the next group of contexts to be displayed in an MSC.
   //  Returns false if all contexts have been displayed.
   //
   bool ExtractGroup() const;

   //  Generates the MSC for the current group of contexts.
   //
   void OutputChart();

   //  Returns the number of contexts in the current MSC.
   //
   size_t CountContexts() const;

   //  Assigns a display column to each context in the current MSC.
   //
   void SetContextColumns();

   //  Assigns a display column to contexts that communicate with CONTEXT.
   //
   void SetNeighbourColumns(const MscContext& context);

   //  Displays the current group of contexts at the top of a new MSC.
   //
   void OutputGroup();

   //  Finds the context that is displayed at COLUMN.
   //
   MscContext* ColumnToContext(MscColumn column) const;

   //  Returns the "rxmsg" event that corresponds to the "RXNET" event
   //  referenced by events_[index].
   //
   const MsgTrace* FindRxMsg(size_t index) const;

   //  Returns the "TRANS" event that handled the "txmsg" event referenced
   //  by events_[index].
   //
   const TransTrace* FindTrans(size_t index) const;

   //  Generates a "blank line" in the MSC.  It contains a vertical line
   //  for each context.  ACTIVE is the running context, if any; it has
   //  a different type of vertical line.
   //
   std::string OutputFiller(const MscContext* active) const;

   //  Displays MT in the MSC.  ACTIVE is the running context, if any.
   //  TT is the "TRANS" event during which the message was handled.
   //
   bool OutputMessage
      (const MscContext* active, const MsgTrace& mt, const TransTrace* tt);

   //  Adds a row with contents S to the MSC.
   //
   void AddRow(const std::string& s);

   //  Generates an MSC's trailer.
   //
   void OutputTrailer() const;

   //  Performs horizontal compression on the MSC to minimize its width.
   //
   void Compress();

   //  Updates column positions when COUNT columns have been deleted to
   //  the right of START.
   //
   void ReduceColumns(MscColumn start, MscColumn count);

   //  Invoked when an unexpected condition occurs.  Generates a log that
   //  contains ERRVAL and OFFSET, and adds a '?' debug line to the MSC to
   //  indicate where the error occurred.  Returns false.
   //
   bool Error(debug64_t errval, debug32_t offset);

   //  Overridden to prohibit copying.
   //
   MscBuilder(const MscBuilder& that);
   void operator=(const MscBuilder& that);

   //  Set if internal data structures are to be displayed.
   //
   bool debug_;

   //  Trace records of interest to generating an MSC.
   //
   const TraceRecord* events_[MaxEvents];

   //  The next available slot in events_.
   //
   size_t nextEvent_;

   //  The addresses (PSMs and factories) in the MSC.
   //
   Q1Way< MscAddress > addressq_;

   //  The contexts in the MSC (SSMs, PSMs, and factories).
   //
   Q1Way< MscContext > contextq_;

   //  Pairs of communicating *internal* contexts in an MSC.
   //
   Q1Way< MscContextPair > pairq_;

   //  The context group (1 to n) whose MSC is currently being generated.
   //
   int group_;

   //  The number of vertical lines (contexts) in the current MSC.
   //
   int lines_;

   //  The columns assigned to contexts, in left to right order.
   //
   MscColumn columns_[MaxCols];

   //  The next column available for a context.
   //
   MscColumn nextCol_;

   //  The last column used for a group name.  Event times are added to the
   //  right of this column.
   //
   MscColumn lastCol_;

   //  Workspace for assembling the current MSC.
   //
   TempString rows_[MaxRows];

   //  The next available slot in rows_.
   //
   size_t nextRow_;

   //  The stream into which the MSC is being written.
   //
   std::ostream* stream_;
};
}
#endif
