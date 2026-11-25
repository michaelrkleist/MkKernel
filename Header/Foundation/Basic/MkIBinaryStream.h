////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2025 by Michael R. Kleist
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

// Mk includes.
#include "MkFoundation.h"
#include "Basic\MkPolymorphicClass.h"
#include "Basic\MkString.h"

// C++ includes.
#include <optional>

// Forward declarations.
class MkIBinaryStream;


////////////////////////////////////////////////////////////////////////////////////////////////////
// HasSerializeMethod
////////////////////////////////////////////////////////////////////////////////////////////////////

template<typename T>
class HasSerializeMethod
{
    template<typename>
    static std::false_type Test(...);

    template<typename U>
    static auto Test(int)
            -> decltype(std::declval<U>().Serialize(std::declval<MkIBinaryStream&>()), std::true_type());

public:
    static constexpr bool value = std::is_same<decltype(Test<T>(0)), std::true_type>::value;
};


////////////////////////////////////////////////////////////////////////////////////////////////////
// HasDeserializeMethod
////////////////////////////////////////////////////////////////////////////////////////////////////

template<typename T>
class HasDeserializeMethod
{
    template<typename>
    static std::false_type Test(...);

    template<typename U>
    static auto Test(int)
            -> decltype(std::declval<U>().Deserialize(std::declval<MkIBinaryStream&>()), std::true_type());

public:
    static constexpr bool value = std::is_same<decltype(Test<T>(0)), std::true_type>::value;
};


////////////////////////////////////////////////////////////////////////////////////////////////////
// MkIBinaryStream
////////////////////////////////////////////////////////////////////////////////////////////////////

class MK_EXPORT_FOUNDATION MkIBinaryStream
{
public:
    MkIBinaryStream() = default;
    virtual ~MkIBinaryStream() = default;

    // Write interfaces.

    // Write a contiguous in memory object.
    template<class T>
    void WriteObject(const T& object)
    { Serialize((const mkbyte*)&object, (mkuint64)sizeof(object)); }

    // Specialize for writing bool to guarantee the size.
    template<>
    void WriteObject(const bool& object)
    {
        mkbool truth = object;
        Serialize((const mkbyte*)&truth, (mkuint64)sizeof(truth));
    }

    template<>
    void WriteObject(const volatile bool& object)
    {
        mkbool truth = object;
        Serialize((const mkbyte*)&truth, (mkuint64)sizeof(truth));
    }

    // Specialize for writing MkAString.
    template<>
    void WriteObject(const MkAString& string)
    {   
        mkuint32 charCnt = string.GetCharacterCount();
        WriteObject(charCnt);

        if (charCnt > 0)
        {
            Serialize((const mkbyte*)string.GetCString(), charCnt);
            AlignWrite();
        }
    }
    
    template<class T>
    void WriteEnum(const T& enumeration)
    {
        mkint32 enumerationi32 = (mkint32)enumeration;
        WriteObject(enumerationi32);
    }

    // Write a variable number of WriteObject-able objects.
    template<class T, template<typename...> class U, class... Other>
    void WriteObjects(const typename U<T, Other...>& objects)
    {
        mkuint32 objectCnt = static_cast<mkuint32>(objects.size());
        WriteObject(objectCnt);

        for (auto& object : objects)
        {
            WriteObject(object);
        }
    }

    // Write a vector of contiguous in memory objects.
    template<class T>
    void WriteObjects(const std::vector<T>& objects)
    {
        mkuint32 objectCnt = static_cast<mkuint32>(objects.size());
        WriteObject(objectCnt);

        const mkbyte* pObjects = (const mkbyte*)&objects[0];
        mkuint64 objectsSize = objectCnt * sizeof(T);
        Serialize(pObjects, objectsSize);
    }

    // Write a vector of MkAStrings.
    void WriteObjects(const std::vector<MkAString>& strings)
    {
        mkuint32 stringCnt = static_cast<mkuint32>(strings.size());
        WriteObject(stringCnt);

        for (auto& stringIter : strings)
        {
            WriteObject(stringIter);
        }
    }

    // Write an optional WriteObject-able object.
    template<class T>
    void WriteOptionalValue(const std::optional<T>& optValue)
    {
        mkbool hasValue = optValue.has_value();
        WriteObject(hasValue);

        if (hasValue) WriteObject<T>(optValue.value());
    }

    // Write an optional enumeration object.
    template<class T>
    void WriteOptionalEnum(const std::optional<T>& optEnum)
    {
        mkbool hasValue = optEnum.has_value();
        WriteObject(hasValue);

        if (hasValue) WriteEnum<T>(optEnum.value());
    }

    // Write a shared pointer.  If the class is derived from the polymorphic base
    // class write its class name so the proper class can be constructed during reading.
    template<class T>
    void WriteSharedPtr(const std::shared_ptr<T>& rObject)
    {
        mkbool exists = rObject != nullptr;
        WriteObject(exists);

        if (exists == 0) return;

        if (std::is_base_of<MkPolymorphicClass, T>::value)
        {
            WriteObject(rObject->GetName());
        }

        rObject->Serialize(*this);
    }

    template<typename... Args>
    void SerializeObjects(const Args&... args)
    { (Serialize(args), ...); }

    // Generic container serialization support.
    template<class T, template<typename...> class U, class... Other>
    void SerializeObjects(const typename U<T, Other...>& objects)
    {
        mkuint32 objectCnt = static_cast<mkuint32>(objects.size());
        WriteObject(objectCnt);

        for (auto& object : objects)
        {
            Serialize(object);
        }
    }

    // Read interfaces.

    // Read a contiguous in memory object.
    template<class T>
    void ReadObject(T& object)
    { Deserialize((mkbyte*)&object, (mkuint64)sizeof(object)); }

    // Specialize read object for bool to guarantee the size.
    template<>
    void ReadObject(bool& object)
    {
        mkbool truth;
        Deserialize((mkbyte*)&truth, (mkuint64)sizeof(truth));
        object = truth;
    }

    template<>
    void ReadObject(volatile bool& object)
    {
        mkbool truth;
        Deserialize((mkbyte*)&truth, (mkuint64)sizeof(truth));
        object = truth;
    }

    // Specialize read object for MkAString.
    template<>
    void ReadObject(MkAString& string)
    {
        mkuint32 charCnt;
        ReadObject(charCnt);

        if (charCnt > 0)
        {
            string.Resize(charCnt);
            Deserialize((mkbyte*)string.GetCString(), charCnt * sizeof(mkchar8));
            AlignRead();
        }
        else
        {
            string = MkAString();
        }
    }

    template<>
    void ReadObject(const MkAString& string)
    { ReadObject(const_cast<MkAString&>(string)); }
    
    // NOTE: This method is an odd duck used to make code using deserialized strings
    // as intermediary data read better.  MRK.
    MkAString ReadString()
    {
        MkAString string;

        mkuint32 charCnt;
        ReadObject(charCnt);

        if (charCnt > 0)
        {
            string.Resize(charCnt);
            Deserialize((mkbyte*)string.GetCString(), charCnt * sizeof(mkchar8));
            AlignRead();
        }

        return string;
    }

    template<class T>
    void ReadEnum(T& enumeration)
    {
        mkint32 enumerationi32;
        Deserialize((mkbyte*)&enumerationi32, (mkuint64)sizeof(enumerationi32));
        enumeration = (T)enumerationi32;
    }

    // Read a variable number of ReadObject-able objects.
    template<class T, template<typename...> class U, class... Other>
    void ReadObjects(typename U<T, Other...>& objects)
    {
        mkuint32 objectCnt = 0;
        ReadObject(objectCnt);

        objects.clear();
        for (mkuint32 objectIdx = 0; objectIdx < objectCnt; ++objectIdx)
        {
            T object;
            ReadObject(object);
            objects.insert(object);
        }
    }

    // Read a vector of contiguous in memory objects.
    template<class T>
    void ReadObjects(std::vector<T>& objects)
    {
        mkuint32 objectCnt = 0;
        ReadObject(objectCnt);

        objects.resize(objectCnt);
        mkbyte* pObjects = (mkbyte*)&objects[0];
        mkuint64 objectsSize = objectCnt * sizeof(T);
        Deserialize(pObjects, objectsSize);
    }

    // Read a vector of MKAStrings.
    void ReadObjects(std::vector<MkAString>& strings)
    {
        mkuint32 stringCnt = 0;
        ReadObject(stringCnt);

        strings.clear();
        for (mkuint32 stringIdx = 0; stringIdx < stringCnt; ++stringIdx)
        {
            strings.push_back(ReadString());
        }
    }

    // Read an optional ReadObject-able object.
    template<class T>
    void ReadOptionalValue(const std::optional<T>& optValue)
    { ReadOptionalValue(const_cast<std::optional<T>&>(optValue)); }

    template<class T>
    void ReadOptionalValue(std::optional<T>& optValue)
    {
        mkbool exists;
        ReadObject(exists);

        if (exists > 0)
        {
            T value;
            ReadObject<T>(value);
            optValue.Set(value);
        }
        else
        {
            optValue.MakeNull();
        }
    }

    // Read an optional enumeration.
    template<class T>
    void ReadOptionalEnum(std::optional<T>& optEnum)
    {
        mkbool exists;
        ReadObject(exists);

        if (exists > 0)
        {
            T enumValue;
            ReadEnum<T>(enumValue);
            optEnum.Set(enumValue);
        }
        else
        {
            optEnum.MakeNull();
        }
    }

    // Read a possibly polymorphic shared pointer object.
    template<class T>
    std::shared_ptr<T> ReadSharedPtr()
    {
        mkbool exists;
        ReadObject(exists);

        if (exists == 0) return nullptr;

        // If the class is derived from MkPolymorphicClass, construct the proper class.
        T* pObject;
        if (std::is_base_of<MkPolymorphicClass, T>::value)
        {
            pObject = (T*)MkPolymorphicClass::CreateInstance(ReadString());
        }
        else
        {
            pObject = new T();
        }

        std::shared_ptr<T> rObject(pObject);
        rObject->Deserialize(*this);

        return rObject;
    }

    // Specialize for shared pointer of MkAString.
    template<>
    std::shared_ptr<MkAString> ReadSharedPtr()
    {
        mkbool exists;
        ReadObject(exists);

        if (exists != 0) return std::make_shared<MkAString>(ReadString());

        return nullptr;
    }

    template<class T>
    void ReadObject(std::optional<T>& optValue)
    { ReadOptionalValue(optValue); }

    template<class T>
    void ReadObject(const std::optional<T>& optValue)
    { ReadOptionalValue(const_cast<std::optional<T>&>(optValue)); }

    template<typename... Args>
    void DeserializeObjects(Args&... args)
    { (Deserialize(args), ...); }

    // Generic containers deserialization support.
    template<class T, template<typename...> class U, class... Other>
    void DeserializeObjects(typename U<T, Other...>& container)
    {
        mkuint32 objectCnt = 0;
        ReadObject(objectCnt);

        container.clear();
        ContainerReserve(container, objectCnt);
        for (mkuint32 objectIdx = 0; objectIdx < objectCnt; ++objectIdx)
        {
            T object;
            Deserialize(object);
            ContainerInsert(container, object);
        }
    }

    template<class K, class T>
    void DeserializeObjects(std::map<K, T>& map)
    {
        mkuint32 objectCnt = 0;
        ReadObject(objectCnt);

        map.clear();
        for (mkuint32 objectIdx = 0; objectIdx < objectCnt; ++objectIdx)
        {
            std::pair<K, T> object;
            Deserialize(object);
            ContainerInsert(map, object);
        }
    }

    virtual void GenerateHashCode() = 0;
    virtual bool CheckHashCode() = 0;

public: 
    template<typename T>
    void Serialize(const std::vector<T>& vector)
    { SerializeObjects(vector); }

    template<typename T, typename U>
    void Serialize(const std::map<T, U>& map)
    { SerializeObjects(map); }

    template<class T>
    void Serialize(const T& object)
    { 
        if constexpr (HasSerializeMethod<T>::value)
        {
            object.Serialize(*this);
        }
        else if constexpr (std::is_enum<T>::value)
        {
            WriteEnum(object);
        }
        else
        {
            WriteObject(object);
        }
    }

    template<class T>
    void Serialize(const std::shared_ptr<T>& rObject)
    { WriteSharedPtr(rObject); }

    template<class T>
    void Serialize(const std::optional<T>& object)
    { WriteOptionalValue(object); }

    template<typename T, typename U>
    void Serialize(const std::pair<T, U>& pair)
    {
        Serialize(pair.first);
        Serialize(pair.second);
    }

    // Shared pointer serialization support.
    template<class T>
    void SerializeSharedPtr(const std::shared_ptr<T>& rObject)
    { rObject->Serialize(*this); }

    template<typename... Args>
    void Serialize(const std::tuple<Args...>& objects)
    {
        std::apply(
                [&](Args const&... object)
                { (Serialize(object), ...); },
                objects);
    }

    template<class T>
    void SerializeByValue(const std::shared_ptr<T>& rObject)
    { 
        const bool exists = rObject != nullptr;
        WriteObject(exists);
        if (exists)
        {
            rObject->Serialize(*this);
        }
    }

    template<typename T>
    void Deserialize(std::vector<T>& vector)
    { DeserializeObjects(vector); }

    template<typename T, typename U>
    void Deserialize(std::map<T, U>& map)
    { DeserializeObjects(map); }

    template<class T>
    void Deserialize(T& object)
    {
        if constexpr (HasDeserializeMethod<T>::value)
        {
            object.Deserialize(*this);
        }
        else if constexpr (std::is_enum<T>::value)
        {
            ReadEnum(object);
        }
        else
        {
            ReadObject(object);
        }
    }

    template<class T>
    void Deserialize(std::shared_ptr<T>& rObject)
    { rObject = ReadSharedPtr<T>(); }

    template<class K, class T>
    void Deserialize(std::shared_ptr<std::map<K,T>>& rMap)
    {
        mkbool exists;
        ReadObject(exists);
        if (exists == 0)
        {
            rMap = nullptr;
            return;
        }
        rMap.reset(new std::map<K, T>());
        DeserializeObjects(*rMap);
    }

    template<class T>
    void Deserialize(const std::optional<T>& object)
    { ReadOptionalValue(object); }

    template<typename T, typename U>
    void Deserialize(std::pair<T, U>& pair)
    {
        Deserialize(pair.first);
        Deserialize(pair.second);
    }

    template<typename... Args>
    void Deserialize(const std::tuple<Args...>& objects)
    {
        std::apply(
                [&](Args const&... object)
                { (Deserialize(object), ...); },
                objects);
    }

    template<class T>
    void DeserializeByValue(std::shared_ptr<T>& rObject)
    { 
        bool exists;
        ReadObject(exists);
        if (exists)
        {
            rObject->Deserialize(*this);
        }
    }

    template<typename T>
    void ContainerReserve(T& container, const mkuint64 size)
    { }

    template<typename T>
    void ContainerReserve(std::vector<T>& container, const mkuint64 size)
    { container.reserve(size); }

    template<typename U, typename T>
    void ContainerInsert(U& container, const T& value)
    { container.insert(value); }

    template<typename T>
    void ContainerInsert(std::vector<T>& container, const T& value)
    { container.push_back(value); }

    virtual void Serialize(const mkbyte* pObject, const mkuint64 objectSize) = 0;
    virtual void Deserialize(mkbyte* pObject, const mkuint64 objectSize) = 0;

    virtual void AlignWrite() = 0;
    virtual void AlignRead() = 0;
};