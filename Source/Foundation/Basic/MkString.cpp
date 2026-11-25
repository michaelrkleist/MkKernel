////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2025 by Michael R. Kleist
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "Foundation\MkFoundationPCH.h"
#include "Basic\MkString.h"

// Mk includes.

// C++ includes.
#include <stdio.h>

// Static member initializations.
mkchar8 MkAString::m_sTerminator = 0;
//mkchar8 MkUString::m_sTerminator = 0;

// Static initializations.
namespace
{
mkchar8 sZeroChar = '0';
mkchar8 sNineChar = '9';
mkchar8 sPlusChar = '+';
mkchar8 sMinusChar = '-';
mkchar8 sDecimalPointChar = '.';
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// mkuchar
////////////////////////////////////////////////////////////////////////////////////////////////////

mkuint32 mkuchar::GetByteCount() const
{
    mkchar8 leadByte = m_bytes[0];
    if ((leadByte & 0b10000000) == 0) return 1;
    if ((leadByte & 0b11100000) == 0b11000000) return 2;
    if ((leadByte & 0b11110000) == 0b11100000) return 3;
    if ((leadByte & 0b11111000) == 0b11110000) return 4;
    return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

mkuint32 mkuchar::GetByteCount(mkchar8 leadByte)
{
    if ((leadByte & 0b10000000) == 0) return 1;
    if ((leadByte & 0b11100000) == 0b11000000) return 2;
    if ((leadByte & 0b11110000) == 0b11100000) return 3;
    if ((leadByte & 0b11111000) == 0b11110000) return 4;
    return 0;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// MkAString
////////////////////////////////////////////////////////////////////////////////////////////////////

MkAString::MkAString()
{
    // NOTE: Ahhh, the semantics of an empty string.  I hope this method is rarely
    // called.  MRK.
    mkuint32 allocCnt = RoundUp8(sizeof(mkchar8));

    Construct(allocCnt, 0, &m_sTerminator);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

MkAString::MkAString(const mkchar8* pString)
{
    mkuint32 charCnt = (mkuint32)strlen(pString);
    mkuint32 allocCnt = RoundUp8((charCnt + 1) * sizeof(mkchar8));

    Construct(allocCnt, charCnt, pString);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

MkAString::MkAString(const std::string string)
{
    mkuint32 charCnt = (mkuint32)string.length();
    mkuint32 allocCnt = RoundUp8((charCnt + 1) * sizeof(mkchar8));

    Construct(allocCnt, charCnt, string.c_str());
}

////////////////////////////////////////////////////////////////////////////////////////////////////

MkAString::MkAString(const MkAString& string, const mkuint32 startIdx, const mkuint32 charCnt)
{
    if (startIdx + charCnt > string.m_characterCnt)
    {
        mkuint32 allocCnt = RoundUp8(sizeof(mkchar8));
        Construct(allocCnt, 0, &m_sTerminator);
        return;
    }

    mkuint32 allocCnt = RoundUp8((charCnt + 1) * sizeof(mkchar8));
    Construct(allocCnt, charCnt, string.m_pBuffer + startIdx);
    *(m_pBuffer + m_characterCnt) = 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

MkAString::MkAString(const mkchar8* pString, const mkuint32 charCnt)
{
    mkuint32 allocCnt = RoundUp8((charCnt + 1) * sizeof(mkchar8));

    Construct(allocCnt, charCnt, pString);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

MkAString::MkAString(const MkAString& other)
{
    // NOTE: Exact copy or no?  I went with yes.  MRK.
    Construct(other.GetAllocatedByteCount(), other.GetCharacterCount(), other.GetCString());
}

////////////////////////////////////////////////////////////////////////////////////////////////////

MkAString::MkAString(const MkConstAString& constAString)
{
    mkuint32 constCharCnt = constAString.GetCharacterCount();
    mkuint32 allocCnt = RoundUp8((constCharCnt + 1) * sizeof(mkchar8));

    Construct(allocCnt, constCharCnt, constAString.GetCString());
}

////////////////////////////////////////////////////////////////////////////////////////////////////

const MkAString& MkAString::operator=(const MkAString& rhs)
{
    // NOTE: Exact copy or no?  I went with no.  MRK.
    if (GetCharacterCapacity() < rhs.GetCharacterCount())
    {
        delete m_pBuffer;
        m_pBuffer = new mkchar8[rhs.GetAllocatedByteCount()];
        SetAllocatedByteCount(rhs.GetAllocatedByteCount());
    }

    SetCharacterCount(rhs.GetCharacterCount());
    SetCString(rhs.GetCString(), rhs.GetCharacterCount());

    return *this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

const MkAString& MkAString::operator=(const MkConstAString& constAString)
{
    mkuint32 constCharCnt = constAString.GetCharacterCount();
    if (GetCharacterCapacity() < constCharCnt)
    {
        delete m_pBuffer;
        mkuint32 allocCnt = RoundUp8((constCharCnt + 1) * sizeof(mkchar8));
        m_pBuffer = new mkchar8[allocCnt];
        SetAllocatedByteCount(allocCnt);
    }

    SetCharacterCount(constCharCnt);
    SetCString(constAString.GetCString(), constCharCnt);

    return *this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

const MkAString& MkAString::operator+=(const MkAString& rhs)
{
    // Check capacity.
    if (GetCharacterCount() + rhs.GetCharacterCount() > GetCharacterCapacity())
    {
        // If we allocate, allocate extra assuming more adding to come.
        mkuint32 charCnt = GetCharacterCount() + rhs.GetCharacterCount();
        mkuint32 allocCnt = RoundUp8((2 * charCnt + 1) * sizeof(mkchar8));

        mkchar8* pBuffer = new mkchar8[allocCnt];
        memcpy(pBuffer, m_pBuffer, GetCharacterCount() + 1);

        delete m_pBuffer;
        m_pBuffer = pBuffer;

        SetAllocatedByteCount(allocCnt);
    }

    AddCString(rhs.GetCString(), rhs.GetCharacterCount());
    return *this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

const MkAString& MkAString::operator+=(const MkConstAString& rhs)
{
    // Check capacity.
    mkuint32 constCharCnt = rhs.GetCharacterCount();
    if (GetCharacterCount() + constCharCnt > GetCharacterCapacity())
    {
        // If we allocate, allocate extra assuming more adding to come.
        mkuint32 charCnt = GetCharacterCount() + constCharCnt;
        mkuint32 allocCnt = RoundUp8((2 * charCnt + 1) * sizeof(mkchar8));

        mkchar8* pBuffer = new mkchar8[allocCnt];
        memcpy(pBuffer, m_pBuffer, GetCharacterCount() + 1);

        delete m_pBuffer;
        m_pBuffer = pBuffer;

        SetAllocatedByteCount(allocCnt);
    }

    AddCString(rhs.GetCString(), constCharCnt);
    return *this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

const MkAString& MkAString::operator+=(const mkachar character)
{
    // Check capacity.
    if (GetCharacterCount() + 1 > GetCharacterCapacity())
    {
        // If we allocate, allocate extra assuming more adding to come.
        mkuint32 charCnt = GetCharacterCount() + 1;
        mkuint32 allocCnt = RoundUp8((2 * charCnt + 1) * sizeof(mkchar8));

        mkchar8* pBuffer = new mkchar8[allocCnt];
        memcpy(pBuffer, m_pBuffer, GetCharacterCount() + 1);

        delete m_pBuffer;
        m_pBuffer = pBuffer;

        SetAllocatedByteCount(allocCnt);
        SetCharacterCount(charCnt - 1);
    }

    AddCharacter(character);
    return *this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

MkAString MkAString::operator+(const MkAString& rhs) const
{
    MkAString string = *this;
    return string += rhs;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

MkAString MkAString::operator+(const MkConstAString& rhs) const
{
    MkAString string = *this;
    return string += rhs;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

MkAString MkAString::operator+(const mkachar character) const
{
    MkAString string = *this;
    return string += character;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

const bool MkAString::operator==(const MkAString& rhs) const
{
    auto charCnt = GetCharacterCount();
    if (charCnt != rhs.GetCharacterCount()) return false;
    
    auto pString = GetCString();
    auto pRHSString = rhs.GetCString();

    for (mkuint32 charIdx = 0; charIdx < charCnt; ++charIdx, ++pString, ++pRHSString)
    {
        if (*pString != *pRHSString) return false;
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

const bool MkAString::operator<(const MkAString& rhs) const
{
    auto pString = GetCString();
    auto pRHSString = rhs.GetCString();

    mkuint32 minCharCnt = MkMin(GetCharacterCount(), rhs.GetCharacterCount());
    for (mkuint32 charIdx = 0; charIdx < minCharCnt; ++charIdx, ++pString, ++pRHSString)
    {
        if (*pString < *pRHSString) return true;
    }

    if (GetCharacterCount() < rhs.GetCharacterCount()) return true;

    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

const std::unique_ptr<mkchar16[]> MkAString::GetWString() const
{
    auto rUString = std::make_unique<mkchar16[]>(GetCharacterCount() + 1);
    auto pAString = GetAString();

    for (mkuint32 charIdx = 0; charIdx <= GetCharacterCount(); ++charIdx)
    {
        rUString.get()[charIdx] = (mkchar16)pAString[charIdx];
    }

    return rUString;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

mkint32 MkAString::Find(const mkachar character, const mkuint32 startIdx) const
{
    mkuint32 charCnt = GetCharacterCount();
    for (mkuint32 charIdx = startIdx; charIdx < charCnt; ++charIdx)
    {
        if (m_pBuffer[charIdx] == character) return charIdx;
    }
    return -1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

mkint64 MkAString::ToMkint64() const
{
    mkint64 value = 0;
    mkint64 sign = 1;

    const mkchar8* pChar = GetCString();
    while (*pChar != m_sTerminator)
    {
        if (*pChar >= sZeroChar && *pChar <= sNineChar)
        {
            value = value * 10 + (*pChar - sZeroChar);
        }
        else if (*pChar == sMinusChar)
        {
            sign = -1;
        }

        ++pChar;
    }

    return value * sign;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

mkuint64 MkAString::ToMkuint64() const
{
    mkuint64 value = 0;

    const mkchar8* pChar = GetCString();
    while (*pChar != m_sTerminator)
    {
        if (*pChar >= sZeroChar && *pChar <= sNineChar)
        {
            value = value * 10 + (*pChar - sZeroChar);
        }
        ++pChar;
    }

    return value;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

mkfloat64 MkAString::ToMkfloat64() const
{
    mkfloat64 value = 0.0;
    mkfloat64 sign = 1.0;
    mkfloat64 divisor = -1.0;

    const mkchar8* pChar = GetCString();
    while (*pChar != m_sTerminator)
    {
        if (*pChar >= sZeroChar && *pChar <= sNineChar)
        {
            value = value * 10.0 + (*pChar - sZeroChar);
            if (divisor > 0.0) divisor = divisor * 10.0;
        }
        else if (*pChar == sMinusChar)
        {
            sign = -1.0;
        }
        else if (*pChar == sDecimalPointChar)
        {
            divisor = 1.0;
        }
        ++pChar;
    }

    return (divisor < 0.0 ? value * sign : value / divisor * sign);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

bool MkAString::Match(const MkAString& matchStr, mkuint startIdx) const
{
    if (startIdx + matchStr.m_characterCnt >= m_characterCnt) return false;

    for (mkuint matchIdx = 0; matchIdx < matchStr.m_characterCnt; ++startIdx, ++matchIdx)
    {
        if (matchStr[matchIdx] != m_pBuffer[startIdx]) return false;
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void MkAString::Resize(mkuint32 charCnt)
{
    if (charCnt > GetAllocatedByteCount() - 2 * sizeof(mkuint32) - 1)
    {
        mkuint32 allocCnt = RoundUp8((charCnt + 1) * sizeof(mkchar8));
        m_pBuffer = new mkchar8[allocCnt];

        SetAllocatedByteCount(allocCnt);
    }

    SetCharacterCount(charCnt);
    *(m_pBuffer + charCnt) = mkchar8(0);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// MkConstAString
////////////////////////////////////////////////////////////////////////////////////////////////////

const bool MkConstAString::operator<(const MkConstAString& rhs) const
{
    auto pString = GetCString();
    auto pRHSString = rhs.GetCString();

    mkuint32 minCharCnt = MkMin(GetCharacterCount(), rhs.GetCharacterCount());
    for (mkuint32 charIdx = 0; charIdx < minCharCnt; ++charIdx, ++pString, ++pRHSString)
    {
        if (*pString < *pRHSString) return true;
    }

    if (GetCharacterCount() < rhs.GetCharacterCount()) return true;

    return false;
}

#if 0
////////////////////////////////////////////////////////////////////////////////////////////////////
// MkUString
////////////////////////////////////////////////////////////////////////////////////////////////////

MkUString::MkUString()
{

}

////////////////////////////////////////////////////////////////////////////////////////////////////

MkUString::MkUString(const mkchar8* pString)
{

}

////////////////////////////////////////////////////////////////////////////////////////////////////

MkUString::MkUString(const std::string string)
{

}
#endif