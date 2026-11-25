////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2025 by Michael R. Kleist
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "Framework\MkFrameworkPCH.h"
#include "Services\MkService.h"

// Mk includes.
#include "Basic\MkMemoryVessel.h"
#include "Basic\MkUID.h"
#include "InputOutput\MkMBTP.h"
#include "Process\MkFrameworkApplication.h"
#include "Process\MkTaskPool.h"
#include "Services\MkResourceLabel.h"
#include "Services\MkServiceMaster.h"

#define DEBUG 0
#if DEBUG
#include "InputOutput\MkDebugText.h"
#endif


////////////////////////////////////////////////////////////////////////////////////////////////////
// HandleRequestTask
////////////////////////////////////////////////////////////////////////////////////////////////////

class HandleRequestTask : public MkMBTPReader::DispatchTask
{
public:
    HandleRequestTask(
            const MkAString& name,
            MkService* pService, 
            const mkuint32 clientIdx) :
        MkMBTPReader::DispatchTask(name, true),         // Execute in line.
        m_pService(pService),
        m_clientIdx(clientIdx)
    { }
    ~HandleRequestTask() override = default;

    MkTask::Status Execute() override;

private:
    MkService* m_pService;
    const mkuint32 m_clientIdx;
};

////////////////////////////////////////////////////////////////////////////////////////////////////

MkTask::Status HandleRequestTask::Execute()
{
    // Read the request.  It can be a request for the service as a whole or a request
    // for a particular service operation.
    MkService::RequestCode requestCode;
    m_rMessage->ReadObject(requestCode);

    switch ((MkService::RequestCode)requestCode)
    {
    case MkService::eServiceRequest :
    {
        mkuint32 pendingRequestCnt = 0;
        {
            MkCriticalSection cs(m_pService->m_pendingRequestsLock);
            m_pService->m_pendingRequests.push_back(MkService::Request(m_clientIdx, m_rMessage));

            // Let the client know how many pending requests there are.
            // NOTE: Yes the value sent is not strictly accurate.  MRK.
            pendingRequestCnt = (mkuint32)m_pService->m_pendingRequests.size();
            
            if (m_pService->m_activeRequestCnt < m_pService->m_maxActiveRequests)
            {
                ++(m_pService->m_activeRequestCnt);

                // Schedule a task for executing a service request.
                Framework::TaskPool().Schedule(std::make_shared<MkService::ServiceTask>(
                        "ServiceTask", m_pService), MkTask::eNormal);
            }
        }

        // Send pending request count message.
        // NOTE: Yes, this code sequence is overwrought for sending a tiny bit of
        // information.  Thinking about it. MRK. 
        MkCriticalSection cs(m_pService->m_clientMapLock);
        auto clientIter = m_pService->m_clientMap.find(m_clientIdx);
        if (clientIter == m_pService->m_clientMap.end()) return eComplete;
        MkService::ClientData& clientData = clientIter->second;
        MkSocketIO& requestSocket = *clientData.m_rRequestSocket;

        MkMemoryVessel responseMessage(MkMemoryVessel::eTiny);
        responseMessage.WriteObject(MkMBTPHeader());
        responseMessage.WriteEnum(MkService::ePendingRequestCnt);
        responseMessage.WriteObject(pendingRequestCnt);

#if DEBUG
        { MkDebugSystemText() << "SSS Service writing service request response message.\n"; }   
#endif

        if (!m_pService->SendMessage(requestSocket, responseMessage))
        {
            m_pService->RemoveClient(m_clientIdx);
        }
        return eComplete;
    }

    default :
    {
        throw MkException(eLogicError, "Invalid service request code.");
    }
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// ServiceSocketEventFunctor
////////////////////////////////////////////////////////////////////////////////////////////////////

// Handle events on a service request socket.
class ServiceSocketEventFunctor : public MkEventScheduler::Functor
{
public:
    ServiceSocketEventFunctor(
            const MkAString& name,
            MkService* pService, 
            const mkuint32 clientIdx) :
        MkEventScheduler::Functor(name),
        m_pService(pService),
        m_clientIdx(clientIdx)
    { }
    ~ServiceSocketEventFunctor() override = default;

    void operator()() override
    { ScheduleSelf(); }

    MkTask::Status Execute() override;

private:
    MkService* m_pService;
    const mkuint32 m_clientIdx;
};

////////////////////////////////////////////////////////////////////////////////////////////////////

MkTask::Status ServiceSocketEventFunctor::Execute()
{
#if DEBUG
    { MkDebugSystemText() << "!!! Service socket event for client " << m_clientIdx << ".\n"; }   
#endif

    MkCriticalSection cs(m_pService->m_clientMapLock);
    auto clientIter = m_pService->m_clientMap.find(m_clientIdx);

#if 0
#if DEBUG
    { MkDebugSystemText() << "Service client map has " <<  m_pService->m_clientMap.size() << " entries.\n"; }   
#endif
#endif

    // NOTE: You can receive socket read events AFTER a close event.  MRK.
    if (clientIter == m_pService->m_clientMap.end()) return eComplete;

    auto clientData = clientIter->second;
    MkSocketIO& requestSocket = *clientIter->second.m_rRequestSocket;

    auto events = requestSocket.IdentifyEvent();
    if (events.IsEmpty())
    {
#if DEBUG
        { MkDebugSystemText() << "??? Spurious socket event for client " << m_clientIdx << ".\n"; }   
#endif
        return eComplete;
    }

    if (events.ReadEvent())
    {
#if DEBUG
        { MkDebugSystemText() << "RRR Service reading service request message.\n"; }   
#endif
        // NOTE: A little bit of magic here as the reader happily reads messages
        // incrementally and dispatches complete messages.  MRK.
        volatile bool complete;
        if (!clientData.m_rMBTPReader->ReadMessage(requestSocket, complete))
        {
            m_pService->RemoveClient(m_clientIdx);
        }
    }

    if (events.CloseEvent())
    {
#if DEBUG
        { MkDebugSystemText() << "Service socket close event.\n"; }   
#endif
        m_pService->RemoveClient(m_clientIdx);
    }

    return eComplete;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// MkService
////////////////////////////////////////////////////////////////////////////////////////////////////

bool MkService::Satisfies(const std::vector<std::shared_ptr<MkResourceLabel>>& clientResourceLabels) const
{
    for (auto& clientLabelIter : clientResourceLabels)
    {
        for (auto& serverLabelIter : m_resourceLabels)
        {
            if (serverLabelIter->Satisfies(clientLabelIter)) goto satisfied;
        }
        return false;

        satisfied: ;
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

mkuint32 MkService::AddClient(std::shared_ptr<MkSocketServer>& rRequestSocket)
{
    auto clientIdx = GetClientIndex();
    {
#if DEBUG
        { MkDebugSystemText() << "Service adding client with index " << clientIdx << ".\n"; }   
#endif

        MkCriticalSection cs(m_clientMapLock);
        m_clientMap.insert(std::make_pair(
                clientIdx,
                ClientData(
                        rRequestSocket, 
                        std::make_shared<MkMBTPReader>(std::make_shared<HandleRequestTask>(
                                "HandleRequest", this, clientIdx)))));
    }

    rRequestSocket->ActivateEvent();
    Socket::EventScheduler().Schedule(
            rRequestSocket->GetEvent(),
            std::make_shared<ServiceSocketEventFunctor>("ServiceSocketEvent", this, clientIdx),
            true);

#if DEBUG
    { MkDebugSystemText() << "Activated service socket events.\n"; }
#endif

    return clientIdx;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void MkService::RemoveClient(const mkuint32 clientIdx)
{
    MkCriticalSection cs(m_clientMapLock);

    auto clientIter = m_clientMap.find(clientIdx);
    if (clientIter != m_clientMap.end())
    {
#if DEBUG
        { MkDebugSystemText() << "Service removing client with index " << clientIter->first << ".\n"; }   
#endif
        Socket::EventScheduler().Cancel(clientIter->second.m_rRequestSocket->GetEvent());
        m_clientMap.erase(clientIter);
    }       
}

////////////////////////////////////////////////////////////////////////////////////////////////////

MkService::Request MkService::GetPendingTask()
{
    MkCriticalSection cs(m_pendingRequestsLock);
    if (m_pendingRequests.empty() || m_activeRequestCnt > m_maxActiveRequests) return Request();

    auto request = m_pendingRequests.front();
    m_pendingRequests.pop_front();

    return request;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void MkService::SendResponse(const MkService::Response& serviceResponse)
{
    MkCriticalSection cs(m_clientMapLock);
    auto clientIter = m_clientMap.find(m_clientIdx);
    if (clientIter == m_clientMap.end()) return;

    MkService::ClientData& clientData = clientIter->second;
    MkSocketIO& requestSocket = *clientData.m_rRequestSocket;

#if DEBUG
    { MkDebugSystemText() << "SSS Service writing service response message.\n"; }   
#endif

    if (!SendMessage(requestSocket, *serviceResponse.m_rResponseMessage))
    {
        RemoveClient(m_clientIdx);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

bool MkService::SendMessage(MkSocketIO& responseSocket, MkMemoryVessel& responseMessage) const
{
    volatile bool writeComplete = false;
    MkMBTPWriter mbtpWriter;
    while (!writeComplete)
    {
        if (!mbtpWriter.WriteMessage(responseSocket, responseMessage, writeComplete)) return false;
    }
    return true;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// MkService::ServiceTask
////////////////////////////////////////////////////////////////////////////////////////////////////

MkTask::Status MkService::ServiceTask::Execute()
{
    while (true)
    {
        auto request = m_pService->GetPendingTask();
        if (request.IsNull()) break;

        m_pService->SendResponse(m_pService->ProcessRequest(request));
    }

    MkCriticalSection cs(m_pService->m_pendingRequestsLock);
    --m_pService->m_activeRequestCnt;

    return eComplete;
}
