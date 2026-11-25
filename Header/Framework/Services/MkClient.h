////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2025 by Michael R. Kleist
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once
#pragma warning(disable : 4251)

// Mk includes.
#include "MkFramework.h"
#include "Basic\MkMemoryVessel.h"
#include "Basic\MkString.h"
#include "Basic\MkUID.h"
#include "InputOutput\MkMBTP.h"

// STL includes.
#include <set>

// Forward declarations.
class MkResourceLabel;
class MkUID;

// A connect message consists of the following
//   A MBTP header.
//   A service connect code.
//   A service name.
//   A list of required resource labels.
//   An IP4 address string for the client response socket.
//   A port string for the client response socket.
//
// A service request message consists of the following
//   A MBTP header.
//   A generic request code (a code that says the message has a request).
//   A service specific request code.
//   A UID for this request.
//   The arguments for the service request.


////////////////////////////////////////////////////////////////////////////////////////////////////
// MkClient
////////////////////////////////////////////////////////////////////////////////////////////////////

class MK_EXPORT_FRAMEWORK MkClient
{
public:
    MkClient(
            const MkAString& serviceName,
            const std::vector<MkIP4PortString>& serviceAddresses,
            const std::vector<std::shared_ptr<MkResourceLabel>>& resourceLabels,
            const bool loadBalance);
    virtual ~MkClient();

    friend class AttachServiceTask;

    enum RequestStatus
    {
        eRequestSent = 0,
        eAlreadySent,
        eNoAvailableService,
        eTimeOut,
        eSocketClosed
    };

    enum ResponseStatus
    {
        eResponseReceived = 0,
        eReadFailed,
        eInvalidResponse,
    };

    bool CheckResponse(const MkUID& requestUID) const;
    std::shared_ptr<MkMemoryVessel> TakeResponse(const MkUID& requestUID);

protected:
    void FillBaseRequest(
            const std::shared_ptr<MkMemoryVessel>& rRequestMessage,
            const MkUID& requestUID,
            mkuint32 requestCode) const;

    mkuint32 GetActiveServiceCount() const
    { return m_activeServiceCnt; }
    bool GetNextServiceIndex(mkuint32& serviceIdx) const;

    friend class RetryRequestFunctor;
    bool MakeRequest(
            const std::shared_ptr<MkMemoryVessel>& rRequestMessage,
            const MkUID& requestUID,
            const std::shared_ptr<MkOSEvent>& rResponseEvent,
            const MkDuration retryDuration);
    void RemakeRequest(const MkUID& requestUID);
    void RemoveRequest(const MkUID& requestUID);

    RequestStatus SendRequest(const MkUID& requestUID);

    friend class HandleResponseTask;
    void PostResponse(
            const std::shared_ptr<MkMemoryVessel>& rResponseMessage,
            const MkUID& requestUID,
            mkuint32 serviceIdx);

private:
    friend class ConnectFunctor;
    void ConnectServices();

    void DisconnectService(mkuint32 serviceIdx);

    friend class ServiceData;
    enum ServiceStatus
    {
        eIdle = 0,
        eConnected,
        eInvalidSocketAddress,
        eNoServiceMaster,
        eNoService,
        eInsufficientResources
    };

    MkAString m_serviceName;
    std::vector<std::shared_ptr<MkResourceLabel>> m_resourceLabels;

    bool m_loadBalance;

    class ServiceData
    {
    public:
        ServiceData() = default;
        ServiceData(const MkAString& serviceHostName, const MkAString& servicePortStr) :
            m_serviceHostName(serviceHostName),
            m_servicePortStr(servicePortStr),
            m_rRequestSocket(nullptr),
            m_status(MkClient::eIdle),
            m_rMBTPReader(nullptr),
            m_pendingRequestCnt(0)
        { }
        ~ServiceData() = default;

        MkAString m_serviceHostName;
        MkAString m_servicePortStr;
        std::shared_ptr<MkSocketClient> m_rRequestSocket;
        MkClient::ServiceStatus m_status;
        std::shared_ptr<MkMBTPReader> m_rMBTPReader;
        mkuint32 m_pendingRequestCnt;   // For this service.
    };

    friend class ClientSocketEventFunctor;
    mutable std::vector<ServiceData> m_services;
    mutable MkCriticalSectionLock m_servicesLock;
    mutable mkuint32 m_previousServiceIdx = 0;
    mutable mkuint32 m_activeServiceCnt = 0;

    struct Request
    {
        Request(
                const std::shared_ptr<MkMemoryVessel>& rRequestMessage,
                const std::shared_ptr<MkOSEvent>& rResponseEvent,
                const MkDuration retryDuration) :
            m_rRequestMessage(rRequestMessage),
            m_rResponseEvent(rResponseEvent),
            m_retryDuration(retryDuration)
        { }

        const std::shared_ptr<MkMemoryVessel> m_rRequestMessage;
        const std::shared_ptr<MkOSEvent> m_rResponseEvent;
        const MkDuration m_retryDuration;
    };

    std::map<MkUID, Request> m_pendingRequestMap;
    std::map<mkuint32, std::set<MkUID>> m_serviceRequestUIDMap;
    std::map<MkUID, std::shared_ptr<MkMemoryVessel>> m_responseMap;
    mutable MkCriticalSectionLock m_transactionMapsLock;

    // NOTE: Built once.  MRK.
    MkMemoryVessel m_connectMessage;
};
