////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2025 by Michael R. Kleist
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

// Mk includes.
#include "Process/MkCriticalSection.h"

// Forward declarations.
class MkAString;


////////////////////////////////////////////////////////////////////////////////////////////////////
// MkSingleton
////////////////////////////////////////////////////////////////////////////////////////////////////

// NOTE: One version of a singleton.  MRK.
template<class SingletonT>
class MkSingleton
{
public:
    static SingletonT& Instance()
    {
        MkCriticalSection cs(m_sLock);

        if (m_srInstance == nullptr)
            m_srInstance = std::make_unique<InstanceT>();

        return *m_srInstance;
    }

    // NOTE: A bit of a slimy way to get arguments to a singleton.  MRK.
    static SingletonT& Instance(const MkAString& arguments)
    {
        MkCriticalSection cs(m_sLock);

        if (m_srInstance == nullptr)
            m_srInstance = std::make_unique<InstanceT>(arguments);

        return *m_srInstance;
    }

protected:
    MkSingleton() = default;
    virtual ~MkSingleton() = default;

    struct InstanceT : public SingletonT
    {
        InstanceT() :
            SingletonT()
        { }

        InstanceT(const MkAString& arguments) :
            SingletonT(arguments)
        { }
    };

    static std::unique_ptr<InstanceT> m_srInstance;
    static MkCriticalSectionLock m_sLock;

private:
    MkSingleton(const MkSingleton& singleton) = delete;
    const MkSingleton& operator=(const MkSingleton&) = delete;

public:
    std::unique_ptr<InstanceT>& GetInstance()
    { return m_srInstance; }
};

#if 1
template<typename SingletonT>
std::unique_ptr<typename MkSingleton<SingletonT>::InstanceT> MkSingleton<SingletonT>::m_srInstance;

template<typename SingletonT>
MkCriticalSectionLock MkSingleton<SingletonT>::m_sLock;
#endif

// NOTE: To make a class T a singleton, do the following.

// When declaring the class T do the following.
// 
// class T : public WxSingleton<T>
// {
//     ...
// }

// When defining the class T do the following static member initializations.
// 
// std::unique_ptr<MkSingleton<T>::InstanceT> MkSingleton<T>::m_srInstance = nullptr;
// MkCriticalSectionLock MkSingleton<T>::m_sLock;

// When referring to the class T do the following.
//
// {
//     ...
//     T::Instance().Method(...);
//     ...
// }

