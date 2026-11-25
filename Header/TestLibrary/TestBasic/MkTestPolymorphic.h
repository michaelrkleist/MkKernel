////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2025 by Michael R. Kleist
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

// Mk includes.
#include "MkTestLibrary.h"
#include "Basic\MkPolymorphicClass.h"
#include "Basic\MkIBinarySerializable.h"


////////////////////////////////////////////////////////////////////////////////////////////////////
// MkTestPolymorphic
////////////////////////////////////////////////////////////////////////////////////////////////////

class MK_EXPORT_TESTLIBRARY MkTestPolymorphic : public MkPolymorphicClass
{
public:
    MkTestPolymorphic() = default;
    ~MkTestPolymorphic() override = default;

    static void* Create()
    { return new MkTestPolymorphic; }

    const MkAString& GetName() override;

    // The binary serialization interface.
    virtual void Serialize(MkIBinaryStream& binaryStream) const;
    virtual void Deserialize(MkIBinaryStream& binaryStream);

    void SetValue(const mkuint32 value)
    { m_value = value; }

private:
    mkuint32 m_value;
};


////////////////////////////////////////////////////////////////////////////////////////////////////
// MkTestPolymorphic2
////////////////////////////////////////////////////////////////////////////////////////////////////

class MK_EXPORT_TESTLIBRARY MkTestPolymorphic2 : public MkTestPolymorphic
{
public:
    MkTestPolymorphic2() :
        m_string("Test2")
    { }
    ~MkTestPolymorphic2() override = default;

    static void* Create()
    { return new MkTestPolymorphic2; }

    const MkAString& GetName() override;

    // The binary serialization interface.
    void Serialize(MkIBinaryStream& binaryStream) const override;
    void Deserialize(MkIBinaryStream& binaryStream) override;

    void SetString(const MkAString& string)
    { m_string = string; }

private:
    MkAString m_string;
};