////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2025 by Michael R. Kleist
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

// Mk includes.
#include "MkFoundation.h"
#include "Basic/MkIBinaryStream.h"


////////////////////////////////////////////////////////////////////////////////////////////////////
// MkMemoryVessel
////////////////////////////////////////////////////////////////////////////////////////////////////

class MkMemoryVessel : public MkIBinaryStream
{
public:
    enum BlockCapacity : mkuint32
    {
        eTiny = 1024 * 1,
        eSmall = 1024 * 16,
        eMedium = 1024 * 32,
        eLarge = 1024 * 64
    };

    MK_EXPORT_FOUNDATION MkMemoryVessel(const BlockCapacity blockCapacity = eSmall) :
        MkIBinaryStream(),
        m_blockByteCnt(blockCapacity)
    { m_blocks.push_back(std::shared_ptr<mkbyte>(new mkbyte[m_blockByteCnt])); }

    // An internal structure aware constructor for supporting efficient filling from
    // an input stream.
    MK_EXPORT_FOUNDATION MkMemoryVessel(mkuint64 totalCnt, const BlockCapacity blockCapacity = eSmall);

    MK_EXPORT_FOUNDATION ~MkMemoryVessel() override = default;

    std::pair<const std::shared_ptr<mkbyte>, const mkuint> GetSerializedData() const;

    void SetSerializedData(void* pData, mkuint64 dataByteCnt)
    {
        mkbyte* pPartialData = (mkbyte*)pData;
        mkuint64 remainingByteCnt = dataByteCnt;
        Serialize(pPartialData, remainingByteCnt);
    }

    void ResetForWrite()
    { m_byteCnt = 0; m_byteIdx = 0; }

    void ResetForRead()
    { m_byteIdx = 0; }

    const mkuint64 GetByteCount() const
    { return m_byteCnt; }

    const mkuint64 GetByteCapacity() const
    { return static_cast<mkuint64>(m_blockByteCnt) * static_cast<mkuint64>(m_blocks.size()); }

    // TODO: Introduce support for hash code.  MRK.
    void GenerateHashCode() override
    { }

    bool CheckHashCode() override
    { return true; }

protected:
    void Serialize(const mkbyte* pObject, const mkuint64 objectByteCnt) override;
    void Deserialize(mkbyte* pObject, const mkuint64 objectByteCnt) override;

    void AlignWrite() override;
    void AlignRead() override
    { m_byteIdx = ((m_byteIdx + 3) / 4) * 4; }

private:
    friend class MkMBTPReader;
    friend class MkMBTPWriter;
    friend class MkMBTPHashCode;

    // Internal structure aware interfaces for supporting efficient I/O.
    const std::vector<std::shared_ptr<mkbyte>>& GetBlocks() const
    { return m_blocks; }

    std::vector<std::shared_ptr<mkbyte>>& GetBlocks()
    { return m_blocks; }

    const mkuint32 GetBlockByteCount() const
    { return m_blockByteCnt; }

    void Reserve(const mkuint64 byteCnt)
    { while (GetByteCapacity() < byteCnt) AddBlock(); }
    
    void SetCount(const mkuint64 byteCnt)
    { m_byteCnt = byteCnt; }

    // NOTE: Write message header that leaves the count and index unmodified.  MRK.
    void ReserializeHeader(const mkbyte* pObject, const mkuint32 objectByteCnt) const
    { ::memcpy((void*)m_blocks[0].get(), pObject, objectByteCnt); }

    // End of internal structure aware interfaces for supporting efficient I/O.

    // Helper methods.
    void PartialSerialize(mkbyte*& pPartialObject, mkuint64& remainingByteCnt);
    void FinishSerialize(const mkbyte* pPartialObject, const mkuint64 remainingByteCnt);

    void PartialDeserialize(
            mkbyte*& pPartialObject,
            mkuint64& remainingByteCnt,
            mkuint64& buffersIdx,
            mkuint64& bufferIdx);
    void FinishDeserialize(
            mkbyte* pPartialObject,
            const mkuint64 remainingByteCnt,
            mkuint64& buffersIdx,
            mkuint64& bufferIdx);

    void AddBlock()
    { m_blocks.push_back(std::shared_ptr<mkbyte>(new mkbyte[m_blockByteCnt])); }

    void RemoveBlock()
    { m_blocks.pop_back(); }

    std::vector<std::shared_ptr<mkbyte>> m_blocks;
    mkuint64 m_byteIdx = 0;
    mkuint64 m_byteCnt = 0;

    mkuint32 m_blockByteCnt;
};