////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2026 by Michael R. Kleist
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

// Mk includes.
#include "MkFramework.h"
#include "Basic\MkHashCode.h"
#include "InputOutput\MkSocket.h"
#include "Process\MkTask.h"

// Forward declarations.
class MkMemoryVessel;

// The MBTP (Michael's Binary Transport Protocol) is designed for event driven transfer
// of binary serialized data.  Every message starts with a MBTP header that includes
// a 64 bit hash code for the entire message (excluding the hash code itself, of course),
// a 'MBTP' stamp, a byte count for the entire message and a block byte count for memory
// vessel (see MkMemoryVessel.h) used to serialize the message content.  The MBTP class
// is designed for event driven message writing and reading.  MRK.

// NOTE: While the protocol can and is used for synchronous request/reply communcation,
// it is designed for one-way or asynchrouns request/reply communcation.


////////////////////////////////////////////////////////////////////////////////////////////////////
// MkMBTPHeader
////////////////////////////////////////////////////////////////////////////////////////////////////

struct MkMBTPHeader
{
public:
    MkMBTPHeader() :
        m_hashCode(),
        m_byteCnt(0),
        m_blockByteCnt(0)
    { m_mbtp = {'M', 'B', 'T', 'P'}; }
    
    MkMBTPHeader(const MkHashCode hashCode, mkuint32 byteCnt, mkuint32 blockByteCnt) :
        m_hashCode(hashCode),
        m_byteCnt(byteCnt),
        m_blockByteCnt(blockByteCnt)
    { m_mbtp = {'M', 'B', 'T', 'P'}; }

    ~MkMBTPHeader() = default;

    bool operator==(const MkMBTPHeader& rhs) const
    { return m_mbtp == rhs.m_mbtp; }

    bool operator!=(const MkMBTPHeader& rhs) const
    { return m_mbtp != rhs.m_mbtp; }

    bool IsValid() const
    { return bool(m_mbtp[0] == 'M' &&  m_mbtp[1] == 'B' && m_mbtp[2] == 'T' && m_mbtp[3] == 'P'); }

#pragma pack(push, 1)
    MkHashCode m_hashCode;
    std::array<mkchar8, 4> m_mbtp;
    mkuint32 m_blockByteCnt;
    mkuint64 m_byteCnt;
#pragma pack(pop)
};


////////////////////////////////////////////////////////////////////////////////////////////////////
// MkMBTPReader
////////////////////////////////////////////////////////////////////////////////////////////////////

class MkMBTPReader
{
public:
    class DispatchTask : public MkTask
    {
    public:
        DispatchTask(
                const MkAString& name, bool inLine = false, MkTask::Priority priority = MkTask::eNormal) :
            MkTask(name),
            m_priority(priority),
            m_inLine(inLine)
        { }
        ~DispatchTask() = default;

        void SetMessage(std::shared_ptr<MkMemoryVessel>& rMessage)
        { m_rMessage = rMessage; }

        MkTask::Priority GetPriority() const
        { return m_priority; }

        bool IsInLine() const
        { return m_inLine; }

    protected:
        std::shared_ptr<MkMemoryVessel> m_rMessage = nullptr;
        MkTask::Priority m_priority = MkTask::eNormal;
        bool m_inLine;
    };

    MkMBTPReader(const std::shared_ptr<DispatchTask>& rDispatchTask) : 
        m_rDispatchTask(rDispatchTask),
        m_headerInProgress(false),
        m_readInProgress(false),
        m_receivedCnt(0),
        m_rMessage(nullptr)
    { }

    ~MkMBTPReader() = default;

    // NOTE: The dispatch task is called upon message completion.  MRK.
    // NOTE: Re-entry must be prevented external to this call.  MRK.
    bool ReadMessage(MkSocketIO& socket, volatile bool& complete);

private:
    bool m_headerInProgress;
    bool m_readInProgress;      // Is inclusive of header in progress.
    mkuint64 m_receivedCnt;
    MkMBTPHeader m_mbtpHeader;
    std::shared_ptr<MkMemoryVessel> m_rMessage;
    std::shared_ptr<DispatchTask> m_rDispatchTask;
};


////////////////////////////////////////////////////////////////////////////////////////////////////
// MkMBTPWriter
////////////////////////////////////////////////////////////////////////////////////////////////////

class MkMBTPWriter
{
public:
    MkMBTPWriter() :
        m_writeInProgress(false),
        m_sentCnt(0)
    { }

    ~MkMBTPWriter() = default;

    // NOTE: WriteMessage fills in the hash code and byte counts of the MBTP header.  MRK.
    bool WriteMessage(MkSocketIO& socket, MkMemoryVessel& message, volatile bool& complete);

private:
    bool m_writeInProgress;
    mkuint64 m_sentCnt;
    MkMBTPHeader m_mbtpHeader;
};


////////////////////////////////////////////////////////////////////////////////////////////////////
// MkMBTPHashCode
////////////////////////////////////////////////////////////////////////////////////////////////////

class MkMBTPHashCode
{
public:
    MkMBTPHashCode(const MkMemoryVessel& message);
    ~MkMBTPHashCode() = default;

    MkHashCode GetHashCode() const
    { return m_hashCode; }

private:
    MkHashCode m_hashCode;
};
