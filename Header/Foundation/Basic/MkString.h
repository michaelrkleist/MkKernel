////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2025 by Michael R. Kleist
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

// A set of character string classes for ASCII, Windows wide, and UTF-8 characters.
// The classes are designed to be compile time compatible.  See sample type
// definitions below.  Nonetheless, the different character string types can
// be mixed and matched to the application needs.
// NOTE: In the Mk classes the MkString/mkchar types are used for general strings.
// The MkAString/mkachar types are used for all OS calls such as file names, IP
// addresses, and so forth.  MRK.
// NOTE: In the Mk classes the MkConstAString type is used for literal strings
// to avoid unnecessary memory allocation.  MRK.

// Mk includes.
#include "MkFoundation.h"

// C++ includes.
#include <format>

// STL includes.
#include <array>

// Forward declarations.
class MkConstAString;

namespace
{
// Helper method for rounding up an 8 bit character count for memory allocation
// purposes.
static mkuint32 RoundUp8(const mkuint32 count)
{ return ((count + 7) >> 3) << 3; }
}

// Fixed width character type definitions.
// NOTE: The 'a' represents ASCII characters, the 'w' represents Windows
// wide characters.  MRK.
typedef mkchar8 mkachar;
typedef mkchar16 mkwchar;

// Variable width UTF-8 character definition.
// The class semantics are the 1 to 4 bytes that make up a UTF-8 character are
// placed in the array, left justified, padded as needed with 0s.
// NOTE: The 'u' represents UTF-8 characters.  MRK.
class mkuchar
{
public:
    typedef std::array<mkchar8, (size_t)4> MkUBytes;

    mkuchar() :
        m_bytes()
    { }
    ~mkuchar() = default;

    const mkuchar& operator=(const mkuchar& rhs)
    { m_bytes = rhs.m_bytes; }
    const mkuchar& operator=(const mkchar8 character)
    { m_bytes = MkUBytes{character, 0, 0, 0}; }

    bool operator==(const mkuchar& rhs) const
    { return m_bytes == rhs.m_bytes; }
    bool operator!=(const mkuchar& rhs) const
    { return m_bytes == rhs.m_bytes; }

private:
    friend class MkUString;

    MkUBytes m_bytes{0, 0, 0, 0};

    // Get the number of active bytes in the character.
    mkuint32 GetByteCount() const;
    static mkuint32 GetByteCount(mkchar8 leadByte);
};


////////////////////////////////////////////////////////////////////////////////////////////////////
// MkAString
////////////////////////////////////////////////////////////////////////////////////////////////////

// A general purpose ASCII string class.
// NOTE: Why not std::string you ask?  Because I cannot add methods to it.  MRK.
class MK_EXPORT_FOUNDATION MkAString
{
public:
    MkAString();
    MkAString(const mkchar8* pString);
    MkAString(const std::string string);

    // A substring constructor.
    MkAString(const MkAString& string, const mkuint32 startIdx, const mkuint32 charCnt);

    // A formatting constructor.
    template<typename... Args>
    MkAString(const char* formatStr, const Args& ... args)
    {
        char formattedStr[128];
        mkuint32 charCnt = sprintf_s(formattedStr, sizeof(formattedStr), formatStr, args...);

        if (charCnt > 0)
        {
            // The formatting was successful.
            Construct(RoundUp8((charCnt + 1) * sizeof(mkchar8)), charCnt, formattedStr);
        }
        else
        {
            Construct(RoundUp8(sizeof(mkchar8)), 0, &m_sTerminator);
        }
    }

    ~MkAString()
    { delete m_pBuffer; }

    MkAString(const MkAString& other);
    MkAString(const MkConstAString& other);

    const MkAString& operator=(const MkAString& rhs);
    const MkAString& operator=(const MkConstAString& rhs);

    const MkAString& operator+=(const MkAString& rhs);
    const MkAString& operator+=(const MkConstAString& rhs);
    const MkAString& operator+=(const mkachar character);

    MkAString operator+(const MkAString& rhs) const;
    MkAString operator+(const MkConstAString& rhs) const;
    MkAString operator+(const mkachar character) const;

    const bool operator==(const MkAString& rhs) const;
    const bool operator<(const MkAString& rhs) const;

    mkachar operator[](const mkuint charIdx) const
    { return GetCString()[charIdx]; }

    // Get basic information.
    bool Empty() const
    { return bool(m_characterCnt == 0); }

    mkuint32 GetCharacterCount() const
    { return m_characterCnt; }

    mkuint32 GetCharacterByteCount() const
    { return GetCharacterCount(); }

    const mkchar8* GetCString() const
    { return m_pBuffer; }

    const mkachar* GetAString() const
    { return GetCString(); }

    // Make a Windows wide string from the ASCII string.
    const std::unique_ptr<mkchar16[]> GetWString() const;

    // Find a character in the string.
    mkint32 Find(const mkachar character, const mkuint32 startIdx = 0) const;

    // Simple string conversion.
    // NOTE: Fast with no error checking.  Extraneous characters are ignored.  MRK.
    mkint64 ToMkint64() const;

    mkint ToMkint() const
    { return static_cast<mkint>(ToMkint64()); }

    mkint32 ToMkint32() const
    { return static_cast<mkint32>(ToMkint64()); }

    mkuint64 ToMkuint64() const;

    mkuint ToMkuint() const
    { return static_cast<mkuint>(ToMkuint64()); }

    mkuint16 ToMkuint16() const
    { return static_cast<mkuint16>(ToMkuint64()); }

    mkuint32 ToMkuint32() const
    { return static_cast<mkuint32>(ToMkuint64()); }

    mkfloat64 ToMkfloat64() const;

    mkfloat ToMkfloat() const
    { return static_cast<mkfloat>(ToMkfloat64()); }

    mkfloat32 ToMkfloat32() const
    { return static_cast<mkfloat32>(ToMkfloat64()); }

    bool Match(const MkAString& matchStr, mkuint startIdx) const;

private:
    mkuint32 m_allocatedCnt = 0;
    mkuint32 m_characterCnt = 0;
    mkchar8* m_pBuffer = nullptr;

    static mkchar8 m_sTerminator;

    // NOTE: A minor optimization.  MRK.
    MkAString(const mkchar8* pString, const mkuint32 charCnt);

    friend class MkIBinaryStream;
    void Resize(mkuint32 charCnt);

    void SetAllocatedByteCount(const mkuint32 allocCnt)
    { m_allocatedCnt = allocCnt; }

    mkuint32 GetAllocatedByteCount() const
    { return m_allocatedCnt; }

    void SetCharacterCount(const mkuint32 charCnt)
    { m_characterCnt = charCnt; }

    void SetCString(const mkchar8* pString, const mkuint32 charCnt)
    { memcpy(m_pBuffer, pString, charCnt + 1); }

    void AddCharacter(const mkchar8 character)
    {
        *(m_pBuffer + GetCharacterCount()) = character;
        SetCharacterCount(GetCharacterCount() + 1);
        *(m_pBuffer + GetCharacterCount()) = 0;
    }

    void AddCString(const mkchar8* pString, const mkuint32 charCnt)
    {
        memcpy(m_pBuffer + GetCharacterCount(), pString, charCnt + 1);
        SetCharacterCount(GetCharacterCount() + charCnt);
    }

    mkuint32 GetCharacterCapacity() const
    { return GetAllocatedByteCount() - sizeof(mkchar8); }

    void Construct(const mkuint32 allocCnt, const mkuint32 charCnt, const mkchar8* pString)
    {
        m_pBuffer = new mkchar8[allocCnt];
        SetAllocatedByteCount(allocCnt);
        SetCharacterCount(charCnt);
        SetCString(pString, charCnt);
    }
};


////////////////////////////////////////////////////////////////////////////////////////////////////
// MkConstAString
////////////////////////////////////////////////////////////////////////////////////////////////////

// A specific purpose constant ASCII string class.  It exists so static literal strings
// can be used without allocating memory.
class MK_EXPORT_FOUNDATION MkConstAString
{
public:
    MkConstAString() = default;
    MkConstAString(const mkchar8* pString) :
        m_pBuffer(pString)
    { }

    ~MkConstAString() = default;

    MkConstAString(const MkConstAString& other) :
        m_pBuffer(other.m_pBuffer)
    { }

    const bool operator<(const MkConstAString& rhs) const;

    mkint32 Find(const mkchar8 character) const
    {
        mkuint32 charCnt = GetCharacterCount();
        for (mkuint32 charIdx = 0; charIdx < charCnt; ++charIdx)
        {
            if (m_pBuffer[charIdx] == character) return charIdx;
        }
        return -1;
    }
   
    const mkchar8* GetCString() const
    { return m_pBuffer; }

    mkuint32 GetCharacterCount() const
    { return (mkuint32)strlen(m_pBuffer); }

    mkuint32 GetByteCount() const
    { return GetCharacterCount(); }

private:
    const mkchar8* m_pBuffer;
};

// NOTE: Special case lexical sugar.  MRK.
typedef MkConstAString MkCAS;


#if 0
////////////////////////////////////////////////////////////////////////////////////////////////////
// MkUString
////////////////////////////////////////////////////////////////////////////////////////////////////

// A general purpose UTF-8 string class.
class MK_EXPORT_FOUNDATION MkUString
{
public:
    MkUString();
    MkUString(const mkchar8* pString);
    MkUString(const std::string string);

#if 0
    // A substring constructor.
    MkUString(const MkUString& string, const mkuint32 startIdx, const mkuint32 charCnt);

    // A formatting constructor.
    template<typename... Args>
    MkUString(const char* formatStr, const Args& ... args)
    {
        char formattedStr[128];
        mkuint32 charCnt = sprintf_s(formattedStr, sizeof(formattedStr), formatStr, args...);

        if (charCnt > 0)
        {
            // The formatting was successful.
            Construct(RoundUp8((charCnt + 1) * sizeof(mkchar8)), charCnt, formattedStr);
        }
        else
        {
            Construct(RoundUp8(sizeof(mkchar8)), 0, &m_sTerminator);
        }
    }

    ~MkUString()
    { delete m_pBuffer; delete m_pIndices; }

    MkUString(const MkUString& other);
    MkUString(const MkConstAString& other);

    const MkUString& operator=(const MkUString& rhs);
    const MkUString& operator=(const MkConstAString& rhs);

    const MkUString& operator+=(const mkchar8 character);
    const MkUString& operator+=(const mkuchar character);

    const MkUString& operator+=(const MkUString& rhs);
    const MkUString& operator+=(const MkConstAString& rhs);

    MkUString operator+(const MkUString& rhs) const;
    MkUString operator+(const MkConstAString& rhs) const;

    const bool operator==(const MkUString& rhs) const;
    const bool operator<(const MkUString& rhs) const;

    mkuchar operator[](const mkuint charIdx) const;

    // Get basic information.
    bool Empty() const
    { return bool(m_characterCnt == 0); }

    mkuint32 GetCharacterCount() const
    { return m_characterCnt; }

    mkuint32 GetCharacterByteCount() const
    { return m_byteCnt; }

    const mkchar8* GetCString() const
    { return m_pBuffer; }

    const std::unique_ptr<mkachar[]> GetAString() const;

    // Make a Windows wide string from the ASCII string.
    const std::unique_ptr<mkwchar[]> GetWString() const;

    // Find a character in the string.
    mkint32 Find(const mkuchar character, const mkuint32 startIdx = 0) const;

    // Simple string conversion.
    // NOTE: Fast with no error checking.  Extraneous characters are ignored.  MRK.
    mkint64 ToMkint64() const;

    mkint ToMkint() const
    { return static_cast<mkint>(ToMkint64()); }

    mkint32 ToMkint32() const
    { return static_cast<mkint32>(ToMkint64()); }

    mkuint64 ToMkuint64() const;

    mkuint ToMkuint() const
    { return static_cast<mkuint>(ToMkuint64()); }

    mkuint16 ToMkuint16() const
    { return static_cast<mkuint16>(ToMkuint64()); }

    mkuint32 ToMkuint32() const
    { return static_cast<mkuint32>(ToMkuint64()); }

    mkfloat64 ToMkfloat64() const;

    mkfloat ToMkfloat() const
    { return static_cast<mkfloat>(ToMkfloat64()); }

    mkfloat32 ToMkfloat32() const
    { return static_cast<mkfloat32>(ToMkfloat64()); }

    bool Match(const MkAString& matchStr, mkuint startIdx) const;

#endif
private:
    mkuint32 m_allocatedCnt = 0;
    mkuint32 m_characterCnt = 0;
    mkuint32 m_byteCnt = 0;
    mkchar8* m_pBuffer = nullptr;
    mkuint32* m_pIndices = nullptr;

    static mkchar8 m_sTerminator;
#if 0

    // NOTE: A minor optimization.  MRK.
    MkUString(const mkchar8* pString, const mkuint32 charCnt);

    friend class MkIBinaryStream;
    void Resize(mkuint32 charCnt);
#endif

    void SetAllocatedByteCount(const mkuint32 allocCnt)
    { m_allocatedCnt = allocCnt; }

    mkuint32 GetAllocatedByteCount() const
    { return m_allocatedCnt; }

    void SetCharacterCount(const mkuint32 charCnt)
    { m_characterCnt = charCnt; }

    void SetCString(const mkchar8* pString, const mkuint32 charCnt)
    { memcpy(m_pBuffer, pString, charCnt + 1); }

#if 0
    void AddCharacter(const mkchar8 character)
    {
        *(m_pBuffer + GetCharacterCount()) = character;
        SetCharacterCount(GetCharacterCount() + 1);
        *(m_pBuffer + GetCharacterCount()) = 0;
    }

    void AddCString(const mkchar8* pString, const mkuint32 charCnt)
    {
        memcpy(m_pBuffer + GetCharacterCount(), pString, charCnt + 1);
        SetCharacterCount(GetCharacterCount() + charCnt);
    }

    mkuint32 GetCharacterCapacity() const
    { return GetAllocatedByteCount() - sizeof(mkchar8); }
#endif

    void Construct(const mkuint32 allocCnt, const mkuint32 charCnt, const mkchar8* pString)
    {
        m_pBuffer = new mkchar8[allocCnt];
        m_pIndices = new mkuint32[allocCnt];
        SetAllocatedByteCount(allocCnt);
        SetCharacterCount(charCnt);
        SetCString(pString, charCnt);
    }
};
#endif


////////////////////////////////////////////////////////////////////////////////////////////////////
// MkString Type Definitions
////////////////////////////////////////////////////////////////////////////////////////////////////

// For ASCII strings
#if 1
typedef MkAString MkString;
typedef mkachar mkchar;
#endif

// For Windows wide strings
#if 0
typedef MkWString MkString;
typedef mkwchar mkchar;
#endif

// For UTF-8 strings
#if 0
typedef MkUString MkString;
typedef mkuchar mkchar;
#endif