////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2025 by Michael R. Kleist
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "Framework\MkFrameworkPCH.h"
#include "Services\MkClient.h"

// Mk includes.
#include "Basic\MkException.h"
#include "Basic\MkMemoryVessel.h"
#include "InputOutput\MkSocket.h"
#include "Process\MkFrameworkApplication.h"
#include "Process\MkTaskPool.h"
#include "Services\MkResourceLabel.h"
#include "Services\MkServiceMaster.h"

#define DEBUG 0
#if DEBUG
#include "InputOutput\MkDebugText.h"
#endif

// Static initializations.
namespace
{
MkDuration sReconnectDuration = MkDuration(100ll, MkDuration::eMilliseconds);

mkuint32 sMaxPendingRequestCnt = 1'000'000;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// HandleResponseTask
////////////////////////////////////////////////////////////////////////////////////////////////////

class HandleResponseTask : public MkMBTPReader::DispatchTask
{
public:
    HandleResponseTask(
            const MkAString& name,
            MkClient* pClient, 
            const mkuint32 serviceIdx) :
        MkMBTPReader::DispatchTask(name),
        m_pClient(pClient),
        m_serviceIdx(serviceIdx)
    { }
    ~HandleResponseTask() override = default;

    Status Execute() override;

private:
    MkClient* m_pClient;
    mkuint32 m_serviceIdx;
};

////////////////////////////////////////////////////////////////////////////////////////////////////

MkTask::Status HandleResponseTask::Execute()
{
    MkService::ResponseCode responseCode;
    m_rMessage->ReadObject(responseCode);

    switch (responseCode)
    {
    case MkService::eRequestComplete :
    {
        MkUID requestUID(false);
        m_rMessage->ReadObject(requestUID);

        m_pClient->PostResponse(m_rMessage, requestUID, m_serviceIdx);
        break;
    }

    case MkService::ePendingRequestCnt :
        m_rMessage->ReadObject(m_pClient->m_services[m_serviceIdx].m_pendingRequestCnt);
        break;

    default :
        throw MkException(eLogicError, "Invalid response code.");
    }

    return eComplete;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// ClientSocketEventFunctor
////////////////////////////////////////////////////////////////////////////////////////////////////

// Handle events on a service response socket.
class ClientSocketEventFunctor : public MkEventScheduler::Functor
{
public:
    ClientSocketEventFunctor(
            const MkAString& name,
            MkClient* pClient, 
            const mkuint32 serviceIdx) :
        MkEventScheduler::Functor(name),
        m_pClient(pClient),
        m_serviceIdx(serviceIdx)
    { }
    ~ClientSocketEventFunctor() override = default;

    void operator()() override
    { ScheduleSelf(); }

    Status Execute() override;

private:
    MkClient* m_pClient;
    mkuint32 m_serviceIdx;
};

////////////////////////////////////////////////////////////////////////////////////////////////////

MkTask::Status ClientSocketEventFunctor::Execute()
{
#if DEBUG
    { MkDebugSystemText() << "!!! Client socket event for service " << m_serviceIdx << ".\n"; }   
#endif

    MkCriticalSection cs(m_pClient->m_servicesLock);
    MkClient::ServiceData serviceData = m_pClient->m_services[m_serviceIdx];

    // NOTE: You can receive socket read events AFTER a close event.  MRK.
    if (serviceData.m_status != MkClient::eConnected) return eComplete;

    auto requestSocket = *serviceData.m_rRequestSocket;
    auto events = requestSocket.IdentifyEvent();

    // NOTE: There are a surprising number of spurious socket events.  MRK.
    if (events.IsEmpty()) return eComplete;

    if (events.ReadEvent())
    {
#if DEBUG
        { MkDebugSystemText() << "RRR Client reading service response message.\n"; }   
#endif
        // NOTE: A little bit of magic here as the reader happily reads messages
        // incrementally and dispatches complete messages.  MRK.
        volatile bool complete;
        if (!serviceData.m_rMBTPReader->ReadMessage(requestSocket, complete))
        {
            // TODO: Set any abandoned service request up for retry.  MRK.
            // NOTE: Need to set up a data structure for keeping track of what requests
            // have been sent to a particular service.  Next release.  MRK.
            m_pClient->DisconnectService(m_serviceIdx);
        }
    }

    if (events.CloseEvent())
    {
#if DEBUG
        { MkDebugSystemText() << "Client socket close event.\n"; }   
#endif
        // TODO: Set any abandoned service request up for retry.  MRK.
        // NOTE: Need to set up a data structure for keeping track of what requests
        // have been sent to a particular service.  Next release.  MRK.
        m_pClient->DisconnectService(m_serviceIdx);
    }

    return eComplete;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// AttachServiceTask
////////////////////////////////////////////////////////////////////////////////////////////////////

class AttachServiceTask : public MkMBTPReader::DispatchTask
{
public:
    AttachServiceTask(
            const MkAString& name,
            MkClient* pClient,
            const mkuint32 serviceIdx) :
        MkMBTPReader::DispatchTask(name, true),
        m_pClient(pClient),
        m_serviceIdx(serviceIdx)
    { }
    ~AttachServiceTask() override = default;

    Status Execute() override;

private:
    void SendError(const MkService::ConnectionStatus connectionStatus);

    MkClient* m_pClient;
    mkuint32 m_serviceIdx;
};

////////////////////////////////////////////////////////////////////////////////////////////////////

MkTask::Status AttachServiceTask::Execute()
{
    MkService::ConnectionStatus connectionStatus;
    m_rMessage->ReadEnum(connectionStatus);

    MkCriticalSection cs(m_pClient->m_servicesLock);
    switch (connectionStatus)
    {
    case MkService::eAvailable :
    {
        m_pClient->m_services[m_serviceIdx].m_rMBTPReader = std::make_shared<MkMBTPReader>(
                std::make_shared<HandleResponseTask>(
                        MkConstAString("HandleResponse"), m_pClient, m_serviceIdx));

        m_pClient->m_services[m_serviceIdx].m_rRequestSocket->ActivateEvent();
        Socket::EventScheduler().Schedule(
                m_pClient->m_services[m_serviceIdx].m_rRequestSocket->GetEvent(),
                std::make_shared<ClientSocketEventFunctor>(
                        MkConstAString("ClientSocketEvent"), m_pClient, m_serviceIdx),
                true);

#if DEBUG
        { MkDebugSystemText() << "Activated client socket events.\n"; }   
#endif

        m_pClient->m_services[m_serviceIdx].m_status = MkClient::eConnected;
        ++m_pClient->m_activeServiceCnt;
        break;
    }

    case MkService::eNotAvailable :
        m_pClient->m_services[m_serviceIdx].m_status = MkClient::eNoService;
        break;

    case MkService::eResourceNotAvailable :
        m_pClient->m_services[m_serviceIdx].m_status = MkClient::eInsufficientResources;
        break;

    default :
        throw MkException(eLogicError, "Invalid connection status.");
        break;
    }

    return eComplete;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// ConnectFunctor
////////////////////////////////////////////////////////////////////////////////////////////////////

class ConnectFunctor : public MkTimeScheduler::Functor
{
public:
    ConnectFunctor(MkClient* pClient) :
        MkTimeScheduler::Functor("ConnectServices"),
        m_pClient(pClient)
    { }
    ~ConnectFunctor() override = default;

    void operator()() override
    { ScheduleSelf(); }

    MkTask::Status Execute() override
    { m_pClient->ConnectServices(); return MkTask::eComplete; }

private:
    MkClient* m_pClient;
};


////////////////////////////////////////////////////////////////////////////////////////////////////
// RetryRequestFunctor
////////////////////////////////////////////////////////////////////////////////////////////////////

class RetryRequestFunctor : public MkTimeScheduler::Functor
{
public:
    RetryRequestFunctor(
            const MkAString& name,
            MkClient* pClient,
            const MkUID& requestUID) :
        Functor(name),
        m_pClient(pClient),
        m_requestUID(requestUID)
    { }
    ~RetryRequestFunctor() override = default;

    void operator()() override
    { ScheduleSelf(); }

    MkTask::Status Execute() override;

private:
    MkClient* m_pClient;
    MkUID m_requestUID;
};

////////////////////////////////////////////////////////////////////////////////////////////////////

MkTask::Status RetryRequestFunctor::Execute()
{
    m_pClient->RemakeRequest(m_requestUID);
    return eComplete;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// MkClient
////////////////////////////////////////////////////////////////////////////////////////////////////

MkClient::MkClient(
        const MkAString& serviceName,
        const std::vector<MkIP4PortString>& serviceAddresses,
        const std::vector<std::shared_ptr<MkResourceLabel>>& resourceLabels,
        const bool loadBalance) :
    m_serviceName(serviceName),
    m_resourceLabels(resourceLabels),
    m_loadBalance(loadBalance),
    m_connectMessage(MkMemoryVessel::eTiny)
{
    for (auto& addressIter : serviceAddresses)
    {
        m_services.push_back(ServiceData(addressIter.GetIP4AddressString(), addressIter.GetPortString()));
    }

    MkAString clientHostName;
    MkGetHostName(clientHostName);

    // Put together the service connect message.
    m_connectMessage.ResetForWrite();
    m_connectMessage.WriteObject(MkMBTPHeader());
    m_connectMessage.WriteEnum(MkService::eServiceConnect);
    m_connectMessage.WriteObject(m_serviceName);
    m_connectMessage.WriteObjects(m_resourceLabels);

    ConnectServices();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

MkClient::~MkClient()
{
    for (mkuint32 serviceIdx = 0; serviceIdx < m_services.size(); ++serviceIdx)
    {
        if (m_services[serviceIdx].m_status == eConnected)
        {
            DisconnectService(serviceIdx);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

bool MkClient::CheckResponse(const MkUID& requestUID) const
{
    MkCriticalSection cs(m_transactionMapsLock);
    auto responseIter = m_responseMap.find(requestUID);

    if (responseIter == m_responseMap.end()) return false;

    return bool(responseIter->second != nullptr);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

std::shared_ptr<MkMemoryVessel> MkClient::TakeResponse(const MkUID& requestUID)
{
    MkCriticalSection cs(m_transactionMapsLock);
    auto responseIter = m_responseMap.find(requestUID);

    if (responseIter == m_responseMap.end()) return nullptr;

    auto rResponseMessage = responseIter->second;
    m_responseMap.erase(responseIter);
    return rResponseMessage;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void MkClient::FillBaseRequest(
        const std::shared_ptr<MkMemoryVessel>& rRequestMessage,
        const MkUID& requestUID,
        mkuint32 requestCode) const
{
    rRequestMessage->WriteObject(MkMBTPHeader());
    rRequestMessage->WriteObject(MkService::eServiceRequest);
    rRequestMessage->WriteObject(requestUID);
    rRequestMessage->WriteObject(requestCode);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

bool MkClient::GetNextServiceIndex(mkuint32& serviceIdx) const
{
    MkCriticalSection cs(m_servicesLock);
    if (m_activeServiceCnt == 0) return false;

    if (m_loadBalance)
    {
        mkuint minPendingRequestCnt = sMaxPendingRequestCnt;
        for (mkuint32 srvIdx = 0; srvIdx < m_services.size(); ++srvIdx)
        {
            if (m_services[srvIdx].m_status == eConnected)
            {
                if (m_services[srvIdx].m_pendingRequestCnt < minPendingRequestCnt)
                {
                    serviceIdx = srvIdx;
                    minPendingRequestCnt = m_services[srvIdx].m_pendingRequestCnt;
                }
            }
        }
        return true;
    }

    serviceIdx = m_previousServiceIdx;
    while (true)
    {
        serviceIdx = (serviceIdx + 1) % m_services.size();
        if (m_services[serviceIdx].m_status == eConnected) break;
    }

    m_previousServiceIdx = serviceIdx;
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

bool MkClient::MakeRequest(
        const std::shared_ptr<MkMemoryVessel>& rRequestMessage,
        const MkUID& requestUID,
        const std::shared_ptr<MkOSEvent>& rResponseEvent,
        const MkDuration retryDuration)
{
    {
        MkCriticalSection cs(m_transactionMapsLock);
        m_pendingRequestMap.insert(
                std::make_pair(requestUID, Request(rRequestMessage, rResponseEvent, retryDuration)));
    }

    if (retryDuration.IsPositive())
    {
        Framework::TimeScheduler().Schedule(
                retryDuration,
                std::make_shared<RetryRequestFunctor>("RetryRequest", this, requestUID));
    }

    return bool(SendRequest(requestUID) == eRequestSent);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void MkClient::RemakeRequest(const MkUID& requestUID)
{
    std::shared_ptr<MkMemoryVessel> rRequestMessage;
    MkDuration retryDuration;

    {
        MkCriticalSection cs(m_transactionMapsLock);
        auto pendingRequestIter = m_pendingRequestMap.find(requestUID);
        if (pendingRequestIter == m_pendingRequestMap.end()) return;

        rRequestMessage = pendingRequestIter->second.m_rRequestMessage;
        retryDuration = pendingRequestIter->second.m_retryDuration;
    }

    // Keep retrying, like, forever.
    // TODO: Let the client know somehow?  MRK.
    Framework::TimeScheduler().Schedule(
            retryDuration,
            std::make_shared<RetryRequestFunctor>("RetryRequest", this, requestUID));

    SendRequest(requestUID); 
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void MkClient::RemoveRequest(const MkUID& requestUID)
{
    MkCriticalSection cs(m_transactionMapsLock);
    m_pendingRequestMap.erase(requestUID);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

MkClient::RequestStatus MkClient::SendRequest(const MkUID& requestUID)
{
#if 0
#if DEBUG
    { MkDebugSystemText() << "Client trying to send service request.\n"; }   
#endif
#endif

    mkuint32 serviceIdx;
    if (!GetNextServiceIndex(serviceIdx)) return eNoAvailableService;

    std::shared_ptr<MkSocketClient> rRequestSocket = nullptr;
    {
        MkCriticalSection cs(m_servicesLock);
        if (m_services[serviceIdx].m_status != eConnected) return eNoAvailableService;
        rRequestSocket = m_services[serviceIdx].m_rRequestSocket;
    }

    std::shared_ptr<MkMemoryVessel> rRequestMessage = nullptr;
    {
        MkCriticalSection cs(m_transactionMapsLock);
        auto requestIter = m_pendingRequestMap.find(requestUID);
        if (requestIter == m_pendingRequestMap.end()) return eAlreadySent;

        rRequestMessage = requestIter->second.m_rRequestMessage;

        // NOTE: Fun with maps and sets.  MRK.
        auto servicePair = m_serviceRequestUIDMap.insert(std::make_pair(serviceIdx, std::set<MkUID>()));
        servicePair.first->second.insert(requestUID);
    }

#if DEBUG
    { MkDebugSystemText() << "SSS Client writing service request message.\n"; }   
#endif

    // We have assembled a service request.  Send it to the indicated service.  Report
    // an error if the service cannot be contacted.
    volatile bool complete = false;
    MkMBTPWriter mbtpWriter;
    while (!complete)
    {
        if (!mbtpWriter.WriteMessage(*rRequestSocket, *rRequestMessage, complete))
        {
            DisconnectService(serviceIdx);
            return eSocketClosed;
        }
    }

    return eRequestSent;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void MkClient::PostResponse(
        const std::shared_ptr<MkMemoryVessel>& rResponseMessage,
        const MkUID& requestUID,
        const mkuint32 serviceIdx)
{
    MkCriticalSection cs(m_transactionMapsLock);
    m_responseMap[requestUID] = rResponseMessage;   // Later (retry) responses replace earlier responses.

    auto requestIter = m_pendingRequestMap.find(requestUID);
    if (requestIter != m_pendingRequestMap.end())
    {
        if (requestIter->second.m_rResponseEvent != nullptr)
        {
            requestIter->second.m_rResponseEvent->Signal();
        }   

        m_pendingRequestMap.erase(requestIter);
    }

    auto serviceIter = m_serviceRequestUIDMap.find(serviceIdx);
    if (serviceIter == m_serviceRequestUIDMap.end()) return;

    auto requestUIDIter = serviceIter->second.find(requestUID);
    if (requestUIDIter == serviceIter->second.end()) return;

    serviceIter->second.erase(requestUIDIter);
}


////////////////////////////////////////////////////////////////////////////////////////////////////

void MkClient::ConnectServices()
{
    MkCriticalSection cs(m_servicesLock);
    for (mkuint32 serviceIdx = 0; serviceIdx < m_services.size(); ++serviceIdx)
    {
        if (m_services[serviceIdx].m_status != eConnected)
        {
            m_services[serviceIdx].m_rRequestSocket = std::make_shared<MkSocketClient>(
                    m_services[serviceIdx].m_serviceHostName, m_services[serviceIdx].m_servicePortStr);

            if (!m_services[serviceIdx].m_rRequestSocket->IsValid())
            {
                m_services[serviceIdx].m_rRequestSocket.reset();
                m_services[serviceIdx].m_status = eInvalidSocketAddress;
                goto nextService;
            }

#if DEBUG_SOCKET
            m_services[serviceIdx].m_rRequestSocket->m_name = MkCAS("ClientRequestSocket");
#endif

#if DEBUG
            { MkDebugSystemText() << "SSS Client writing service connection message.\n"; }   
#endif

            volatile bool writeComplete = false;
            MkMBTPWriter mbtpWriter;
            while (!writeComplete)
            {
                if (!mbtpWriter.WriteMessage(
                        *m_services[serviceIdx].m_rRequestSocket, m_connectMessage, writeComplete))
                {
                    m_services[serviceIdx].m_status = eNoServiceMaster;
                    goto nextService;
                }
            }

            auto rAttachServiceTask = std::make_shared<AttachServiceTask>("AttachService", this, serviceIdx);

#if DEBUG
            { MkDebugSystemText() << "RRR Client reading service connection response message.\n"; }   
#endif

            volatile bool readComplete = false;
            MkMBTPReader mbtpReader(rAttachServiceTask);
            while (!readComplete)
            {
                if (!mbtpReader.ReadMessage(*m_services[serviceIdx].m_rRequestSocket, readComplete))
                {
                    m_services[serviceIdx].m_status = eNoServiceMaster;
                    goto nextService;
                }
            }
        }

        nextService: ;
    }

    for (auto& serviceIter : m_services)
    {
        if (serviceIter.m_status != eConnected)
        {
            // Set up to retry connecting to the services.
            Framework::TimeScheduler().Schedule(
                    sReconnectDuration, std::make_shared<ConnectFunctor>(this));
            break;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void MkClient::DisconnectService(mkuint32 serviceIdx)
{
    {
        MkCriticalSection cs(m_servicesLock);

        // Reset the service entry.
        m_services[serviceIdx].m_rRequestSocket.reset();
        m_services[serviceIdx].m_status = eIdle;
        m_services[serviceIdx].m_pendingRequestCnt = 0;
        m_previousServiceIdx = 0;
        --m_activeServiceCnt;

        // Reset the previous service index.
        m_previousServiceIdx = 0;
        for (mkuint nextServiceIdx = 0; nextServiceIdx < m_services.size(); ++nextServiceIdx)
        {
            if (m_services[nextServiceIdx].m_status == eConnected)
            {
                m_previousServiceIdx = nextServiceIdx;
                break;
            }
        }

        // Set up to retry connecting to the service.
        Framework::TimeScheduler().Schedule(sReconnectDuration, std::make_shared<ConnectFunctor>(this));
    }

    // Re-send pending service requests that are associated with this service.
    MkCriticalSection cs(m_transactionMapsLock);

    auto serviceIter = m_serviceRequestUIDMap.find(serviceIdx);
    for (auto requestIter = serviceIter->second.begin(); requestIter != serviceIter->second.end(); )
    {
        RemakeRequest(*requestIter);
        requestIter = serviceIter->second.erase(requestIter);
    }
}