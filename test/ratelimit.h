class GrpcClientImpl: public Client, public RateLimitAsyncCallbacks {
public:
    GrpcClientImpl(raw_async_client, timeout, transport_api_version) {

    }

    ~GrpcClientImpl() override {

    }

    static void createRequest(RateLimitRequest& request) {

    }

    void cancel() override {
        request->cancel();
        callbacks = nullptr;
    }

    void limit(RequestCallbacks& callbacks, std::string domain, vector<RateLimit:Descriptor>& descriptors) override {
        callbacks_ = &callbacks;

        RateLimitRequest request;
        createRequest(request, domain, descriptor);

        request_ = async_client_->send(service_method_, request);
    }

    void onSuccess(std::unique_ptr<RateLimitResponse>&& response) override {
        LimitStatus status = LimitStatus::OK;

        if (response->overall_code() == RateLimitResponse::OVER_LIMIT) {
            status = LimitStatus::OverLimit;
        }
        else {

        }

        ResponseHeaderMapPtr response_headers_to_add;
        RequestHeadersMapPtr request_headers_to_add;

        if (!response)
    }

    void onFailure(Status status, std::string message) override {

    }
private:
    AsyncClient<RateLimitRequest, RateLimitResponse> async_client_;
    AsyncRequest* request_{};
    RequestCallbacks* callbacks{};
    MethodDescriptor& service_method_;
};


template <typename Request, typename Response>
class AsyncClient {
public:
    AsyncClient() = default;
    AsyncClient(RawAsyncClientSharedPtr client): client_{client} {}
    virtual AsyncRequest* send(service_method, Message& request, AsyncRequestCallbacks<Response>& callbacks) {
        return Internal::sendUntyped(client_.get(), servce_method, request, callbacks);
    }

    virtual AsyncStream<Request> start(service_method, AsyncStreamCallbacks callbacks) {
        return AsyncStream<Request>(Internal::startUntyped(client_.get(), service_method, callbacks));
    }
}
