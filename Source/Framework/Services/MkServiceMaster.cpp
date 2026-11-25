////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2025 by Michael R. Kleist
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "Framework\MkFrameworkPCH.h"
#include "Services\MkServiceMaster.h"

// Mk includes.
#include "Basic\MkException.h"
#include "Basic\MkMemoryVessel.h"
#include "InputOutput\MkMBTP.h"
#include "Process\MkFrameworkApplication.h"

#define DEBUG 0
#if DEBUG
#include "InputOutput\MkDebugText.h"
#endif

// Static member initializations.
std::unique_ptr<MkSingleton<MkServiceMaster>::InstanceT> MkSingleton<MkServiceMaster>::m_srInstance = nullptr;
MkCriticalSectionLock MkSingleton<MkServiceMaster>::m_sLock;

// Static initializations.
namespace
{
MkDuration sAcceptTimeOut = MkDuration(1.0, MkDuration::eSeconds);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// ConnectServiceTask
////////////////////////////////////////////////////////////////////////////////////////////////////

class ConnectServiceTask : public MkMBTPReader::DispatchTask
{
public:
    ConnectServiceTask(
            const MkAString& name,
            MkServiceMaster* pServiceMaster,
            const std::shared_ptr<MkSocketServer>& rRequestSocket) :
        MkMBTPReader::DispatchTask(name, true),
        m_pServiceMaster(pServiceMaster),
        m_rRequestSocket(rRequestSocket)
    { }
    ~ConnectServiceTask() override = default;

    MkTask::Status Execute() override;

private:
    void SendError(const MkService::ConnectionStatus connectionStatus);

    MkServiceMaster* m_pServiceMaster;
    std::shared_ptr<MkSocketServer> m_rRequestSocket;
};

////////////////////////////////////////////////////////////////////////////////////////////////////

MkTask::Status ConnectServiceTask::Execute()
{
#if DEBUG
    { MkDebugSystemText() << "Service master processing connection request.\n"; }   
#endif

    // Read and verify the request.
    MkService::RequestCode requestCode;
    m_rMessage->ReadObject(requestCode);

    if (requestCode != MkService::eServiceConnect)
    {
        SendError(MkService::eInvalidConnection);
        return eComplete;
    }

    // Read and verify the service name.
    MkAString serviceName = m_rMessage->ReadString();

    // Get the service.
    auto rService = m_pServiceMaster->GetService(serviceName);
    if (rService == nullptr)
    {
        SendError(MkService::eNotAvailable);
        return eComplete;
    }

    // Check if the service has the required resources.
    std::vector<std::shared_ptr<MkResourceLabel>> resourceLabels;
    m_rMessage->ReadObjects(resourceLabels);

    if (!rService->Satisfies(resourceLabels))
    {
        SendError(MkService::eResourceNotAvailable);
        return eComplete;
    }

    // Get this client set up.
    mkuint32 clientIdx = rService->AddClient(m_rRequestSocket);

    // Send service connection accepted message.
    MkMemoryVessel responseMessage(MkMemoryVessel::eTiny);
    responseMessage.WriteObject(MkMBTPHeader());
    responseMessage.WriteObject(MkService::eAvailable);
    responseMessage.WriteObject(rService->GetPendingRequestCount());

#if DEBUG
    { MkDebugSystemText() << "SSS Service master writing service connection response.\n"; }   
#endif

    volatile bool complete = false;
    MkMBTPWriter mbtpWriter;
    while (!complete)
    {
        if (!mbtpWriter.WriteMessage(*m_rRequestSocket, responseMessage, complete))
        {
            rService->RemoveClient(clientIdx);
            return eComplete;
        }
    }

#if DEBUG
    { MkDebugSystemText() << "Service master service connection succeeded.\n"; }   
#endif
    
    return eComplete;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void ConnectServiceTask::SendError(const MkService::ConnectionStatus connectionStatus)
{
    MkMemoryVessel errorMessage;
    errorMessage.WriteObject(MkMBTPHeader());
    errorMessage.WriteObject(connectionStatus);

#if DEBUG
    { 
        MkDebugSystemText() <<
                "SSS !!! Service master writing service connection error '" << connectionStatus << "'.\n";
    }   
#endif

    volatile bool complete = false;
    MkMBTPWriter mbtpWriter;
    while (!complete)
    {
        if (!mbtpWriter.WriteMessage(*m_rRequestSocket, errorMessage, complete)) break;
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// ReadConnectTask
////////////////////////////////////////////////////////////////////////////////////////////////////

class ReadConnectTask : public MkTask
{
public:
    ReadConnectTask(
            const MkAString& name, 
            MkServiceMaster* pServiceMaster,
            std::shared_ptr<MkSocketServer>& rRequestSocket) :
        MkTask(name),
        m_pServiceMaster(pServiceMaster),
        m_rRequestSocket(rRequestSocket)
    { }
    ~ReadConnectTask() override = default;

    MkTask::Status Execute() override;

    void SendError(const MkService::ConnectionStatus connectionStatus);

private:
    MkServiceMaster* m_pServiceMaster;
    std::shared_ptr<MkSocketServer> m_rRequestSocket;
};

////////////////////////////////////////////////////////////////////////////////////////////////////

MkTask::Status ReadConnectTask::Execute()
{
    auto rConnectServiceTask =
            std::make_shared<ConnectServiceTask>("ConnectService", m_pServiceMaster, m_rRequestSocket);

#if DEBUG
    { MkDebugSystemText() << "RRR Service master reading a service connection message.\n"; }
#endif

    volatile bool complete = false;
    MkMBTPReader mbtpReader(rConnectServiceTask);
    while (!complete)
    {
        if (!mbtpReader.ReadMessage(*m_rRequestSocket, complete)) break;
    }

    return eComplete;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// MkServiceMaster
////////////////////////////////////////////////////////////////////////////////////////////////////

MkServiceMaster::MkServiceMaster() :
    m_services()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////

bool MkServiceMaster::Execute(const MkAString& portStr)
{
    m_rRequestAcceptor = std::make_shared<MkSocketAcceptor>(portStr);
    if (!m_rRequestAcceptor->IsValid())
    {
        throw MkException(eOSFailure, "Invalid listener socket.");
    }

    while (true)
    {
#if DEBUG
        { MkDebugSystemText() << "Entering service master accept.\n"; }
#endif

        // Generate the private socket server.
        auto rRequestSocket = m_rRequestAcceptor->Accept(sAcceptTimeOut);
        if (rRequestSocket == nullptr) continue;

#if DEBUG_SOCKET
    rRequestSocket->m_name = MkCAS("ServiceRequestSocket");
#endif

#if DEBUG
        { MkDebugSystemText() << "Service master accept occurred.\n"; }
#endif

        // Schedule a task for registering a client with the service.
        Framework::TaskPool().Schedule(
                std::make_shared<ReadConnectTask>("ReadConnect", this, rRequestSocket),
                MkTask::eNormal);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void  MkServiceMaster::AddService(const MkAString& name, const std::shared_ptr<MkService>& rService)
{
    MkCriticalSection cs(m_servicesLock);
    m_services.insert(std::make_pair(name, rService));
}

////////////////////////////////////////////////////////////////////////////////////////////////////

std::shared_ptr<MkService> MkServiceMaster::GetService(const MkAString& name)
{
    MkCriticalSection cs(m_servicesLock);
    auto serviceIter = m_services.find(name);

    if (serviceIter == m_services.end()) return nullptr;

    return serviceIter->second;
}