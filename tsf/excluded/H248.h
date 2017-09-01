/*===========================================================================*/
//
//  H248.h
//
//  Copyright (C) 2013-2014 Greg Utas.  All rights reserved.
//
#ifndef _H248H_H_INCLUDED_
#define _H248H_H_INCLUDED_

#include <string>
#include <iostream>
#include "SysDefs.h"
#include "SysCalls.h"
#include "Objects.h"
#include "PooledObjects.h"
#include "TraceRecord.h"
#include "Sessions.h"

using namespace NodeBase;
using namespace SessionBase;

namespace SessionBase
{
class H248Endpt;  // forward declaration

/*---------------------------------------------------------------------------*/

typedef uint32 H248CtxtId;  // H.248 context identifier
typedef uint32 H248TermId;  // H.248 termination identifier

const H248CtxtId H248NilCtxtId = 0;  // nil H.248 context identifier
const H248TermId H248NilTermId = 0;  // nil H.248 ternination identifier

//  The attributes of an ephemeral channel allocated for media
//  transmission/receiption on an H.248 media gateway.
//
struct H248Chnl
{
    H248CtxtId   cid;  // the H.248 context to which this channel belongs
    H248TermId   tid;  // the channel's identifier within its context
    IpL3Address  eph;  // the channel's ephemeral port on the media gateway

    //  Displays member variables.
    //
    void Display(ostream &stream, uint8 indent) const;
};

const H248Chnl H248NilChnl =
               {
                   H248NilCtxtId,
                   H248NilTermId,
                   {IpNilAddress, IpNilPort}
               };

/*---------------------------------------------------------------------------*/
//
//  Attributes of a media channel.
//
struct ChnlAttrs
{
    H248Chnl  chnl;  // the channel's address information
    bool      tx;    // true if the channel is willing to transmit

    //  Displays member variables.
    //
    void Display(ostream &stream, uint8 indent) const;
};

const ChnlAttrs NilChnlAttrs =
                {
                    {
                        H248NilCtxtId,
                        H248NilTermId,
                        {IpNilAddress, IpNilPort}
                    },
                    false
                };

/*---------------------------------------------------------------------------*/
//
//  Information about one of a media endpoint's connections.
//
struct H248Conn
{
    //  Connection identifier.
    //
    typedef int16 Id;

    //  Nil connection identifier.
    //
    static const Id NilId = -1;

    //> Highest valid connection identifier.
    //
    static const Id MaxId = 8;

    H248Endpt  *remMep;     // the mate endpoint involved in this connection
    Id          remConn;    // the mate's identifier for this connection
    bool        txEnabled;  // true if this endpoint is willing to transmit
    bool        rxEnabled;  // true if this endpoint is willing to receive

    //  Displays member variables.
    //
    void Display(ostream &stream, uint8 indent) const;
};

const H248Conn NilH248Conn = {NULL, H248Conn::NilId, false, false};

/*---------------------------------------------------------------------------*/
//
//  Applications invoke media endpoint (MEP) functions to control media
//  channels and connections.  Each PSM that supports media has an MEP.
//  Here, H248Endpt provides the implementation of an MEP.
//
class H248Endpt : public MediaEndpt
{
public:
    //  Results when performing media operations.
    //
    enum Result
    {
        Ok,          // success
        NoResource,  // resource not available
        Denied,      // connection would have overwritten another one
        Error        // unexpected error
    };

    //  States for channel assignment.
    //
    static const State::Id Requested = 1;  // allocation pending
    static const State::Id Assigned  = 2;  // allocation completed

    //  Allocates an ephemeral channel for the MEP.  Must be overridden
    //  by subclasses that support this capability.
    //
    virtual Result CreateChnl(void);

    //  Sets a subscriber's media port.  Must be overridden by edge MEPs.
    //
    virtual Result ModifyChnl(const IpL3Address &chnl);

    //  Sets an ephemeral port.  Must be overridden by edge MEPs.
    //
    virtual Result ModifyChnl(const H248Chnl &chnl);

    //  Sets an ephemeral port.  Must be overridden by proxy MEPs.
    //
    virtual Result ModifyChnl(const ChnlAttrs &chnl);

    //  Stops all transmission and reception by the channel.  Used to
    //  suspend a media stream.
    //
    void DisableChnl(void);

    //  Restarts transmission and reception by the channel.  Used to
    //  restart a media stream that was suspended by DisableChnl.
    //
    void EnableChnl(void);

    //  Deallocates the MEP's ephemeral channel.  Must be overridden
    //  by subclasses that support this capability.
    //
    virtual void DestroyChnl(void);

    //  Creates a connection association with remMep.  The connection is
    //  initially disabled; ModifyConn must be invoked to enable it.
    //
    H248Conn::Id CreateConn(H248Endpt &remMep);

    //  Returns the mate MEP's identifier for the connection known to this
    //  MEP as CID.
    //
    H248Conn::Id RemConn(H248Conn::Id cid);

    //  Controls a connection by specifying whether the endpoint associated
    //  with the MEP is willing to transmit (TX) and/or receive (RX).
    //
    virtual Result ModifyConn(H248Conn::Id cid, bool tx, bool rx);

    //  Destroys a connection.
    //
    void DestroyConn(H248Conn::Id cid);

    //  Idles the MEP, which deletes itself at the end of the transaction.
    //  This function must be used (instead of the destructor) so that the
    //  MEP can send any pending messages at the end of the transaction.
    //
    virtual void Deallocate(void);

    //  Overridden to display member variables.
    //
    virtual void Display(ostream &stream, uint8 indent, bool verbose) const;

    //  Overridden for patching.
    //
    virtual void Patch(uint8 selector, void *arguments);
protected:
    //  Creates an MEP that is owned by PSM.
    //
    H248Endpt(ProtocolSM &psm);

    //  Deregisters the MEP from its PSM.  Generates a log if the MEP is not
    //  in the idle state (that is, if Deallocate was not invoked).
    //
    virtual ~H248Endpt(void);

    //  Invoked so that the MEP can process any connection control parameter
    //  in incoming message MSG.  Must be overridden by subclasses that need
    //  this capability.
    //
    virtual void ProcessIcMsg(Message &msg);

    //  Establishes an incoming connection to the MEP's channel.
    //  CHNL specifies the attributes of the far-end channel.
    //
    virtual Result MakeConnection(const ChnlAttrs &chnl) = 0;

    //  Frees the incoming connection to the MEP's channel.
    //
    virtual void FreeConnection(void) = 0;

    //  Deletes the MEP at the end of the transaction in which Deallocate
    //  was invoked.  May also be overridden by subclasses that need to
    //  add connection control parameters to outgoing messages, but the
    //  superclass function must be invoked.
    //
    virtual void EndOfTransaction(void);

    //  Creates a connection association with remMep, which has already
    //  assigned remConn as its identifier for the connection.
    //
    H248Conn::Id InsertConn(H248Endpt &remMep, H248Conn::Id remConn);

    //  Sets CHNL to the channel attributes for a specific connection.
    //
    void GetChnlAttrs(H248Conn::Id cid, ChnlAttrs &chnl) const;

    //  Returns true if the BEFORE and AFTER are sufficiently different that
    //  connection modification is required.
    //
    bool ChnlChanged(const IpL3Address &before, const IpL3Address &after) const;

    //  Returns true if the BEFORE and AFTER are sufficiently different that
    //  connection modification is required.
    //
    bool ChnlChanged(const H248Chnl &before, const H248Chnl &after) const;

    //  Returns true if the BEFORE and AFTER are sufficiently different that
    //  connection modification is required.
    //
    bool ChnlChanged(const ChnlAttrs &before, const ChnlAttrs &after) const;

    //  Returns the default value for an instance of H248Conn.txEnabled.
    //
    virtual bool TxConnInit(void) const;

    //  Updates all connections when the channel's attributes have changed.
    //
    Result UpdateConns(void);

    //  Updates a connection.  Invoked by the mate MEP's UpdateConns when
    //  its CHNL attributes have changed.
    //
    Result UpdateConn(H248Conn::Id cid, const ChnlAttrs &chnl);

    //  Frees all connections.  Used during deallocation.
    //
    void FreeConns(void);

    //  Destroys a connection.
    //
    void DeleteConn(H248Conn::Id cid);

    //  Records the results of a ModifyChnl for debugging.
    //
    Result TraceChnl(Result res) const;

    //  The subscriber's (external) media port.
    //
    IpL3Address userPort_;

    //  The MEP's ephemeral channel attributes.
    //
    ChnlAttrs locChnl_;

    //  The attributes for what rxConn_ is receiving.
    //
    ChnlAttrs remChnl_;

    //  True if a connection parameter should be generated.
    //
    bool generate_;
private:
    //  Returns a reference to CID's connection data.
    //
    H248Conn *AccessConn(H248Conn::Id cid);

    //  Establishes CID as the incoming connection.
    //
    Result MakeRxConn(H248Conn::Id cid);

    //  Frees the incoming connection.
    //
    void FreeRxConn(void);

    //  Destroys all connections.
    //
    void DestroyConns(void);

    //  Records the results of a ModifyConn for debugging.
    //
    Result TraceConn(H248Conn::Id cid, Result res);

    //  The connections in which the MEP is involved.
    //
    H248Conn conns_[H248Conn::MaxId + 1];

    //  The connection on which the MEP wishes to receive.
    //
    H248Conn::Id rxConn_;

    //  True if the channel is disabled.
    //
    bool disabled_;
};

/*---------------------------------------------------------------------------*/
//
//  Records an invocation of H248Endpt::ModifyChnl.
//
class ChnlTrace : public TraceRecord
{
public:
    //  Captures the result (MER) of a ModifyChnl, which was invoked to
    //  establish ATTRS on the H248Endpt owned by PSM.
    //
    ChnlTrace(ProtocolSM &psm, const ChnlAttrs &attrs, H248Endpt::Result mer);

    //  Does nothing.
    //
    ~ChnlTrace(void);

    //  Overridden to return the tool that owns this record.
    //
    ToolId Owner(void) const;

    //  Overridden to return a string for displaying this type of record.
    //
    const string *EventString(void) const;

    //  Overridden to display the trace record.
    //
    void Display(ostream &stream, int32 bid);
private:
    //  The PSM whose MEP performed the ModifyChnl.
    //
    ProtocolSM::Id psm_;

    //  The H.248 context associated with the channel.
    //
    H248CtxtId cid_;

    //  The H.248 termination associated with the channel.
    //
    H248TermId tid_;

    //  The IP port associated with the channel.
    //
    IpPort port_;

    //  True if the port was willing to transmit.
    //
    bool tx_;

    //  The outcome of the ModifyChnl.
    //
    H248Endpt::Result mer_;
};

/*---------------------------------------------------------------------------*/
//
//  Records an invocation of H248Endpt::ModifyConn.
//
class ConnTrace : public TraceRecord
{
public:
    //  Captures the result (MER) of ModifyConn, which was invoked on CONN of
    //  the H248Endpt owned by PSM.
    //
    ConnTrace(ProtocolSM &psm, const H248Conn *conn, H248Endpt::Result mer);

    //  Does nothing.
    //
    ~ConnTrace(void);

    //  Overridden to return the tool that owns this record.
    //
    ToolId Owner(void) const;

    //  Overridden to return a string for displaying this type of record.
    //
    const string *EventString(void) const;

    //  Overridden to display the trace record.
    //
    void Display(ostream &stream, int32 bid);
private:
    //  The PSM whose MEP performed the ModifyConn.
    //
    ProtocolSM::Id locPsm_;
    
    //  The PSM associated with the remote MEP.
    //
    ProtocolSM::Id remPsm_;
    
    //  True if the local MEP was willing to receive.
    //
    bool rx_;
    
    //  True if the local MEP was willing to transmit.
    //
    bool tx_;
    
    //  The outcome of the ModifyConn.
    //
    H248Endpt::Result mer_;
};

};
#endif
