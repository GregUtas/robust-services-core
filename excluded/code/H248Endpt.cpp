/*===========================================================================*/
//
//  H248Endpt.cpp
//
//  Copyright (C) 2013-2014 Greg Utas.  All rights reserved.
//
#include <string>
#include <iostream>
#include "SysDefs.h"
#include "Singleton.h"
#include "SysCalls.h"
#include "Formatters.h"
#include "Logs.h"
#include "Objects.h"
#include "PooledObjects.h"
#include "NbAppIds.h"
#include "Sessions.h"
#include "SbPools.h"
#include "SbTraceRecords.h"
#include "H248.h"

using namespace NodeBase;
using namespace SessionBase;

/*===========================================================================*/

void H248Chnl::Display(ostream &stream, uint8 indent) const
{
    stream << spaces(indent) << "cid : " << cid << endl;
    stream << spaces(indent) << "tid : " << tid << endl;
    stream << spaces(indent) << "eph : " << eph.strL3Addr() << endl;
}

/*---------------------------------------------------------------------------*/

void ChnlAttrs::Display(ostream &stream, uint8 indent) const
{
    stream << spaces(indent) << "chnl : " << endl;
    chnl.Display(stream, indent+2);
    stream << spaces(indent) << "tx : " << tx << endl;
}

/*---------------------------------------------------------------------------*/

void H248Conn::Display(ostream &stream, uint8 indent) const
{
    stream << spaces(indent) << "remMep    : " << remMep << endl;
    stream << spaces(indent) << "remConn   : " << remConn << endl;
    stream << spaces(indent) << "txEnabled : " << txEnabled << endl;
    stream << spaces(indent) << "rxEnabled : " << rxEnabled << endl;
}

/*===========================================================================*/

const string MerString[] =  // indexed by H248Endpt::Result
{
    "ok",
    "noResource",
    "denied",
    "error"
};

/*===========================================================================*/

const string H248Endpt_ctor = "H248Endpt.ctor";


H248Endpt::H248Endpt(ProtocolSM &psm) : MediaEndpt(psm)
{
    Logs::ft(&H248Endpt_ctor);

    //  Initialize all data.
    //
    userPort_  = IpL3NilAddress;
    locChnl_   = NilChnlAttrs;
    remChnl_   = NilChnlAttrs;
    generate_  = false;

    for(H248Conn::Id i = 0; i <= H248Conn::MaxId; i++) conns_[i] = NilH248Conn;

    rxConn_    = H248Conn::NilId;
    disabled_  = false;
}

/*---------------------------------------------------------------------------*/

const string H248Endpt_dtor = "H248Endpt.dtor";


H248Endpt::~H248Endpt(void)
{
    Logs::ft(&H248Endpt_dtor);

    //  If the MEP is not idle, Deallocate was never invoked, so generate a
    //  log and invoke it now.
    //
    if(GetState() != Idle)
    {
        for(H248Conn::Id i = 0; i <= H248Conn::MaxId; i++)
        {
            H248Conn *conn = &conns_[i];
            if(conn->remMep != NULL) conn->remMep->DeleteConn(conn->remConn);
        }
    }
}

/*---------------------------------------------------------------------------*/

const string H248Endpt_AccessConn = "H248Endpt.AccessConn";


H248Conn *H248Endpt::AccessConn(H248Conn::Id cid)
{
    Logs::ft(&H248Endpt_AccessConn);

    //  CID is a valid connection if it is in range and a remote MEP is
    //  registered against it.
    //
    if((cid >= 0) && (cid <= H248Conn::MaxId))
    {
        if(conns_[cid].remMep != NULL) return &conns_[cid];
    }

    Logs::SwErr(&H248Endpt_AccessConn, cid, 0);
    return NULL;
}

/*---------------------------------------------------------------------------*/

const string H248Endpt_ChnlChanged_user = "H248Endpt.ChnlChanged[user]";


bool H248Endpt::ChnlChanged(const IpL3Address &before, const IpL3Address &after) const
{
    Logs::ft(&H248Endpt_ChnlChanged_user);

    return ((before.port != after.port) || (before.addr != after.addr));
}

/*---------------------------------------------------------------------------*/

const string H248Endpt_ChnlChanged_edge = "H248Endpt.ChnlChanged[edge]";


bool H248Endpt::ChnlChanged(const H248Chnl &before, const H248Chnl &after) const
{
    Logs::ft(&H248Endpt_ChnlChanged_edge);

    return
        ((before.eph.port != after.eph.port) ||
         (before.tid      != after.tid)      ||
         (before.cid      != after.cid));
}

/*---------------------------------------------------------------------------*/

const string H248Endpt_ChnlChanged_proxy = "H248Endpt.ChnlChanged[proxy]";


bool H248Endpt::ChnlChanged(const ChnlAttrs &before, const ChnlAttrs &after) const
{
    Logs::ft(&H248Endpt_ChnlChanged_proxy);

    return ((before.tx != after.tx) || (ChnlChanged(before.chnl, after.chnl)));
}

/*---------------------------------------------------------------------------*/

const string H248Endpt_CreateChnl = "H248Endpt.CreateChnl";


H248Endpt::Result H248Endpt::CreateChnl(void)
{
    Logs::ft(&H248Endpt_CreateChnl);
    return Error;
}

/*---------------------------------------------------------------------------*/

const string H248Endpt_CreateConn = "H248Endpt.CreateConn";


H248Conn::Id H248Endpt::CreateConn(H248Endpt &remMep)
{
    Logs::ft(&H248Endpt_CreateConn);

    //  If a connection slot is available, try to allocate one on the remote
    //  MEP as well.  Each MEP saves a reference to its mate MEP and the slot
    //  (connection identifier) that the mate allocated for the connection.
    //
    for(H248Conn::Id cid = 0; cid <= H248Conn::MaxId; cid++)
    {
        H248Conn *conn = &conns_[cid];

        if(conn->remMep == NULL)
        {
            conn->remConn = remMep.InsertConn(*this, cid);

            if(conn->remConn != H248Conn::NilId)
            {
                conn->remMep    = &remMep;
                conn->txEnabled = false;
                conn->rxEnabled = false;

                return cid;
            }

            return H248Conn::NilId;
        }
    }

    return H248Conn::NilId;
}

/*---------------------------------------------------------------------------*/

const string H248Endpt_Deallocate = "H248Endpt.Deallocate";


void H248Endpt::Deallocate(void)
{
    Logs::ft(&H248Endpt_Deallocate);

    //  Destroy all connections and the channel.
    //
    DestroyConns();
    DestroyChnl();

    MediaEndpt::Deallocate();
}

/*---------------------------------------------------------------------------*/

const string H248Endpt_DeleteConn = "H248Endpt.DeleteConn";


void H248Endpt::DeleteConn(H248Conn::Id cid)
{
    Logs::ft(&H248Endpt_DeleteConn);

    //  If this is the incoming connection, free it before deleting it.
    //
    if(rxConn_ == cid) FreeRxConn();

    conns_[cid].remMep = NULL;
}

/*---------------------------------------------------------------------------*/

const string H248Endpt_DestroyChnl = "H248Endpt.DestroyChnl";


void H248Endpt::DestroyChnl(void)
{
    Logs::ft(&H248Endpt_DestroyChnl);
}

/*---------------------------------------------------------------------------*/

const string H248Endpt_DestroyConn = "H248Endpt.DestroyConn";


void H248Endpt::DestroyConn(H248Conn::Id cid)
{
    Logs::ft(&H248Endpt_DestroyConn);

    //  If the connection exists, delete it at both ends.
    //
    H248Conn *conn = AccessConn(cid);

    if(conn != NULL)
    {
        conn->remMep->DeleteConn(conn->remConn);
        DeleteConn(cid);
    }
}

/*---------------------------------------------------------------------------*/

const string H248Endpt_DestroyConns = "H248Endpt.DestroyConns";


void H248Endpt::DestroyConns(void)
{
    Logs::ft(&H248Endpt_DestroyConns);

    for(H248Conn::Id i = 0; i <= H248Conn::MaxId; i++)
    {
        if(conns_[i].remMep != NULL) DestroyConn(i);
    }
}

/*---------------------------------------------------------------------------*/

const string H248Endpt_DisableChnl = "H248Endpt.DisableChnl";


void H248Endpt::DisableChnl(void)
{
    Logs::ft(&H248Endpt_DisableChnl);

    disabled_ = true;
    UpdateConns();
}

/*---------------------------------------------------------------------------*/

void H248Endpt::Display(ostream &stream, uint8 indent, bool verbose) const
{
    MediaEndpt::Display(stream, indent, verbose);

    stream << spaces(indent) << "userPort : " << userPort_.strL3Addr() << endl;

    stream << spaces(indent) << "locChnl  : " << endl;
    locChnl_.Display(stream, indent+2);

    stream << spaces(indent) << "remChnl  : " << endl;
    remChnl_.Display(stream, indent+2);

    stream << spaces(indent) << "generate : " << generate_ << endl;
    stream << spaces(indent) << "rxConn   : " << (int) rxConn_ << endl;
    stream << spaces(indent) << "disabled : " << disabled_ << endl;

    stream << spaces(indent) << "conns [H248Conn::Id]" << endl;

    for(H248Conn::Id i = 0; i <= H248Conn::MaxId; i++)
    {
        if(conns_[i].remMep != NULL)
        {
            stream << spaces(indent+2) << strIdx(i, 0) << endl;
            conns_[i].Display(stream, indent+4);
        }
    }
}

/*---------------------------------------------------------------------------*/

const string H248Endpt_EnableChnl = "H248Endpt.EnableChnl";


void H248Endpt::EnableChnl(void)
{
    Logs::ft(&H248Endpt_EnableChnl);

    disabled_ = false;
    UpdateConns();
}

/*---------------------------------------------------------------------------*/

const string H248Endpt_EndOfTransaction = "H248Endpt.EndOfTransaction";


void H248Endpt::EndOfTransaction(void)
{
    Logs::ft(&H248Endpt_EndOfTransaction);

    MediaEndpt::EndOfTransaction();
}

/*---------------------------------------------------------------------------*/

const string H248Endpt_FreeConnection = "H248Endpt.FreeConnection";


void H248Endpt::FreeConnection(void)
{
    Logs::ft(&H248Endpt_FreeConnection);
}

/*---------------------------------------------------------------------------*/

const string H248Endpt_FreeConns = "H248Endpt.FreeConns";


void H248Endpt::FreeConns(void)
{
    Logs::ft(&H248Endpt_FreeConns);

    //  Free the incoming connection and inform each remote MEP that our
    //  connections have been disabled.
    //
    if(rxConn_ != H248Conn::NilId) FreeConnection();

    for(H248Conn::Id cid = 0; cid <= H248Conn::MaxId; cid++)
    {
        H248Conn *conn = &conns_[cid];

        if(conn->remMep != NULL)
        {
            Result res = conn->remMep->UpdateConn(conn->remConn, NilChnlAttrs);

            if(res != Ok) Logs::SwErr(&H248Endpt_FreeConns, cid, (int32) res);
        }
    }
}

/*---------------------------------------------------------------------------*/

const string H248Endpt_FreeRxConn = "H248Endpt.FreeRxConn";


void H248Endpt::FreeRxConn(void)
{
    //  Free the incoming connection.
    //
    Logs::ft(&H248Endpt_FreeRxConn);

    FreeConnection();
    conns_[rxConn_].rxEnabled = false;
    rxConn_ = H248Conn::NilId;
}

/*---------------------------------------------------------------------------*/

const string H248Endpt_GetChnlAttrs = "H248Endpt.GetChnlAttrs";


void H248Endpt::GetChnlAttrs(H248Conn::Id cid, ChnlAttrs &chnl) const
{
    Logs::ft(&H248Endpt_GetChnlAttrs);

    if(!disabled_)
    {
        //  Get our channel's attributes.  If the channel is allowed to
        //  transmit, then what to transmit is up to the connection.
        //
        chnl = locChnl_;

        if(chnl.tx) chnl.tx = conns_[cid].txEnabled;

        if(!chnl.tx) chnl = NilChnlAttrs;
    }
    else
    {
        chnl = NilChnlAttrs;
    }
}

/*---------------------------------------------------------------------------*/

const string H248Endpt_InsertConn = "H248Endpt.InsertConn";


H248Conn::Id H248Endpt::InsertConn(H248Endpt &remMep, H248Conn::Id remConn)
{
    Logs::ft(&H248Endpt_InsertConn);

    //  Search for an available connection slot.  If one is found, register
    //  remMep and remConn within it and return its index.
    //
    for(H248Conn::Id cid = 0; cid <= H248Conn::MaxId; cid++)
    {
        H248Conn *conn = &conns_[cid];

        if(conn->remMep == NULL)
        {
            conn->remMep    = &remMep;
            conn->remConn   = remConn;
            conn->txEnabled = false;
            conn->rxEnabled = false;

            return cid;
        }
    }

    return H248Conn::NilId;
}

/*---------------------------------------------------------------------------*/

const string H248Endpt_MakeConnection = "H248Endpt.MakeConnection";


H248Endpt::Result H248Endpt::MakeConnection(const ChnlAttrs &chnl)
{
    //  This is defined as a pure virtual function.
    //
    Logs::ft(&H248Endpt_MakeConnection);

    Context::KillContext(&H248Endpt_MakeConnection, 0, 0);
    return Error;
}

/*---------------------------------------------------------------------------*/

const string H248Endpt_MakeRxConn = "H248Endpt.MakeRxConn";


H248Endpt::Result H248Endpt::MakeRxConn(H248Conn::Id cid)
{
    Logs::ft(&H248Endpt_MakeRxConn);

    H248Conn  *conn;
    ChnlAttrs chnl;
    Result    res;

    //  Get the remote MEP's channel attributes and make a connection from it.
    //
    conn = &conns_[cid];
    conn->remMep->GetChnlAttrs(conn->remConn, chnl);
    res = MakeConnection(chnl);

    if(res != Ok) return res;

    conn->rxEnabled = true;
    rxConn_ = cid;

    return Ok;
}

/*---------------------------------------------------------------------------*/

const string H248Endpt_ModifyChnl_user = "H248Endpt.ModifyChnl[user]";


H248Endpt::Result H248Endpt::ModifyChnl(const IpL3Address &chnl)
{
    Logs::ft(&H248Endpt_ModifyChnl_user);

    return Error;
}

/*---------------------------------------------------------------------------*/

const string H248Endpt_ModifyChnl_edge = "H248Endpt.ModifyChnl[edge]";


H248Endpt::Result H248Endpt::ModifyChnl(const H248Chnl &chnl)
{
    Logs::ft(&H248Endpt_ModifyChnl_edge);

    return Error;
}

/*---------------------------------------------------------------------------*/

const string H248Endpt_ModifyChnl_proxy = "H248Endpt.ModifyChnl[proxy]";


H248Endpt::Result H248Endpt::ModifyChnl(const ChnlAttrs &chnl)
{
    Logs::ft(&H248Endpt_ModifyChnl_proxy);

    return Error;
}

/*---------------------------------------------------------------------------*/

const string H248Endpt_ModifyConn = "H248Endpt.ModifyConn";


H248Endpt::Result H248Endpt::ModifyConn(H248Conn::Id cid, bool tx, bool rx)
{
    Logs::ft(&H248Endpt_ModifyConn);

    Result    res = Ok;
    H248Conn  *conn;
    ChnlAttrs chnl;

    //  Check that CID references a valid connection.
    //
    conn = AccessConn(cid);

    if(conn == NULL) return TraceConn(cid, Error);

    //  If we want to receive on CID, then
    //
    //  o If CID is already the incoming connection, there is nothing to do.
    //  o If there is already another incoming connection, deny the request.
    //  o If neither of the above, try to make CID the incoming connection.
    //
    //  If we don't want to receive on CID but it is currently the incoming
    //  connection, then free it.
    //
    if(rx)
    {
        if(rxConn_ != cid)
        {
            if(rxConn_ != H248Conn::NilId) return TraceConn(cid, Denied);

            res = MakeRxConn(cid);

            if(res != Ok) return TraceConn(cid, res);
        }
    }
    else
    {
        if(rxConn_ == cid) FreeRxConn();
    }

    //  If CID has flipped its transmit flag, update its connection.
    //
    if(conn->txEnabled != tx)
    {
        conn->txEnabled = tx;
        GetChnlAttrs(cid, chnl);
        res = conn->remMep->UpdateConn(conn->remConn, chnl);
    }

    return TraceConn(cid, res);
}

/*---------------------------------------------------------------------------*/

void H248Endpt::Patch(uint8 selector, void *arguments)
{
    PooledObject::Patch(selector, arguments);
}

/*---------------------------------------------------------------------------*/

const string H248Endpt_ProcessIcMsg = "H248Endpt.ProcessIcMsg";


void H248Endpt::ProcessIcMsg(Message &msg)
{
    Logs::ft(&H248Endpt_ProcessIcMsg);
}

/*---------------------------------------------------------------------------*/

const string H248Endpt_RemConn = "H248Endpt.RemConn";


H248Conn::Id H248Endpt::RemConn(H248Conn::Id cid)
{
    Logs::ft(&H248Endpt_RemConn);

    //  Return the mate MEP's identifier for the connection that this MEP
    //  identifies as CID.
    //
    H248Conn *conn = AccessConn(cid);

    if(conn != NULL) return conn->remConn;

    return H248Conn::NilId;
}

/*---------------------------------------------------------------------------*/

const string H248Endpt_TraceChnl = "H248Endpt.TraceChnl";


H248Endpt::Result H248Endpt::TraceChnl(Result res) const
{
    Logs::ft(&H248Endpt_TraceChnl);

    SysClock::Ticks timeWarp;

    if(Context::RunningContext()->TraceOn())
    {
        TransTrace::StopTime(timeWarp);

        if(Singleton < Tracer >::Instance()->ToolIsOn(ContextTracer))
            new ChnlTrace(*Psm(), locChnl_, res);

        TransTrace::RestartTime(timeWarp);
    }

    return res;
}

/*---------------------------------------------------------------------------*/

const string H248Endpt_TraceConn = "H248Endpt.TraceConn";


H248Endpt::Result H248Endpt::TraceConn(H248Conn::Id cid, Result res)
{
    Logs::ft(&H248Endpt_TraceConn);

    SysClock::Ticks timeWarp;

    if(Context::RunningContext()->TraceOn())
    {
        TransTrace::StopTime(timeWarp);

        if(Singleton < Tracer >::Instance()->ToolIsOn(ContextTracer))
           new ConnTrace(*Psm(), AccessConn(cid), res);

        TransTrace::RestartTime(timeWarp);
    }

    return res;
}

/*---------------------------------------------------------------------------*/

const string H248Endpt_TxConnInit = "H248Endpt.TxConnInit";


bool H248Endpt::TxConnInit(void) const
{
    Logs::ft(&H248Endpt_TxConnInit);

    return false;
}

/*---------------------------------------------------------------------------*/

const string H248Endpt_UpdateConn = "H248Endpt.UpdateConn";


H248Endpt::Result H248Endpt::UpdateConn(H248Conn::Id cid, const ChnlAttrs &chnl)
{
    Logs::ft(&H248Endpt_UpdateConn);

    //  If CID is the incoming connection, apply the new attributes.
    //
    if(rxConn_ == cid) return MakeConnection(chnl);
  
    return Ok;
}

/*---------------------------------------------------------------------------*/

const string H248Endpt_UpdateConns = "H248Endpt.UpdateConns";


H248Endpt::Result H248Endpt::UpdateConns(void)
{
    Logs::ft(&H248Endpt_UpdateConns);

    Result    res;
    ChnlAttrs chnl;

    //  If an incoming connection exists, reestablish it.  Tell each remote
    //  MEP to update its connection.
    //
    if(rxConn_ != H248Conn::NilId)
    {
        res = MakeRxConn(rxConn_);

        if(res != Ok) return res;
    }

    for(H248Conn::Id cid = 0; cid <= H248Conn::MaxId; cid++)
    {
        H248Conn *conn = &conns_[cid];

        if(conn->remMep != NULL)
        {
            GetChnlAttrs(cid, chnl);
            res = conn->remMep->UpdateConn(conn->remConn, chnl);
        }
    }

    return Ok;
}
/*===========================================================================*/

ChnlTrace::ChnlTrace(ProtocolSM &psm, const ChnlAttrs &attrs, H248Endpt::Result mer) :
    TraceRecord(sizeof(ChnlTrace))
{
    psm_  = psm.Pid();
    cid_  = attrs.chnl.cid;
    tid_  = attrs.chnl.tid;
    port_ = attrs.chnl.eph.port;
    tx_   = attrs.tx;
    mer_  = mer;

    Singleton < Tracer >::Instance()->Unlock();
}

/*---------------------------------------------------------------------------*/

ChnlTrace::~ChnlTrace(void)
{
}

/*---------------------------------------------------------------------------*/

void ChnlTrace::Display(ostream &stream, int32 bid)
{
    TraceRecord::Display(stream, bid);

    stream << spaces(Tracer::EvtToIdRc) << "psm=";
    stream << psm_ << spaces(Tracer::IdRcWidth - (intWidth(psm_, false) + 4));
    stream << "ctx=" << cid_ << " ";
    stream << "term=" << tid_ << " " << endl;

    stream << spaces(Tracer::StartToDesc);
    stream << "port=" << port_ << " ";
    stream << "tx="   << tx_ << " ";
    stream << "res="   << MerString[mer_];
}

/*---------------------------------------------------------------------------*/

const string ChnlEventStr = " chnl";


const string *ChnlTrace::EventString(void) const
{
    return &ChnlEventStr;
}

/*---------------------------------------------------------------------------*/

TraceRecord::ToolId ChnlTrace::Owner(void) const
{
    return ContextTracer;
}

/*===========================================================================*/

ConnTrace::ConnTrace(ProtocolSM &psm, const H248Conn *conn, H248Endpt::Result mer) :
    TraceRecord(sizeof(ConnTrace))
{
    mer_    = mer;
    locPsm_ = psm.Pid();

    if(conn != NULL)
    {
        rx_ = conn->rxEnabled;
        tx_ = conn->txEnabled;

        if(conn->remMep != NULL)
            remPsm_ = conn->remMep->Psm()->Pid();
        else
            remPsm_ = ProtocolSM::NilId;
    }
    else
    {
        rx_     = false;
        tx_     = false;
        remPsm_ = ProtocolSM::NilId;
    }

    Singleton < Tracer >::Instance()->Unlock();
}

/*---------------------------------------------------------------------------*/

ConnTrace::~ConnTrace(void)
{
}

/*---------------------------------------------------------------------------*/

void ConnTrace::Display(ostream &stream, int32 bid)
{
    TraceRecord::Display(stream, bid);

    stream << spaces(Tracer::EvtToIdRc) << "psm=";
    stream << locPsm_ << spaces(Tracer::IdRcWidth - (intWidth(locPsm_, false) + 4));
    stream << "tx=" << tx_ << " ";
    stream << "rx=" << rx_ << " ";
    stream << "rempsm=" << remPsm_ << " ";
    stream << "res=" << MerString[mer_];
}

/*---------------------------------------------------------------------------*/

const string ConnEventStr = " conn";


const string *ConnTrace::EventString(void) const
{
    return &ConnEventStr;
}

/*---------------------------------------------------------------------------*/

TraceRecord::ToolId ConnTrace::Owner(void) const
{
    return ContextTracer;
}
