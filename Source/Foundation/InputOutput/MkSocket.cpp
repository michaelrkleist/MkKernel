////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2025 by Michael R. Kleist
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "Foundation\MkFoundationPCH.h"
#include "InputOutput\MkSocket.h"

// Mk includes.
#include "Basic\MkException.h"
#include "Basic\MkMemoryVessel.h"

#define DEBUG 0
#if DEBUG
#include "InputOutput\MkDebugText.h"
#endif

// Windows includes.
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib") 

// Static initializations.
MkConstAString sHostIP4AddressStr("127.0.0.1");
MkConstAString sAnyIP4AddressStr("0.0.0.0");
MkConstAString sZeroPortStr("0");

// Static initializations.
namespace
{
bool sWSAInitialized = false;
WSADATA sWSAData;

unsigned long sBlocking = 0;
unsigned long sNonBlocking = 1;
}

#if DEBUG_SOCKET
#include "InputOutput\MkDebugText.h"
#endif


////////////////////////////////////////////////////////////////////////////////////////////////////
// MkIP4SocketAddress
////////////////////////////////////////////////////////////////////////////////////////////////////

MkIP4SocketAddress::MkIP4SocketAddress(const MkAString& ip4AddressStr, const MkAString& portStr) :
    m_valid(true),
    m_lastError(0)
{
    MkInitializeWSA();

    // NOTE: Bizarre conventional naming (hints) for this stucture.  MRK.
    ADDRINFOA addressHints;
    ZeroMemory(&addressHints, sizeof(addressHints));

    addressHints.ai_family = GetFamily();
    addressHints.ai_socktype = GetType();
    addressHints.ai_protocol = GetProtocol();

    ADDRINFOA* pAddressInfo;

    // Resolve the address.
    if (getaddrinfo(
            ip4AddressStr.GetCString(), portStr.GetCString(), &addressHints, &pAddressInfo) != 0)
    {
        m_valid = false;
        freeaddrinfo(pAddressInfo);
        m_lastError = WSAGetLastError();
        throw MkException(eOSConfiguration, "Cannot resolve IP address.");
    }

    ::memcpy(
            &m_socketAddress, 
            pAddressInfo->ai_addr,
            MkMin(sizeof(m_socketAddress), pAddressInfo->ai_addrlen));

    freeaddrinfo(pAddressInfo);

#if 0
    char ipStr[INET6_ADDRSTRLEN];
    uint16_t port = 0;

    if (m_socketAddress.sa_family == AF_INET)
    {
        const struct sockaddr_in* pIPv4 = (const struct sockaddr_in*)&m_socketAddress;

        inet_ntop(AF_INET, &(pIPv4->sin_addr), ipStr, sizeof(ipStr));
        port = ntohs(pIPv4->sin_port);

        printf("IPv4: %s:%u\n", ipStr, port);
    } 
    else if (m_socketAddress.sa_family == AF_INET6)
    {
        const struct sockaddr_in6* pIPv6 = (const struct sockaddr_in6*)&m_socketAddress;

        inet_ntop(AF_INET6, &(pIPv6->sin6_addr), ipStr, sizeof(ipStr));
        port = ntohs(pIPv6->sin6_port);

        printf("IPv6: [%s]:%u\n", ipStr, port);
    } 
    else 
    {
        printf("Unsupported address family: %d\n", m_socketAddress.sa_family);
    }
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////

MkIP4SocketAddress::MkIP4SocketAddress(const MkAString& portStr) :
    m_valid(true),
    m_lastError(0)
{
    MkInitializeWSA();

    // NOTE: Bizarre conventional naming (hints) for this stucture.  MRK.
    ADDRINFOA addressHints;
    ZeroMemory(&addressHints, sizeof(addressHints));

    addressHints.ai_family = GetFamily();
    addressHints.ai_socktype = GetType();
    addressHints.ai_protocol = GetProtocol();

    ADDRINFOA* pAddressInfo;

    // Resolve the address.
    if (getaddrinfo(
            sAnyIP4AddressStr.GetCString(), portStr.GetCString(), &addressHints, &pAddressInfo) != 0)
    {
        m_valid = false;
        freeaddrinfo(pAddressInfo);
        m_lastError = WSAGetLastError();
        return;
    }

    ::memcpy(
            &m_socketAddress, 
            pAddressInfo->ai_addr,
            MkMin(sizeof(m_socketAddress), pAddressInfo->ai_addrlen));

    freeaddrinfo(pAddressInfo);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// MkIP4PortString
////////////////////////////////////////////////////////////////////////////////////////////////////

MkIP4PortString::MkIP4PortString(const MkAString& ip4PortStr) :
    m_ip4PortStr(ip4PortStr)
{
    auto colonIdx = m_ip4PortStr.Find(':');
    if (colonIdx < 0)
    {
        m_ip4AddressStr = m_ip4PortStr;
    }
    else
    {
        m_ip4AddressStr = MkAString(m_ip4PortStr, 0, colonIdx);
        m_portStr = MkAString(m_ip4PortStr, colonIdx + 1, m_ip4PortStr.GetCharacterCount() - colonIdx - 1);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

MkIP4PortString::MkIP4PortString(const MkAString& ip4AddressStr, const MkAString& portStr) :
    m_ip4AddressStr(ip4AddressStr),
    m_portStr(portStr)
{
    m_ip4PortStr = m_ip4AddressStr + '.' + m_portStr;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// MkSocket
////////////////////////////////////////////////////////////////////////////////////////////////////

MkSocket::MkSocket()
{
    MkInitializeWSA();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

MkSocket::SocketStatus MkSocket::SetBlocking(bool blocking)
{
    if (ioctlsocket(m_socket, FIONBIO, blocking ? &sBlocking : &sNonBlocking) != NO_ERROR)
    {
        m_lastError = WSAGetLastError();
        CloseSocket(m_socket);
        m_socket = INVALID_SOCKET;
        return eSocketError;
    }

    return eBlockingSet;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

MkSocket::SocketStatus MkSocket::SetTimeOut(const MkDuration& timeOut)
{
    if (timeOut.GetDuration() <= 0)
    {
        return SetBlocking(false);
    }
    else
    {
        if (SetBlocking(true) == eSocketError) return eSocketError;

        timeval timeValue;
        timeValue.tv_sec = timeOut.GetIntegral(MkDuration::eSeconds);
        timeValue.tv_usec =
                timeOut.GetIntegral(MkDuration::eMicroseconds) - (timeValue.tv_sec * 1000000);

        auto status = setsockopt(
                m_socket, 
                SOL_SOCKET, 
                SO_RCVTIMEO,
                (const char*)&timeValue,
                sizeof(timeValue));
        if (status == SOCKET_ERROR) return eSocketError;

        return eBlockingSet;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void MkSocket::CloseSocket(SOCKET socket)
{
    closesocket(socket);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// MkSocketIO
////////////////////////////////////////////////////////////////////////////////////////////////////

MkSocketIO::MkSocketIO() :
    MkSocket(),
    m_rEvent(std::make_shared<MkOSEvent>("Socket", true))
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////

MkSocketIO::SocketStatus MkSocketIO::ReadBytes(
        const mkbyte* pBytes, const mkuint32 requestCnt, mkuint32& receivedCnt)
{ 
    int readCnt = recv(m_socket, (char*)pBytes, requestCnt, 0);

#if DEBUG_SOCKET
    { 
        MkDebugSystemText() << "Read " << readCnt << " bytes from socket " <<
                m_socket << " " << m_name.GetCString() << ".\n";
    }
#endif

    if (readCnt == SOCKET_ERROR) 
    {
        int status = WSAGetLastError();
        if (status == WSAEWOULDBLOCK)
        {
            receivedCnt = 0;
            return eReadBufferEmpty;
        }

        return ShutDown(status, eSocketError);
    }

    receivedCnt = readCnt;
    return eBytesReceived;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

MkSocketIO::SocketStatus MkSocketIO::WriteBytes(
        const mkbyte* pBytes, const mkuint32 requestCnt, mkuint32& sentCnt)
{ 
    int writeCnt = send(m_socket, (char*)pBytes, requestCnt, 0);

#if DEBUG_SOCKET
    { 
        MkDebugSystemText() << "Wrote " << writeCnt << " bytes to socket " <<
                m_socket << " " << m_name.GetCString() << ".\n";
    }
#endif

    if (writeCnt == SOCKET_ERROR) 
    {
        int status = WSAGetLastError();
        if (status == WSAEWOULDBLOCK)
        {
            sentCnt = 0;
            return eWriteBufferFull;
        }

        return ShutDown(status, eSocketError);
    }

    sentCnt = writeCnt;
    return eBytesSent;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

bool MkSocketIO::WouldReadBlock()
{
    fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET(m_socket, &readSet);

    // Set timeout to 0 so select returns immediately.
    timeval timeout{0, 0};

    // Check readability.
    auto status = select(0, &readSet, NULL, NULL, &timeout);
    if (status == SOCKET_ERROR) return ShutDown(status, eSocketError);

    return bool(status == 0);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

bool MkSocketIO::WouldWriteBlock()
{
    fd_set writeSet;
    FD_ZERO(&writeSet);
    FD_SET(m_socket, &writeSet);

    // Set timeout to 0 so select returns immediately.
    timeval timeout{0, 0};

    // Check writability.
    auto status = select(0, NULL, &writeSet, NULL, &timeout);
    if (status == SOCKET_ERROR) return ShutDown(status, eSocketError);

    return bool(status == 0);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

MkSocketIO::SocketStatus MkSocketIO::ActivateEvent()
{
    if (m_eventActivated) return eEventActivated;

#if 0
#if DEBUG_SOCKET
    { MkDebugSystemText() << "Activate event on socket " << m_socket << " " << m_name.GetCString() << ".\n"; }
#endif
#endif

    if (SetBlocking(false) != eBlockingSet) return eSocketError;

    if (WSAEventSelect(m_socket, m_rEvent->GetHandle(), FD_READ | FD_CLOSE) == SOCKET_ERROR)
    {
        m_lastError = WSAGetLastError();
        CloseSocket(m_socket);
        m_socket = INVALID_SOCKET;
        return eSocketError;
    };

    m_eventActivated = true;
    return eEventActivated;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

MkSocketIO::Event MkSocketIO::IdentifyEvent() const
{
#if 0
#if DEBUG_SOCKET
    { MkDebugSystemText() << "Identify event on socket " << m_socket << " " << m_name.GetCString() << ".\n"; }
#endif
#endif

    WSANETWORKEVENTS wsaNetworkEvents;
    if (WSAEnumNetworkEvents(m_socket, m_rEvent->GetHandle(), &wsaNetworkEvents) != 0)
    {
        throw MkException(eLogicError, "Cannot identify socket event.");
    }

    return Event(wsaNetworkEvents);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// MkSocketServer
////////////////////////////////////////////////////////////////////////////////////////////////////

MkSocketServer::MkSocketServer(const MkAString& portStr) :
    MkSocketIO(),
    MkIP4SocketAddress(portStr)
{
    if (!IsValidAddress())
    {
        MkSocketIO::m_lastError = MkIP4SocketAddress::GetLastError();
        m_socket = INVALID_SOCKET;
        return;
    }

    m_socket = socket(GetFamily(), GetType(), GetProtocol());
    if (m_socket == INVALID_SOCKET)
    {
        MkSocketIO::m_lastError = WSAGetLastError();
        return;
    }

    if (bind(m_socket, GetSocketAddress(), GetSocketAddressByteCnt()) == SOCKET_ERROR)
    {
        MkSocketIO::m_lastError = WSAGetLastError();
        CloseSocket(m_socket);
        m_socket = INVALID_SOCKET;
        return;
    }

    if (SetBlocking(false) != eBlockingSet)
    {
        MkSocketIO::m_lastError = WSAGetLastError();
        CloseSocket(m_socket);
        m_socket = INVALID_SOCKET;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

MkSocketServer::MkSocketServer(SOCKET socket) :
    MkSocketIO(),
    MkIP4SocketAddress(sZeroPortStr)
{
    m_socket = socket;

    if (SetBlocking(true) != eBlockingSet)
    {
        MkSocketIO::m_lastError = WSAGetLastError();
        CloseSocket(m_socket);
        m_socket = INVALID_SOCKET;
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// MkSocketClient
////////////////////////////////////////////////////////////////////////////////////////////////////

MkSocketClient::MkSocketClient(const MkAString& ip4AddressStr, const MkAString& portStr) :
    MkSocketIO(),
    MkIP4SocketAddress(ip4AddressStr, portStr)
{
    if (!IsValidAddress())
    {
        MkSocket::m_lastError = MkIP4SocketAddress::GetLastError();
        return;
    }

    m_socket = socket(GetFamily(), GetType(), GetProtocol());
    if (m_socket == INVALID_SOCKET)
    {
        MkSocket::m_lastError = WSAGetLastError();
        return;
    }

    // Connect to the server.
    if (connect(m_socket, GetSocketAddress(), GetSocketAddressByteCnt()) == SOCKET_ERROR)
    {
        MkSocket::m_lastError = WSAGetLastError();
        CloseSocket(m_socket);
        m_socket = INVALID_SOCKET;
        return;
    }

    if (SetBlocking(false) != eBlockingSet)
    {
        MkSocket::m_lastError = WSAGetLastError();
        CloseSocket(m_socket);
        m_socket = INVALID_SOCKET;
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// MkSocketAcceptor
////////////////////////////////////////////////////////////////////////////////////////////////////

MkSocketAcceptor::MkSocketAcceptor(const MkAString& portStr) :
    MkIP4SocketAddress(portStr),
    m_rEvent(std::make_shared<MkOSEvent>("Accept", true))
{
    if (!IsValidAddress())
    {
        MkSocket::m_lastError = MkIP4SocketAddress::GetLastError();
        m_socket = INVALID_SOCKET;
        return;
    }

    m_socket = socket(GetFamily(), GetType(), GetProtocol());
    if (m_socket == INVALID_SOCKET)
    {
        MkSocket::m_lastError = WSAGetLastError();
        return;
    }

    if (bind(m_socket, GetSocketAddress(), GetSocketAddressByteCnt()) == SOCKET_ERROR)
    {
        MkSocket::m_lastError = WSAGetLastError();
        CloseSocket(m_socket);
        m_socket = INVALID_SOCKET;
        return;
    }

    if (listen(m_socket, SOMAXCONN) == SOCKET_ERROR) 
    {
        MkSocket::m_lastError = WSAGetLastError();
        CloseSocket(m_socket);
        m_socket = INVALID_SOCKET;
        return;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

bool MkSocketAcceptor::SetBlocking(bool blocking)
{
    if (ioctlsocket(m_socket, FIONBIO, blocking ? &sBlocking : &sNonBlocking) != NO_ERROR)
    {
        MkSocket::m_lastError = WSAGetLastError();
        CloseSocket(m_socket);
        m_socket = INVALID_SOCKET;
        return false;
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

std::shared_ptr<MkSocketServer> MkSocketAcceptor::Accept(const MkDuration& timeOut)
{
    if (m_socket == INVALID_SOCKET) return nullptr;

    SetTimeOut(timeOut);

    sockaddr clientSocketAddress;
    int clientSocketAddressCnt = sizeof(clientSocketAddress);

    SOCKET serverSocket = accept(m_socket, &clientSocketAddress, &clientSocketAddressCnt);

    if (serverSocket == INVALID_SOCKET)
    {
        MkSocket::m_lastError = WSAGetLastError();
        CloseSocket(m_socket);
        m_socket = INVALID_SOCKET;
        return nullptr;
    }

    if (!SetBlocking(true))
    {
        MkSocket::m_lastError = WSAGetLastError();
        CloseSocket(m_socket);
        m_socket = INVALID_SOCKET;
        return nullptr;
    }
    
    return std::make_shared<MkSocketServer>(serverSocket);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// MkGetHostName
////////////////////////////////////////////////////////////////////////////////////////////////////

MkHostIDStatus MkGetHostName(MkAString& hostName)
{
    MkInitializeWSA();

    char hostNameCStr[256];
    auto status = gethostname(hostNameCStr, 256);
    if (status == SOCKET_ERROR) throw MkException(eOSFailure, "Cannot get host name.");

    hostName = MkAString(hostNameCStr);
    return eHostIDSuccess;
}


#if 0
////////////////////////////////////////////////////////////////////////////////////////////////////
// MkGetHostIP4Address
////////////////////////////////////////////////////////////////////////////////////////////////////

MkHostIDStatus MkGetHostIP4Address(MkAString& ip4AddressStr)
{
    // TODO: Error handling?  MRK.
    char hostNameCStr[256];
    auto status = gethostname(hostNameCStr, 256);
    if (status == SOCKET_ERROR) return eHostNameNotFound;

    ADDRINFOA* pAddressInfo;
    status = getaddrinfo(hostNameCStr, nullptr, nullptr, &pAddressInfo);
    if (status != 0) return eHostIP4AddressNotFound;

    // NOTE: What a mess Windows makes of this information.  MRK.
    char ip4CStr[256];
    ip4AddressStr = MkAString(inet_ntop(AF_INET, pAddressInfo, ip4CStr, 256));

    return eHostIDSuccess;
}
#endif


////////////////////////////////////////////////////////////////////////////////////////////////////
// MkInitializeWSA
////////////////////////////////////////////////////////////////////////////////////////////////////

void MkInitializeWSA()
{
    if (!sWSAInitialized)
    {
        int status = WSAStartup(MAKEWORD(2, 2), &sWSAData);
        if (status != 0)
        {
            throw MkException(eOSFailure, "WSA failed to start.");
        }
        sWSAInitialized = true;
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// MkSocketIO::Event
////////////////////////////////////////////////////////////////////////////////////////////////////

MkAString MkSocketIO::Event::Description() const
{
    if (m_eventBits == 0) return MkCAS("    *** No events set ***\n");

    MkAString description;

    if (bool(m_eventBits & (mkuint64)FD_READ)) description += MkCAS("  Read");
    if (bool(m_eventBits & (mkuint64)FD_WRITE)) description += MkCAS("  Write");
    if (bool(m_eventBits & (mkuint64)FD_OOB)) description += MkCAS("  OOB");
    if (bool(m_eventBits & (mkuint64)FD_ACCEPT)) description += MkCAS("  Accept");
    if (bool(m_eventBits & (mkuint64)FD_CONNECT)) description += MkCAS("  Connect");
    if (bool(m_eventBits & (mkuint64)FD_CLOSE)) description += MkCAS("  Close");
    if (bool(m_eventBits & (mkuint64)FD_QOS)) description += MkCAS("  QOS");
    if (bool(m_eventBits & (mkuint64)FD_GROUP_QOS)) description += MkCAS("  Group QOS");
    if (bool(m_eventBits & (mkuint64)FD_ROUTING_INTERFACE_CHANGE)) description += MkCAS("  Routing Change");
    if (bool(m_eventBits & (mkuint64)FD_ADDRESS_LIST_CHANGE)) description += MkCAS("  Address change");

    description += MkCAS("\n");
    return description;
}