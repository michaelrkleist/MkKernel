////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2026 by Michael R. Kleist
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

// Wx includes.
#include "MkFoundation.h"
#if 0
//#include "Basic\MkInterval.h"
#include "Basic\MkString.h"
#include "InputOutput\MkIP.h"
//#include "Process\MKOSEvent.h"

#include "External\curl\curl.h"

#include <variant>

// Forward declarations.


////////////////////////////////////////////////////////////////////////////////////////////////////

class MkHTTPResource
{
public:
    struct Parameter
    {
        enum Type
        {
            eString,
            eNumber,
            eFloat,
            eBool
        };

        Type type;
        bool optional;
        union {
                int minRgVal;
                int minStrLen; };
        union {
                int maxRgVal;
                int maxStrLen; };
    };

public:   
    WxRef<WxHttpResponse> DoProcessRequest(const WxRef<WxHttpRequest>& rSession);

    void SetId(const std::string& resourceId);

    bool IsPath() const
    { return m_isPath; }

    const std::string& GetId() const
    { return m_id; }

    virtual void GetInfo(json &desc)
    {
        desc = {
                { "type", "Http API" },
                { "name", m_id } };
    }

    void SetLogEnabled(bool value)
    { m_logEnabled = true; }

    bool GetLogEnabled() const
    { return m_logEnabled; }

protected:
    virtual WxRef<WxHttpResponse> ProcessRequest(const WxRef<WxHttpRequest>& rSession);

    MkHTTPResource() :
        m_counterMetric(-1),
        m_timingGaugeMetric(-1),
        m_totalTimeMetric(-1),
        m_logEnabled(false),
        m_isPath(false)
    { }

    virtual ~MkHTTPResource() = default;

    friend class WxHttpChannel;

protected:
    std::string m_id;
    std::size_t m_counterMetric;
    std::size_t m_totalTimeMetric;
    std::size_t m_timingGaugeMetric;
    bool m_isPath;
    bool m_logEnabled;
};


////////////////////////////////////////////////////////////////////////////////////////////////////

class WX_EXPORT_SHARED MkHTTPOkResource : public MkHTTPResource
{
public:
    static WxRef<MkHTTPOkResource> Create()
    { return new MkHTTPOkResource(); }

    std::shared_ptr<MkHTTPResponse> ProcessRequest(
            const std::shared_ptr<WxHTTPRequest>& rRequest) override
    { return WxHTTPResponse::CreateWx("ok", "text/plain", 200); }
};

////////////////////////////////////////////////////////////////////////////////////////////////////


class WX_EXPORT_SHARED WxAsyncHttpResourceHandler : public WxRefCounter
{
public:
    template <typename T>
    struct WorkReadyT
    {
        T m_value;
    };

    using WorkReady = WorkReadyT<WxRef<WxHttpResponse>>;

    template <typename T>
    struct WorkContinueT
    {
        WxTime m_at;
        std::function<std::variant<WorkReadyT<T>, WorkContinueT<T>>()> m_continueFn;
    };

    template <typename T>
    using VariantT = std::variant<WorkReadyT<T>, WorkContinueT<T>>;

    using Variant = VariantT<WxRef<WxHttpResponse>>;

    // Receives ref to `metricsUrl` so that it can be read and modified if necessary.
    virtual Variant ProcessRequestAsync(
            const WxRef<WxHttpRequest>& request,
            const std::vector<std::string>& params,
            std::string& metricsUrl) = 0;

    // Apply `fn` to value of `inp` when it is ready.
    template <typename T, typename S, typename F>
    static WxAsyncHttpResourceHandler::VariantT<T> Map(WxAsyncHttpResourceHandler::VariantT<S> inp, F fn)
    {
        if (inp.index() == 0)
        {
            return WxAsyncHttpResourceHandler::WorkReadyT<T>{ fn(std::get<0>(inp).m_value) };
        }
        else
        {
            return WxAsyncHttpResourceHandler::WorkContinueT<T>{
                    std::get<1>(inp).m_at,
                    [inp, fn]()
                    {
                        WxAsyncHttpResourceHandler::VariantT<S> v = std::get<1>(inp).m_continueFn();
                        return Map<T, S, F>(v, fn);
                    }};
        }
    }

    // Keep running `fn` each `interval` until it returns true
    template <typename T, typename F>
    static WxAsyncHttpResourceHandler::VariantT<T> Timer(const WxTimeInterval& interval, F fn)
    {
        return WxAsyncHttpResourceHandler::WorkContinueT<T>{
                WxTime::CurrentTime() + interval,
                [interval, fn]() -> WxAsyncHttpResourceHandler::VariantT<T>
                {
                    T t;
                    if (fn(t))
                    {
                        return WxAsyncHttpResourceHandler::WorkReadyT<T>{ t };
                    }
                    else
                    {
                        return Timer<T, F>(interval, fn);
                    }
                }};
    }
};


////////////////////////////////////////////////////////////////////////////////////////////////////

// Adapter, fullfills request in one take.
class WxAsyncHttpResourceHandlerAdapter : public WxAsyncHttpResourceHandler
{
public:
    static WxRef<WxAsyncHttpResourceHandlerAdapter> Create(WxRef<WxHttpResourceHandler> rSync)
    { return new WxAsyncHttpResourceHandlerAdapter(rSync); }

    Variant ProcessRequestAsync(
            const WxRef<WxHttpRequest>& rRequest,
            const std::vector<std::string>& params,
            std::string& metricsUrl) override
    { return WorkReady{ m_rHandler->DoProcessRequest(rRequest) }; }

private:
    WxAsyncHttpResourceHandlerAdapter(WxRef<WxHttpResourceHandler>& rHandler) :
        m_rHandler(rHandler)
    { }
    WxRef<WxHttpResourceHandler> m_rHandler;
};


////////////////////////////////////////////////////////////////////////////////////////////////////

// Adapter, fullfills prefix request in one take.
class WxAsyncHttpRouterHandlerAdapter : public WxAsyncHttpResourceHandler {
public:
    static WxRef<WxAsyncHttpRouterHandlerAdapter> Create(WxRef<WxLuaHttpRequestRouter> rRouter)
    { return new WxAsyncHttpRouterHandlerAdapter(rRouter); }

    Variant ProcessRequestAsync(
            const WxRef<WxHttpRequest>& rRequest,
            const std::vector<std::string>& params,
            std::string& metricsUrl) override;

private:
    WxAsyncHttpRouterHandlerAdapter(WxRef<WxLuaHttpRequestRouter>& rHandler) :
        m_rHandler(rHandler)
    { }

    WxRef<WxLuaHttpRequestRouter> m_rHandler;
};


////////////////////////////////////////////////////////////////////////////////////////////////////

// Default metrics resource handler.
class WX_EXPORT_SHARED WxMetricsResourceHandler : public WxAsyncHttpResourceHandler
{
public:
    static WxRef<WxMetricsResourceHandler> Create()
    { return new WxMetricsResourceHandler(); }

    Variant ProcessRequestAsync(
            const WxRef<WxHttpRequest>& rReq,
            const std::vector<std::string>& params,
            std::string& metricsUrl) override;
};


////////////////////////////////////////////////////////////////////////////////////////////////////

class WX_EXPORT_SHARED WxWebServer : public WxRefCounter
{
public:
    static WxRef<WxWebServer> Create(wxuint port, const std::string& host = "0.0.0.0");

    void AddPrefix(const char* pPrefix, WxRef<WxLuaHttpRequestRouter> rResource);
    void AddGetResource(
            const char* path,
            std::function<void(
                    uWS::Loop*,
                    uWS::HttpResponse<false>*,
                    uWS::HttpRequest*,
                    WxRef<WxHttpRequest>)> rHandler,
            bool requireAuth = true);

    void AddPostResource(
            const char* path,
            std::function<void(uWS::HttpResponse<false>*,
            uWS::HttpRequest*, WxRef<WxHttpRequest>)> rHandler);

    void AddDeleteResource(
            const char* path,
            std::function<void(uWS::HttpResponse<false>*, uWS::HttpRequest*,
            WxRef<WxHttpRequest>)> rHandler);

    void AddAsyncResource(const char* path, WxRef<WxAsyncHttpResourceHandler> rResource);
    void AddResource(const char* path, WxRef<WxHttpResourceHandler> rResource, bool defer = true);
    void AddResource(const WxString& path, WxRef<WxHttpResourceHandler> rResource, bool defer = true)
    { AddResource(path.ToSingleByte(WxString::eUTF8).c_str(), rResource, defer); }

    void AddWsRoute(const char* pPrefix, std::unique_ptr<WxWebSocketHandler> rWsResource);
    void SetAuthorizationHandler(const WxRef<WxHttpAuthorization>& rHandler);
    bool Start(wxuint workersCnt = 4, wxuint wsWorkersCnt = 0);
    void NotifyClientsWs(const char* pTopic, const char* pPayload = "");

    // TODO: it is valid to AddPrefixes only before Start.

    struct LoadBalancer
    {
        std::vector<uWS::Loop*> m_pWsWorkerLoops;
        std::atomic<wxuint> m_counter = 0;

        wxuint GetNextWorker()
        {
            wxuint idx = m_counter.fetch_add(1);
            return idx % m_pWsWorkerLoops.size();
        }

        uWS::Loop* GetWorker(wxuint idx)
        { return m_pWsWorkerLoops[idx]; }
    };
    
private:
    std::string m_host;
    wxuint m_port;
    struct PrefixHandler
    {
        WxRef<WxAsyncHttpResourceHandler> m_rHandler;
        bool m_defer;
    };
    std::vector<std::pair<std::string, PrefixHandler>> m_prefixes;
    std::vector<std::pair<std::string, std::unique_ptr<WxWebSocketHandler>>> m_wsRoutes;

    struct HandlerDef
    {
        std::string m_path;
        std::function<void(uWS::Loop*, uWS::HttpResponse<false>*, uWS::HttpRequest*, WxRef<WxHttpRequest>)> m_fn;
        bool m_requireAuth;
    };

    std::vector<HandlerDef> m_get;
    std::vector<std::pair<
            std::string,
            std::function<void(uWS::HttpResponse<false>*, uWS::HttpRequest*, WxRef<WxHttpRequest>)>>> m_post;
    std::vector<std::pair<
            std::string,
            std::function<void(uWS::HttpResponse<false>*, uWS::HttpRequest*, WxRef<WxHttpRequest>)>>> m_delete;

    WxRef<WxHttpAuthorization> m_rAuthorizationHandler;

    struct ThreadPair
    {
        std::thread m_listener;
        std::thread m_worker;
        us_timer_t* m_pWorkerTimer;
        uWS::Loop* m_pMainLoop;
    };

    WxCriticalSectionToken m_threadsLock;
    std::vector<ThreadPair> m_threads;

    struct WsWorker
    {
        std::thread m_worker;
        us_timer_t* m_pWorkerTimer;
    };

    std::vector<WsWorker> m_wsThreads;
    LoadBalancer m_wsBalancer;

    void Run(wxuint id, uWS::Loop* pMainLoop, uWS::Loop* pWorkerLoop);

    WxWebServer(wxuint port, const std::string& host = "0.0.0.0");
    WxWebServer(const WxWebServer& other) = delete;
    WxWebServer& operator=(const WxWebServer& other) = delete;
    ~WxWebServer();
};
#endif