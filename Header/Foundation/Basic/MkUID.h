////////////////////////////////////////////////////////////////////////////////////////////////////
//  COPYRIGHT (C) 2026 by Michael R. Kleist
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

// Mk includes.
#include "Basic\MkString.h"

// Windows includes.
#include <guiddef.h>
#include <objbase.h>


////////////////////////////////////////////////////////////////////////////////////////////////////
// MkUID
////////////////////////////////////////////////////////////////////////////////////////////////////

class MK_EXPORT_FOUNDATION MkUID
{
public:
    MkUID(const bool create = true)
    { if (create) ::CoCreateGuid(&m_uid); }

    MkUID(const GUID& uid)
    { m_uid = uid; }

    MkUID(const MkAString& uidStr);

    ~MkUID() = default;

    bool operator==(const MkUID& rhs) const
    { return m_uid == rhs.m_uid; }

    bool operator!=(const MkUID& rhs) const 
    { return !(*this == rhs); }

    bool operator<(const MkUID& rhs) const
    { return std::memcmp(&m_uid, &rhs.m_uid, sizeof(GUID)) < 0; }

    bool operator>(const MkUID& rhs) const
    { return std::memcmp(&m_uid, &rhs.m_uid, sizeof(GUID)) > 0; }

    void SetUID(const GUID& uid)
    { m_uid = uid; }

private:
    GUID m_uid;
};
