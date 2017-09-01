//=============================================================================
//
//  TpfTcpArpaInputHandler.cpp
//
//  Copyright (C) 2013-2014 Greg Utas.  All rights reserved.
//
#include <string>
#include <iostream>
#include "SysCalls.h"
#include "Formatters.h"
#include "Logs.h"
#include "Objects.h"
#include "PooledObjects.h"
#include "TcpInputHandler.h"
#include "TcpArpaInputHandler.h"
#include "IpBuffer.h"
#include "Sessions.h"

using namespace std;
using namespace NodeBase;
using namespace SessionBase;

//-----------------------------------------------------------------------------

const char *TpfArpaHeader_ContentLength = "Content-Length:";

//-----------------------------------------------------------------------------

const string TpfTcpArpaInputHandlerTpfTcpArpaInputHandler = "TpfTcpArpaInputHandler::TpfTcpArpaInputHandler";


TpfTcpArpaInputHandler::TpfTcpArpaInputHandler(void)
{
    Logs::ft(&TpfTcpArpaInputHandlerTpfTcpArpaInputHandler);

    SysMemory::Set(arpaBuff_, 0, sizeof(TcpArpaParseBuff) * TcpIoThread::MaxConns);
    numUsedBuff_ = 0;
    for(uint16 i = 0; i < TcpIoThread::MaxConns; i++) usedBuff_[i] = i;
}

//-----------------------------------------------------------------------------

const string TpfTcpArpaInputHandler_TpfTcpArpaInputHandler = "TpfTcpArpaInputHandler::~TpfTcpArpaInputHandler";


TpfTcpArpaInputHandler::~TpfTcpArpaInputHandler(void)
{
    Logs::ft(&TpfTcpArpaInputHandlerTpfTcpArpaInputHandler);
}

//-----------------------------------------------------------------------------

void TpfTcpArpaInputHandler::Display(ostream &stream, uint8 indent, bool verbose)
{
    TcpInputHandler::Display(stream, indent, verbose);

    indent = indent + 2;

    stream << spaces(indent) << "numUsedBuff : " << numUsedBuff_ << endl;

    for(uint16 i = 0; i < numUsedBuff_; i++)
    {
        stream << spaces(indent) << "[" << i << "]" << endl;
        uint16 j = usedBuff_[i];
        stream << spaces(indent+2) << "arpaBuff      : " << j << endl;
        TcpArpaParseBuff &buff = arpaBuff_[j];
        stream << spaces(indent+2) << "length        : " << buff.length << endl;
        stream << spaces(indent+2) << "state         : " << buff.state << endl;
        stream << spaces(indent+2) << "socket        : " << buff.socket << endl;
        stream << spaces(indent+2) << "bodyLength    : " << buff.bodyLength << endl;
        stream << spaces(indent+2) << "headerLength  : " << buff.headerLength << endl;
        if(buff.length < MaxMsgSize)
            buff.buff[buff.length] = '\0';
        stream << spaces(indent+2) << "buff          : " << endl << buff.buff << endl;
    }
}

//-----------------------------------------------------------------------------

void TpfTcpArpaInputHandler::Patch(uint8 selector, void *arguments)
{
    TcpInputHandler::Patch(selector, arguments);
}

//-----------------------------------------------------------------------------

const string TpfTcpArpaInputHandlerReceiveBuff = "TpfTcpArpaInputHandler::ReceiveBuff";


void TpfTcpArpaInputHandler::ReceiveBuff
(
    PooledObject*   &wrapper,
    MsgSize         length,
    IpL3Address     txAddr,
    IpL3Address     rxAddr,
    SysClock::Ticks rxTime
)
{
    Logs::ft(&TpfTcpArpaInputHandlerReceiveBuff);

    TcpInputHandler::ReceiveBuff(wrapper, length, txAddr, rxAddr, rxTime);
}

//-----------------------------------------------------------------------------

const string TpfTcpArpaInputHandlerProcessBuff = "TpfTcpArpaInputHandler::ProcessBuff";


TcpArpaParseRc TpfTcpArpaInputHandler::ProcessBuff
(
    PooledObject*   &wrapper,
    MsgSize         &length,
    SysSocket       *socket
)
{
    Logs::ft(&TpfTcpArpaInputHandlerProcessBuff);

    IpBuffer *buff = (IpBuffer *)wrapper;
    MsgLayout *layout = (MsgLayout*)(buff->Header());
    char *inBuff = (char*)(layout->bytes);

    uint16 iUsedBuff = GetUsedBuff(socket);
    uint16 iArpaBuff = usedBuff_[iUsedBuff];

    if(arpaBuff_[iArpaBuff].socket == NULL)
    {
        arpaBuff_[iArpaBuff].socket = socket;
        numUsedBuff_++;
    }

    char outBuff[MaxMsgSize];
    MsgSize outLen = 0;
    TcpArpaParseRc rc = AccumulateBuff
                         (inBuff, length, outBuff, outLen,
                          MaxMsgSize, arpaBuff_[iArpaBuff]);

    if(rc == TCP_ARPA_PARSE_RC_COMPLETE)
    {
        PooledObject *newWrapper;
        bytestream   ihBuffer = AllocBuff(outLen, newWrapper);

        if(!ihBuffer)
        {
            Logs::SwErr(&TpfTcpArpaInputHandlerProcessBuff, outLen, 1);

            if(!FreeBuff(iUsedBuff))
                Logs::SwErr(&TpfTcpArpaInputHandlerProcessBuff, iUsedBuff, 2);

            return TCP_ARPA_PARSE_RC_ERROR;
        }

        if(wrapper) delete wrapper;
        wrapper = newWrapper;
        IpBuffer *ipBuff = (IpBuffer*) wrapper;
        bool moved = false;
        ipBuff->AddBytes((bytestream) outBuff, outLen, moved);
        length = outLen;

        if(arpaBuff_[iArpaBuff].length == 0)
        {
            if(!FreeBuff(iUsedBuff))
                Logs::SwErr(&TpfTcpArpaInputHandlerProcessBuff, iUsedBuff, 3);
        }

        return TCP_ARPA_PARSE_RC_COMPLETE;
    }
    else
    {
        if(rc == TCP_ARPA_PARSE_RC_INCOMPLETE) return rc;
    }

    Logs::SwErr(&TpfTcpArpaInputHandlerProcessBuff, rc, 4);

    if(!FreeBuff(iUsedBuff))
        Logs::SwErr(&TpfTcpArpaInputHandlerProcessBuff, iUsedBuff, 5);

    return rc;
}

//-----------------------------------------------------------------------------

const string TpfTcpArpaInputHandlerFreeSocket = "TpfTcpArpaInputHandler::FreeSocket";


bool TpfTcpArpaInputHandler::FreeSocket(SysSocket *socket)
{
    Logs::ft(&TpfTcpArpaInputHandlerFreeSocket);

    return FreeBuff(GetUsedBuff(socket));
}

//-----------------------------------------------------------------------------

const string TpfTcpArpaInputHandlerAccumulateBuff = "TpfTcpArpaInputHandler::AccumulateBuff";


TcpArpaParseRc TpfTcpArpaInputHandler::AccumulateBuff
(
    const char       *inBuff,
    MsgSize          inLen,
    char             *outBuff,
    MsgSize          &outLen,
    MsgSize          outSize,
    TcpArpaParseBuff &arpaBuff
)
{
    Logs::ft(&TpfTcpArpaInputHandlerAccumulateBuff);

    MsgSize &tmpLen = arpaBuff.length;
    char   *tmpBuff  = arpaBuff.buff;
    MsgSize &bodyLength = arpaBuff.bodyLength;
    MsgSize &headerLength = arpaBuff.headerLength;
    TcpArpaParseState &state = arpaBuff.state;
    size_t headerStringLength = strlen(TpfArpaHeader_ContentLength);

    if(tmpLen + inLen > MaxMsgSize)
    {
        Logs::SwErr(&TpfTcpArpaInputHandlerAccumulateBuff, tmpLen + inLen, 0);
        return TCP_ARPA_PARSE_RC_TMPBUFF_OVERFLOW;
    }

    if(inLen > 0)
    {
        SysMemory::Copy(&(tmpBuff[tmpLen]), inBuff, inLen);
        tmpLen += inLen;
    }

    if(state == TCP_ARPA_PARSE_STATE_INIT)
    {
        MsgSize index = tmpLen-inLen;

        while((index > 0) && ((tmpBuff[index-1] == '\r') ||
              (tmpBuff[index-1] == '\n')))
            index--;

        for(; index < tmpLen; index++)
        {
            if((tmpBuff[index] == '\r') && (tmpBuff[index+1] == '\n') &&
               (tmpBuff[index+2] == '\r') && (tmpBuff[index+3] == '\n'))
            {
                headerLength = index + 4;

                if(headerLength < headerStringLength)
                {
                    Logs::SwErr(&TpfTcpArpaInputHandlerAccumulateBuff,
                                headerLength, 1);
                    return TCP_ARPA_PARSE_RC_ERROR;
                }

                state = TCP_ARPA_PARSE_STATE_HEADER;
                break;
            }
        }

        if(state != TCP_ARPA_PARSE_STATE_HEADER)
            return TCP_ARPA_PARSE_RC_INCOMPLETE;
    }

    if(state == TCP_ARPA_PARSE_STATE_HEADER)
    {
        for(size_t i = 0; i < headerLength - headerStringLength; i++)
        {
            if(strncmp(&tmpBuff[i], TpfArpaHeader_ContentLength,
                       headerStringLength) == 0)
            {
                i += headerStringLength;

                if(i >= headerLength) break;

                while((tmpBuff[i] == ' ') || (tmpBuff[i] == '\t'))
                {
                    if(i >= headerLength)
                        break;
                    else
                        i++;
                }

                if(i >= headerLength) break;

                bodyLength = atoi(&tmpBuff[i]);
                state = TCP_ARPA_PARSE_STATE_BODY;
                break;
            }
        }

        if(state != TCP_ARPA_PARSE_STATE_BODY)
        {
            Logs::SwErr(&TpfTcpArpaInputHandlerAccumulateBuff, state, 2);
            return TCP_ARPA_PARSE_RC_ERROR;
        }
    }

    if(state == TCP_ARPA_PARSE_STATE_BODY)
    {
        if(headerLength + bodyLength > tmpLen)
            return TCP_ARPA_PARSE_RC_INCOMPLETE;

        if(headerLength + bodyLength > outSize)
        {
            Logs::SwErr(&TpfTcpArpaInputHandlerAccumulateBuff,
                        headerLength + bodyLength, 3);
            return TCP_ARPA_PARSE_RC_OUTBUFF_OVERFLOW;
        }

        outLen = headerLength + bodyLength;
        SysMemory::Copy(outBuff, tmpBuff, outLen);

        tmpLen -= outLen;
        SysMemory::Copy(tmpBuff, &tmpBuff[outLen], tmpLen);
        bodyLength = 0;
        headerLength = 0;
        state = TCP_ARPA_PARSE_STATE_INIT;
        return TCP_ARPA_PARSE_RC_COMPLETE;
    }
    else
    {
        Logs::SwErr(&TpfTcpArpaInputHandlerAccumulateBuff, state, 4);
        return TCP_ARPA_PARSE_RC_ERROR;
    }

    Logs::SwErr(&TpfTcpArpaInputHandlerAccumulateBuff, state, 5);
    return TCP_ARPA_PARSE_RC_ERROR;
}

//-----------------------------------------------------------------------------

const string TpfTcpArpaInputHandlerGetUsedBuff = "TpfTcpArpaInputHandler::GetUsedBuff";


uint16 TpfTcpArpaInputHandler::GetUsedBuff(SysSocket *socket)
{
    Logs::ft(&TpfTcpArpaInputHandlerGetUsedBuff);

    uint16 i;

    //@ This should be optimized, but it isn't as bad as it looks.
    //  Only incomplete messages are held in arpaBuff_.  Once a message
    //  is complete and there are no extra bytes, the buffer is released.
    //
    for(i = 0; i < numUsedBuff_; i++)
    {
        if(arpaBuff_[usedBuff_[i]].socket == socket) return i;
    }

    return i;
}

//-----------------------------------------------------------------------------

const string TpfTcpArpaInputHandlerFreeBuff = "TpfTcpArpaInputHandler::FreeBuff";


bool TpfTcpArpaInputHandler::FreeBuff(uint16 iUsedBuff)
{
    Logs::ft(&TpfTcpArpaInputHandlerFreeBuff);

    if(iUsedBuff >= numUsedBuff_) return false;

    uint16 iArpaBuff = usedBuff_[iUsedBuff];

    arpaBuff_[iArpaBuff].socket = NULL;
    arpaBuff_[iArpaBuff].length = 0;
    arpaBuff_[iArpaBuff].state = TCP_ARPA_PARSE_STATE_INIT;

    usedBuff_[iUsedBuff] = usedBuff_[numUsedBuff_-1];
    usedBuff_[numUsedBuff_-1] = iArpaBuff;
    numUsedBuff_--;

    return true;
}
