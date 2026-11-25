////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2025 by Michael R. Kleist
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

// Mk includes.
#include "MkFoundation.h"
#include "Basic\MkDuration.h"
#include "Basic\MkString.h"
#include "Process\MKOSEvent.h"

#define DEBUG_SOCKET 0

// STL includes.
#include <array>

// Windows includes.
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

// Forward declarations.
class MkMemoryVessel;

// NOTE: This set of classes is NOT a general purpose socket library although in 
// principle it could be.  It is currently designed to support non-blocking I/O for
// client and server sockets.  MRK.


////////////////////////////////////////////////////////////////////////////////////////////////////
// MkIP4SocketAddress
////////////////////////////////////////////////////////////////////////////////////////////////////

class MkIP4SocketAddress
{
public:
    MkIP4SocketAddress(const MkAString& ip4AddressStr, const MkAString& portStr);
    MkIP4SocketAddress(const MkAString& portStr);
    ~MkIP4SocketAddress() = default;

    const SOCKADDR* GetSocketAddress() const
    { return &m_socketAddress; }

    int GetSocketAddressByteCnt() const
    { return (int)sizeof(m_socketAddress); }

    int GetFamily() const
    { return AF_INET; }

    int GetType() const
    { return SOCK_STREAM; }

    int GetProtocol() const
    { return IPPROTO_TCP; }

    bool IsValidAddress() const
    { return m_valid; }

    mkint32 GetLastError() const
    { return m_lastError; }

private:
    SOCKADDR m_socketAddress;
    bool m_valid;
    mkint32 m_lastError;
};


////////////////////////////////////////////////////////////////////////////////////////////////////
// MkIP4PortString
////////////////////////////////////////////////////////////////////////////////////////////////////

class MK_EXPORT_FOUNDATION MkIP4PortString
{
public:
    MkIP4PortString(const MkAString& ip4PortStr);
    MkIP4PortString(const MkAString& ip4AddressStr, const MkAString& portStr);
    ~MkIP4PortString() = default;

    const MkAString& GetIP4AddressString() const
    { return m_ip4AddressStr; }

    const MkAString& GetPortString() const
    { return m_portStr; }

    const MkAString& GetIP4PortString() const
    { return m_ip4PortStr; }

private:
    // NOTE: No validation is done.  MRK.
    MkAString m_ip4PortStr;         // IP4 address and port separated by a ':'.
    MkAString m_ip4AddressStr;      // IP4 address can be a system name or a four integer IP address.
    MkAString m_portStr;
};


////////////////////////////////////////////////////////////////////////////////////////////////////
// MkSocket
////////////////////////////////////////////////////////////////////////////////////////////////////

class MkSocket
{
public:
    MK_EXPORT_FOUNDATION MkSocket();
    MK_EXPORT_FOUNDATION virtual ~MkSocket() = default;

    enum SocketStatus
    {
        eBytesSent = 0,
        eBytesReceived = 0,
        eBlockingSet = 0,
        eBlockingReset = 0,
        eEventActivated = 0,
        eEventDeactivated = 0,
        eWriteBufferFull,
        eReadBufferEmpty,
        eSocketError
    };

    bool MK_EXPORT_FOUNDATION IsValid() const
    { return m_socket != INVALID_SOCKET; }

    SocketStatus SetBlocking(bool blocking);
    SocketStatus SetTimeOut(const MkDuration& timeOut);

#if DEBUG_SOCKET
    MkAString m_name;
#endif

protected:
    // NOTE: Used to make it easy to tell why a socket was closed.  MRK.
    void CloseSocket(SOCKET socket);

    SOCKET m_socket = INVALID_SOCKET;
    mkint32 m_lastError = ERROR_SUCCESS;
};


////////////////////////////////////////////////////////////////////////////////////////////////////
// MkSocketIO
////////////////////////////////////////////////////////////////////////////////////////////////////

class MkSocketIO : public MkSocket
{
public:
    MK_EXPORT_FOUNDATION MkSocketIO();
    MK_EXPORT_FOUNDATION virtual ~MkSocketIO() = default;

    enum SocketManagement : mkuint32
    {
        eSocketHeard = 75
    };

    SocketStatus MK_EXPORT_FOUNDATION ReadBytes(
            const mkbyte* pBytes, const mkuint32 requestCnt, mkuint32& receivedCnt);

    SocketStatus MK_EXPORT_FOUNDATION WriteBytes(
            const mkbyte* pBytes, const mkuint32 requestCnt, mkuint32& sentCnt);

    bool MK_EXPORT_FOUNDATION WouldReadBlock();
    bool MK_EXPORT_FOUNDATION WouldWriteBlock();

    class Event
    {
    public:
        Event() :
            m_eventBits(0)
        { }

        Event(const WSANETWORKEVENTS wsaNetworkEvents)
        { m_eventBits = (mkuint64)wsaNetworkEvents.lNetworkEvents; }

        ~Event() = default;

        bool ReadEvent() const
        { return bool(m_eventBits & (mkuint64)FD_READ); }

        bool CloseEvent() const
        { return bool(m_eventBits & (mkuint64)FD_CLOSE); }

        bool IsEmpty() const
        { return bool(m_eventBits == 0); }

        MkAString MK_EXPORT_FOUNDATION Description() const;

    private:
        mkuint64 m_eventBits;
    };

    SocketStatus MK_EXPORT_FOUNDATION ActivateEvent();
    Event MK_EXPORT_FOUNDATION IdentifyEvent() const;

    const std::shared_ptr<MkOSEvent>& GetEvent() const
    { return m_rEvent; }

protected:
    SocketStatus ShutDown(const mkint32 lastError, const SocketStatus socketStatus)
    {
        m_lastError = lastError;
        CloseSocket(m_socket);
        m_socket = INVALID_SOCKET;
        return socketStatus;
    }

    std::shared_ptr<MkOSEvent> m_rEvent;
    bool m_eventActivated = false;
};


////////////////////////////////////////////////////////////////////////////////////////////////////
// MkSocketServer
////////////////////////////////////////////////////////////////////////////////////////////////////

class MkSocketServer : public MkSocketIO, public MkIP4SocketAddress
{
public:
    MK_EXPORT_FOUNDATION MkSocketServer(const MkAString& portStr);
    MK_EXPORT_FOUNDATION MkSocketServer(SOCKET socket);
    MK_EXPORT_FOUNDATION ~MkSocketServer() override
    { CloseSocket(m_socket); }
};


////////////////////////////////////////////////////////////////////////////////////////////////////
// MkSocketClient
////////////////////////////////////////////////////////////////////////////////////////////////////

class MkSocketClient : public MkSocketIO, public MkIP4SocketAddress
{
public:
    MK_EXPORT_FOUNDATION MkSocketClient(const MkAString& ip4AddressStr, const MkAString& portStr);
    MK_EXPORT_FOUNDATION MkSocketClient(const MkIP4PortString& ip4PortStr) :
            MkSocketClient(ip4PortStr.GetIP4AddressString(), ip4PortStr.GetPortString())
    { }
    MK_EXPORT_FOUNDATION ~MkSocketClient() override = default;
};


////////////////////////////////////////////////////////////////////////////////////////////////////
// MkSocketAcceptor
////////////////////////////////////////////////////////////////////////////////////////////////////

class MkSocketAcceptor : public MkSocket, public MkIP4SocketAddress
{
public:
    MK_EXPORT_FOUNDATION MkSocketAcceptor(const MkAString& portStr);
    MK_EXPORT_FOUNDATION ~MkSocketAcceptor()
    { CloseSocket(m_socket); }

    bool MK_EXPORT_FOUNDATION IsValid() const
    { return m_socket != INVALID_SOCKET; }

    bool MK_EXPORT_FOUNDATION SetBlocking(bool blocking);

    std::shared_ptr<MkSocketServer> MK_EXPORT_FOUNDATION Accept(
            const MkDuration& timeOut = MkDuration());

    const std::shared_ptr<MkOSEvent>& GetEvent() const
    { return m_rEvent; }
    
private:
    std::shared_ptr<MkOSEvent> m_rEvent;
};


////////////////////////////////////////////////////////////////////////////////////////////////////
// MkGetHostName, MkGetHostIP4Address
////////////////////////////////////////////////////////////////////////////////////////////////////

enum MkHostIDStatus
{
    eHostIDSuccess = 0,
    eHostNameNotFound,
    eHostIP4AddressNotFound
};

MkHostIDStatus MK_EXPORT_FOUNDATION MkGetHostName(MkAString& hostName);
#if 0
MkHostIDStatus MK_EXPORT_FOUNDATION MkGetHostIP4Address(MkAString& ip4AddressStr);
#endif


////////////////////////////////////////////////////////////////////////////////////////////////////
// MkInitializeWSA
////////////////////////////////////////////////////////////////////////////////////////////////////

void MkInitializeWSA();