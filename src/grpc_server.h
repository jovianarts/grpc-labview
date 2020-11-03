//---------------------------------------------------------------------
// LabVIEW implementation of a gRPC Server
//---------------------------------------------------------------------
#pragma once

//---------------------------------------------------------------------
//---------------------------------------------------------------------
#ifdef __WIN32__
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

//---------------------------------------------------------------------
//---------------------------------------------------------------------
#include <grpcpp/grpcpp.h>
#include <grpcpp/impl/codegen/async_generic_service.h>
#include <grpcpp/impl/codegen/completion_queue.h>
#include <grpcpp/impl/codegen/message_allocator.h>
#include <grpcpp/impl/codegen/method_handler.h>
#include <grpcpp/impl/codegen/proto_utils.h>
#include <grpcpp/impl/codegen/server_callback.h>
#include <grpcpp/impl/codegen/server_callback_handlers.h>
#include <grpcpp/impl/codegen/server_context.h>
#include <lv_interop.h>
#include <condition_variable>
#include <future>

//---------------------------------------------------------------------
//---------------------------------------------------------------------
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

//---------------------------------------------------------------------
//---------------------------------------------------------------------
using namespace std;

//---------------------------------------------------------------------
// QueryServer LabVIEW definitions
//---------------------------------------------------------------------
typedef void* LVgRPCid;
typedef void* LVgRPCServerid;

//---------------------------------------------------------------------
//---------------------------------------------------------------------
class LabVIEWgRPCServer;
class LVMessage;
class CallData;

//---------------------------------------------------------------------
//---------------------------------------------------------------------
class EventData
{
public:
    EventData(ServerContext* context);

private:
    mutex lockMutex;
    condition_variable lock;

public:
    ServerContext* context;

public:
    void WaitForComplete();
    void NotifyComplete();
};

//---------------------------------------------------------------------
//---------------------------------------------------------------------
enum class LVMessageMetadataType
{
    Int32Value,
    FloatValue,
    DoubleValue,
    BoolValue,
    StringValue,
    MessageValue
};

//---------------------------------------------------------------------
//---------------------------------------------------------------------
class MessageElementMetadata
{
public:
    std::string embeddedMessageName;
    
    int protobufIndex;
    
    int clusterOffset;

    LVMessageMetadataType type;    

    bool isRepeated;    
};

//---------------------------------------------------------------------
//---------------------------------------------------------------------
struct LVMesageElementMetadata
{
    LStrHandle embeddedMessageName;
    int protobufIndex;
    int clusterOffset;
    int valueType;
    bool isRepeated;
};

//---------------------------------------------------------------------
//---------------------------------------------------------------------
using LVMessageMetadataList = std::map<google::protobuf::uint32, MessageElementMetadata>;

//---------------------------------------------------------------------
//---------------------------------------------------------------------
struct MessageMetadata
{
    std::string messageName;
    LVMessageMetadataList elements;
};

//---------------------------------------------------------------------
//---------------------------------------------------------------------
struct LVMessageMetadata
{
    LStrHandle messageName;
    LV1DArrayHandle elements;
};

//---------------------------------------------------------------------
//---------------------------------------------------------------------
class LVMessageValue
{
public:
    LVMessageValue(int protobufId);

public:
    int _protobufId;    

public:
    virtual void* RawValue() = 0;
    virtual size_t ByteSizeLong() = 0;
    virtual google::protobuf::uint8* Serialize(google::protobuf::uint8* target, google::protobuf::io::EpsCopyOutputStream* stream) const = 0;
};

//---------------------------------------------------------------------
//---------------------------------------------------------------------
class LVStringMessageValue : public LVMessageValue
{
public:
    LVStringMessageValue(int protobufId, std::string& value);

public:
    std::string _value;

public:
    void* RawValue() override { return (void*)(_value.c_str()); };
    size_t ByteSizeLong() override;
    google::protobuf::uint8* Serialize(google::protobuf::uint8* target, google::protobuf::io::EpsCopyOutputStream* stream) const override;
};

//---------------------------------------------------------------------
//---------------------------------------------------------------------
class LVBooleanMessageValue : public LVMessageValue
{
public:
    LVBooleanMessageValue(int protobufId, bool value);

public:
    bool _value;    

    void* RawValue() override { return &_value; };
    size_t ByteSizeLong() override;
    google::protobuf::uint8* Serialize(google::protobuf::uint8* target, google::protobuf::io::EpsCopyOutputStream* stream) const override;
};

//---------------------------------------------------------------------
//---------------------------------------------------------------------
class LVInt32MessageValue : public LVMessageValue
{
public:
    LVInt32MessageValue(int protobufId, int value);

public:
    int _value;    

    void* RawValue() override { return &_value; };
    size_t ByteSizeLong() override;
    google::protobuf::uint8* Serialize(google::protobuf::uint8* target, google::protobuf::io::EpsCopyOutputStream* stream) const override;
};

//---------------------------------------------------------------------
//---------------------------------------------------------------------
class LVFloatMessageValue : public LVMessageValue
{
public:
    LVFloatMessageValue(int protobufId, float value);

public:
    float _value;    

    void* RawValue() override { return &_value; };
    size_t ByteSizeLong() override;
    google::protobuf::uint8* Serialize(google::protobuf::uint8* target, google::protobuf::io::EpsCopyOutputStream* stream) const override;
};

//---------------------------------------------------------------------
//---------------------------------------------------------------------
class LVDoubleMessageValue : public LVMessageValue
{
public:
    LVDoubleMessageValue(int protobufId, double value);

public:
    double _value;    

    void* RawValue() override { return &_value; };
    size_t ByteSizeLong() override;
    google::protobuf::uint8* Serialize(google::protobuf::uint8* target, google::protobuf::io::EpsCopyOutputStream* stream) const override;
};

//---------------------------------------------------------------------
//---------------------------------------------------------------------
class LVMessage : public google::protobuf::Message
{
public:
    LVMessage(LVMessageMetadataList& metadata);

    ~LVMessage();

    google::protobuf::Message* New() const final; 
    void SharedCtor();
    void SharedDtor();
    void ArenaDtor(void* object);
    void RegisterArenaDtor(google::protobuf::Arena*);

    void Clear()  final;
    bool IsInitialized() const final;

    const char* _InternalParse(const char *ptr, google::protobuf::internal::ParseContext *ctx)  override;
    google::protobuf::uint8 *_InternalSerialize(google::protobuf::uint8 *target, google::protobuf::io::EpsCopyOutputStream *stream) const override;
    void SetCachedSize(int size) const final;
    int GetCachedSize(void) const final;
    size_t ByteSizeLong() const final;
    
    void MergeFrom(const google::protobuf::Message &from) final;
    void MergeFrom(const LVMessage &from);
    void CopyFrom(const google::protobuf::Message &from) final;
    void CopyFrom(const LVMessage &from);
    void InternalSwap(LVMessage *other);
    google::protobuf::Metadata GetMetadata() const final;

public:
    std::vector<shared_ptr<LVMessageValue>> _values;
    LVMessageMetadataList& _metadata;

private:
    mutable google::protobuf::internal::CachedSize _cached_size_;
};

//---------------------------------------------------------------------
//---------------------------------------------------------------------
class ServerStartEventData : public EventData
{
public:
    ServerStartEventData();

public:
    int serverStartStatus;
};


//---------------------------------------------------------------------
//---------------------------------------------------------------------
class GenericMethodData : public EventData
{
public:
    GenericMethodData(CallData* call, ServerContext* context, std::shared_ptr<LVMessage> request, std::shared_ptr<LVMessage> response);

public:
    CallData* _call;
    std::shared_ptr<LVMessage> _request;
    std::shared_ptr<LVMessage> _response;
};

//---------------------------------------------------------------------
//---------------------------------------------------------------------
struct LVEventData
{
    LVUserEventRef event;
    string requestMetadataName;
    string responseMetadataName;
};

//---------------------------------------------------------------------
//---------------------------------------------------------------------
class LabVIEWgRPCServer
{
public:
    int Run(string address, string serverCertificatePath, string serverKeyPath);
    void StopServer();
    void RegisterMetadata(std::shared_ptr<MessageMetadata> requestMetadata);
    void RegisterEvent(string eventName, LVUserEventRef reference, string requestMessageName, string responseMessageName);
    void SendEvent(string name, EventData* data);

    bool FindEventData(string name, LVEventData& data);
    shared_ptr<MessageMetadata> FindMetadata(const string& name);

private:
    mutex _mutex;
    unique_ptr<Server> _server;
    map<string, LVEventData> _registeredServerMethods;
    map<string, shared_ptr<MessageMetadata>> _registeredMessageMetadata;
    unique_ptr<grpc::AsyncGenericService> _rpcService;
    std::future<void> _runFuture;

private:
    void RunServer(string address, string serverCertificatePath, string serverKeyPath, ServerStartEventData* serverStarted);
    void HandleRpcs(grpc::ServerCompletionQueue *cq);
};

//---------------------------------------------------------------------
//---------------------------------------------------------------------
class Semaphore
{
public:
    Semaphore (int count_ = 1)
        : count(count_)
    {        
    }

    inline void notify()
    {
        std::unique_lock<std::mutex> lock(mtx);
        count++;
        cv.notify_one();
    }

    inline void wait()
    {
        std::unique_lock<std::mutex> lock(mtx);

        while(count == 0)
        {
            cv.wait(lock);
        }
        count--;
    }

private:
    std::mutex mtx;
    std::condition_variable cv;
    int count;
};

//---------------------------------------------------------------------
//---------------------------------------------------------------------
class CallData
{
public:
    CallData(LabVIEWgRPCServer* server, grpc::AsyncGenericService* service, grpc::ServerCompletionQueue* cq);
    void Proceed();
    void Write();
    void Finish();

private:
    bool ParseFromByteBuffer(const grpc::ByteBuffer& buffer, grpc::protobuf::Message& message);
    std::unique_ptr<grpc::ByteBuffer> SerializeToByteBuffer(const grpc::protobuf::Message& message);

private:
    LabVIEWgRPCServer* _server;
    grpc::AsyncGenericService* _service;
    grpc::ServerCompletionQueue* _cq;
    grpc::GenericServerContext _ctx;
    grpc::GenericServerAsyncReaderWriter _stream;
    grpc::ByteBuffer _rb;

    Semaphore _writeSemaphore;
    std::shared_ptr<GenericMethodData> _methodData;
    std::shared_ptr<LVMessage> _request;
    std::shared_ptr<LVMessage> _response;


    enum class CallStatus
    {
        Create,
        Read,
        Writing,
        Process,
        Finish
    };
    CallStatus _status;
};

//---------------------------------------------------------------------
//---------------------------------------------------------------------
struct LVRegistrationRequest
{
    LStrHandle eventName;
};

//---------------------------------------------------------------------
//---------------------------------------------------------------------
struct LVServerEvent
{
    LStrHandle eventData;
    int32_t serverId;
    int32_t status;
};

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void OccurServerEvent(LVUserEventRef event, EventData* data);
