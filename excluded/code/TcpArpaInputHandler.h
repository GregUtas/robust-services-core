//=============================================================================
//
//  TpfTcpArpaInputHandler.h
//
//  Copyright (C) 2013-2014 Greg Utas.  All rights reserved.
//
#ifndef _TPFTCPARPAINPUTHANDLER_H_INCLUDED_
#define _TPFTCPARPAINPUTHANDLER_H_INCLUDED_

#include <iostream>
#include "SysDefs.h"
#include "SysCalls.h"
#include "Objects.h"
#include "PooledObjects.h"
#include "Threads.h"
#include "TcpInputHandler.h"
#include "TcpIoThread.h"
#include "Sessions.h"

//------------------------------------------------------------------------------
//
//  Types for ARPA messages on TCP.
//
enum TcpArpaParseState
{
    TCP_ARPA_PARSE_STATE_INIT,
    TCP_ARPA_PARSE_STATE_HEADER,
    TCP_ARPA_PARSE_STATE_BODY
};

struct TcpArpaParseBuff
{
    MsgSize             length;
    char                buff[MaxMsgSize];
    TcpArpaParseState   state;
    MsgSize             bodyLength;
    MsgSize             headerLength;
    SysSocket           *socket;
};

enum TcpArpaParseRc
{
    TCP_ARPA_PARSE_RC_INCOMPLETE = 0,
    TCP_ARPA_PARSE_RC_COMPLETE,
    TCP_ARPA_PARSE_RC_TMPBUFF_OVERFLOW,
    TCP_ARPA_PARSE_RC_OUTBUFF_OVERFLOW,
    TCP_ARPA_PARSE_RC_ERROR
};

//------------------------------------------------------------------------------

class TpfTcpArpaInputHandler :
    public TcpInputHandler
{
public:
    virtual void    ReceiveBuff
                    (
                        PooledObject*   &wrapper,
                        MsgSize         length,
                        IpL3Address     txAddr,
                        IpL3Address     rxAddr,
                        SysClock::Ticks rxTime
                    );

    virtual void    Display(ostream &stream, uint8 indent, bool verbose);

    virtual void    Patch(uint8 selector, void *arguments);
protected:
    TpfTcpArpaInputHandler(void);

    ~TpfTcpArpaInputHandler(void);

    TcpArpaParseRc  ProcessBuff
                    (
                        PooledObject*   &wrapper,
                        MsgSize			&length,
                        SysSocket       *socket
                    );

    bool            FreeSocket(SysSocket *socket);
private:
    TcpArpaParseRc  AccumulateBuff
                    (
                        const char       *inBuff,
                        MsgSize          inLen,
                        char             *outBuff,
                        MsgSize          &outLen,
                        MsgSize          outSize,
                        TcpArpaParseBuff &arpaBuff
                    );

    uint16          GetUsedBuff(SysSocket *socket);

    bool            FreeBuff(uint16 iUsedBuff);


    TcpArpaParseBuff    arpaBuff_[TcpIoThread::MaxConns];
    uint16              usedBuff_[TcpIoThread::MaxConns];
    uint16              numUsedBuff_;
};

#endif
