////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2026 by Michael R. Kleist
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "Framework\MkFrameworkPCH.h"
#include "InputOutput\MkMBTP.h"

// Mk includes.
#include "Basic\MkMemoryVessel.h"
#include "Process\MkFrameworkApplication.h"


////////////////////////////////////////////////////////////////////////////////////////////////////
// MkMBTPReader
////////////////////////////////////////////////////////////////////////////////////////////////////

bool MkMBTPReader::ReadMessage(MkSocketIO& socket, volatile bool& complete)
{
    if (!socket.IsValid()) return false;

    // NOTE: There is some careful coding below to handle unexpected or unneeded
    // read events.  MRK.

    // NOTE: This code sequence is messy.  How can it be done better while remaining
    // robust?  MRK.
    // NOTE: This code sequence does not handle the writer simply stopping writing the
    // promised message (can you say DOS attack?).  MRK.
    // TODO: Fix the hostile writer problem?  MRK.
    complete = false;

    while (!m_readInProgress || m_headerInProgress)
    {
        // Read the message header.
        mkuint32 receivedCnt;
        auto status = socket.ReadBytes(
                (mkbyte*)&m_mbtpHeader + m_receivedCnt,
                (mkuint32)(sizeof(m_mbtpHeader) - m_receivedCnt),
                receivedCnt);
        if (status == MkSocket::eReadBufferEmpty) return true;
        if (status != MkSocket::eBytesReceived) return false;

        m_readInProgress = true;
        m_receivedCnt += receivedCnt;

        // NOTE: This circumstance is likely to never happen, but...  MRK.
        if (m_receivedCnt < (mkuint64)sizeof(m_mbtpHeader))
        {
            // Set up to read the rest of the MBTP header.
            m_headerInProgress = true;
            continue;
        }

        if (!m_mbtpHeader.IsValid()) return false;

        // We have a header.  Get the message set up.
        m_rMessage = std::make_shared<MkMemoryVessel>(m_mbtpHeader.m_blockByteCnt);
        m_rMessage->Reserve(m_mbtpHeader.m_byteCnt);
        m_rMessage->WriteObject(m_mbtpHeader);
        m_headerInProgress = false;
    }

    // Read as much of the message as is available.
    while (m_receivedCnt < m_mbtpHeader.m_byteCnt)
    {
        mkuint32 blockIdx = m_receivedCnt / m_mbtpHeader.m_blockByteCnt;
        mkuint32 byteIdx = m_receivedCnt % m_mbtpHeader.m_blockByteCnt;
        mkuint32 requestedCnt = MkMin(
                mkuint64(m_mbtpHeader.m_blockByteCnt - byteIdx), m_mbtpHeader.m_byteCnt - m_receivedCnt);

        mkuint32 receivedCnt;
        auto status = socket.ReadBytes(
                m_rMessage->GetBlocks()[blockIdx].get() + byteIdx, requestedCnt, receivedCnt);
        if (status == MkSocket::eReadBufferEmpty) return true;
        if (status != MkSocket::eBytesReceived) return false;

        m_receivedCnt += receivedCnt;
        m_rMessage->SetCount(m_receivedCnt);
    }
    
    // We have the whole message.  Dispatch or execute it and clean up the wreckage.
    m_rMessage->ResetForRead();
    m_rMessage->ReadObject(m_mbtpHeader);

    // Check the hash code.
    //if (MkMBTPHashCode(*m_rMessage).GetHashCode() != m_mbtpHeader.m_hashCode) return false;

    m_rDispatchTask->SetMessage(m_rMessage);

    m_headerInProgress = false;
    m_readInProgress = false;
    m_receivedCnt = 0;
    m_rMessage = nullptr;

    // NOTE: Sometimes we are already in a perfectly good task.  MRK.
    if (m_rDispatchTask->IsInLine())
    {
        m_rDispatchTask->Execute();
    }
    else
    {
        Framework::TaskPool().Schedule(m_rDispatchTask, m_rDispatchTask->GetPriority());
    }

    complete = true;
    return true;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// MkMBTPWriter
////////////////////////////////////////////////////////////////////////////////////////////////////

bool MkMBTPWriter::WriteMessage(MkSocketIO& socket, MkMemoryVessel& message, volatile bool& complete)
{
    if (!socket.IsValid()) return false;

    complete = false;
    
    // Set up the MBTP header as needed.
    if (!m_writeInProgress)
    {
        // Calculate hash code.
        m_mbtpHeader.m_hashCode = MkMBTPHashCode(message).GetHashCode();
        m_mbtpHeader.m_blockByteCnt = message.GetBlockByteCount();
        m_mbtpHeader.m_byteCnt = message.GetByteCount();

        // NOTE: Get the hash code into the message.  MRK.
        message.ReserializeHeader((mkbyte*)&m_mbtpHeader, sizeof(m_mbtpHeader));

        m_writeInProgress = true;
    }

    while (m_sentCnt < (mkuint32)message.GetByteCount())
    {
        mkuint32 blockIdx = m_sentCnt / m_mbtpHeader.m_blockByteCnt;
        mkuint32 byteIdx = m_sentCnt % m_mbtpHeader.m_blockByteCnt;
        mkuint32 requestedCnt = MkMin(
                mkuint64(m_mbtpHeader.m_blockByteCnt - byteIdx), m_mbtpHeader.m_byteCnt - m_sentCnt);

        mkuint32 sentCnt;
        auto status = socket.WriteBytes(
                message.GetBlocks()[blockIdx].get() + byteIdx, requestedCnt, sentCnt);
        if (status == MkSocket::eWriteBufferFull) return true;
        if (status != MkSocket::eBytesReceived) return false;

        m_sentCnt += sentCnt;
    }

    // We have sent the whole message.  Clean up the wreckage.
    m_writeInProgress = false;
    m_sentCnt = 0;

    complete = true;
    return true;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// MkMBTPHashCode
////////////////////////////////////////////////////////////////////////////////////////////////////

MkMBTPHashCode::MkMBTPHashCode(const MkMemoryVessel& message) :
    m_hashCode()
{
    mkuint64 runningByteCnt = 0;
    mkuint64 startByteIdx = sizeof(MkHashCode);
    while (runningByteCnt < message.GetByteCount() - sizeof(MkHashCode))
    {
        mkuint32 blockIdx = runningByteCnt / message.GetBlockByteCount();
        mkuint32 hashByteCnt =
                MkMin(mkuint64(message.GetBlockByteCount()), message.GetByteCount() - runningByteCnt) -
                startByteIdx;
        m_hashCode.Hash(message.GetBlocks()[blockIdx].get() + startByteIdx, hashByteCnt);

        runningByteCnt += hashByteCnt;
        startByteIdx = 0;
    }
}