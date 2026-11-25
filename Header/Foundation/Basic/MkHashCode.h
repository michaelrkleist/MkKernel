////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2026 by Michael R. Kleist
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

// Uses the FNV-1a algorithm.
// NOTE: It is unknown whether this algorithm introduces any performance or collision
// issues when it is used in this kernel.  MRK.

// Mk includes.
#include "MkFoundation.h"


////////////////////////////////////////////////////////////////////////////////////////////////////
// MkHashCode
////////////////////////////////////////////////////////////////////////////////////////////////////

class MK_EXPORT_FOUNDATION MkHashCode
{
public:
    MkHashCode() :
        m_hashCode(14695981039346656037ull)     // The FNV offset basis.
    { }
    ~MkHashCode() = default;

    bool operator==(const MkHashCode rhs) const
    { return bool(m_hashCode == rhs.m_hashCode); }

    bool operator!=(const MkHashCode rhs) const
    { return bool(m_hashCode != rhs.m_hashCode); }

    bool operator<(const MkHashCode rhs) const
    { return bool(m_hashCode < rhs.m_hashCode); }

    MkHashCode& Hash(mkbyte* pBytes, const mkuint64 byteCnt);

private:
    mkuint64 m_hashCode;
};