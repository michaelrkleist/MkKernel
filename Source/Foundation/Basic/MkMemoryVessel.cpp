////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2025 by Michael R. Kleist
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "Foundation\MkFoundationPCH.h"
#include "Basic\MkMemoryVessel.h"

// Mk includes.
#include "Basic\MkIBinaryStream.h"
#include "InputOutput\MkDebugText.h"

// Static initializations.
namespace
{
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// MkMemoryVessel
////////////////////////////////////////////////////////////////////////////////////////////////////

MkMemoryVessel::MkMemoryVessel(mkuint64 totalCnt, const BlockCapacity blockCapacity) :
    MkIBinaryStream(),
    m_blockByteCnt(blockCapacity)
{
    AddBlock();
    while (totalCnt > 0)
    {
        if (totalCnt > m_blockByteCnt)
        {
            AddBlock();
            totalCnt -= m_blockByteCnt;
        }
        else
        {
            totalCnt = 0;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

std::pair<const std::shared_ptr<mkbyte>, const mkuint> MkMemoryVessel::GetSerializedData() const
{
    if (GetByteCount() <= m_blockByteCnt) return std::make_pair(m_blocks[0], GetByteCount());

    // And then there is the ugly case.
    const std::shared_ptr<mkbyte> rAggregateVessel(new mkbyte[GetByteCount()]);

    // Allow for there to be more vessels than needed in case of re-use.
    mkuint64 byteIdx = 0;
    auto vesselIter = m_blocks.begin();
    while (byteIdx < m_byteCnt)
    {
        memcpy(
                rAggregateVessel.get() + byteIdx,
                vesselIter->get(),
                MkMin<mkuint64>(m_byteCnt - byteIdx, m_blockByteCnt));

        byteIdx += m_blockByteCnt;
        ++vesselIter;
    }

    return std::make_pair(rAggregateVessel, GetByteCount());
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void MkMemoryVessel::Serialize(const mkbyte* pObject, const mkuint64 objectByteCnt)
{
    mkuint64 blockIdx = m_byteCnt % m_blockByteCnt;

    if (objectByteCnt <= m_blockByteCnt - blockIdx)
    {
        // It fits just put it there.
        FinishSerialize(pObject, objectByteCnt);
    }
    else
    {
        // Need to spread the objects over two or more vessels.
        mkbyte* pPartialObject = const_cast<mkbyte*>(pObject);
        mkuint64 remainingByteCnt = objectByteCnt;

        PartialSerialize(pPartialObject, remainingByteCnt);
        FinishSerialize(pPartialObject, remainingByteCnt);
    }

    if (m_byteCnt > m_blockByteCnt * m_blocks.size())
    {
        MkDebugText() << "----------**********---------- Vessel Overflow ----------**********----------\n";
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void MkMemoryVessel::AlignWrite()
{
    mkuint64 prevByteCnt = m_byteCnt;
    m_byteCnt = ((m_byteCnt + 3) / 4) * 4;

    if (m_byteCnt != prevByteCnt && m_byteCnt % m_blockByteCnt == 0)
    {
        AddBlock();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void MkMemoryVessel::PartialSerialize(mkbyte*& pPartialObject, mkuint64& remainingByteCnt)
{
    // Need to spread the object over two or more vessels.
    mkuint64 blockIdx = m_byteCnt % m_blockByteCnt;

    while (remainingByteCnt > m_blockByteCnt - blockIdx)
    {
        mkuint64 vesselObjectByteCnt = m_blockByteCnt - blockIdx;

        ::memcpy(
                m_blocks.back().get() + m_blockByteCnt - vesselObjectByteCnt,
                pPartialObject,
                vesselObjectByteCnt);
        m_byteCnt += vesselObjectByteCnt;

        AddBlock();
        blockIdx = 0;

        pPartialObject += vesselObjectByteCnt;
        remainingByteCnt -= vesselObjectByteCnt;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void MkMemoryVessel::FinishSerialize(const mkbyte* pPartialObject, const mkuint64 remainingByteCnt)
{
    mkuint64 blockIdx = m_byteCnt % m_blockByteCnt;

    ::memcpy(
            m_blocks.back().get() + blockIdx,
            pPartialObject,
            remainingByteCnt);
    m_byteCnt += remainingByteCnt;

    if ((blockIdx + remainingByteCnt) == m_blockByteCnt)
    {
        AddBlock();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void MkMemoryVessel::Deserialize(mkbyte* pObject, const mkuint64 objectByteCnt)
{
    mkuint64 blocksIdx = m_byteIdx / m_blockByteCnt;
    mkuint64 blockIdx = m_byteIdx % m_blockByteCnt;

    m_byteIdx += objectByteCnt;

    if (objectByteCnt <= m_blockByteCnt - blockIdx)
    {
        // It is all there, gather it up.
        FinishDeserialize(pObject, objectByteCnt, blocksIdx, blockIdx);
    }
    else
    {
        // It is spread over two or more vessels.
        mkbyte* pPartialObject = pObject;
        mkuint64 remainingByteCnt = objectByteCnt;

        PartialDeserialize(pPartialObject, remainingByteCnt, blocksIdx, blockIdx);
        FinishDeserialize(pPartialObject, remainingByteCnt, blocksIdx, blockIdx);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void MkMemoryVessel::PartialDeserialize(
        mkbyte*& pPartialObject,
        mkuint64& remainingByteCnt,
        mkuint64& blocksIdx,
        mkuint64& blockIdx)
{
    // Need to gather the object from two or more vessels.
    while (remainingByteCnt > m_blockByteCnt - blockIdx)
    {
        mkuint64 vesselObjectByteCnt = m_blockByteCnt - blockIdx;

        ::memcpy(
                pPartialObject,
                m_blocks[blocksIdx].get() + m_blockByteCnt - vesselObjectByteCnt,
                vesselObjectByteCnt);

        pPartialObject += vesselObjectByteCnt;
        remainingByteCnt -= vesselObjectByteCnt;

        ++blocksIdx;
        blockIdx = 0;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void MkMemoryVessel::FinishDeserialize(
        mkbyte* pPartialObject,
        const mkuint64 remainingObjectByteCnt,
        mkuint64& blocksIdx,
        mkuint64& blockIdx)
{
    ::memcpy(
            pPartialObject,
            m_blocks[blocksIdx].get() + blockIdx,
            remainingObjectByteCnt);
    blockIdx += remainingObjectByteCnt;

    if (blockIdx == m_blockByteCnt)
    {
        ++blocksIdx;
        blockIdx = 0;
    }
}