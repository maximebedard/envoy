#pragma once

#include <cstdint>
#include <list>
#include <string>
#include <vector>

#include "common/common/logger.h"
#include "common/buffer/buffer_impl.h"

#include "envoy/buffer/buffer.h"

#include "extensions/filters/network/memcached_proxy/codec.h"
#include <list>
#include <memory>
#include <string>
#include <vector>

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MemcachedProxy {

class BufferHelper {
public:
  static std::string drainString(Buffer::Instance& buffer, uint32_t size);
};

class MessageImpl {
public:
  MessageImpl(uint8_t data_type, uint8_t vbucket_id_or_status, uint32_t opaque, uint64_t cas) :
    data_type_(data_type), vbucket_id_or_status_(vbucket_id_or_status), opaque_(opaque), cas_(cas) {}

  uint8_t dataType() const { return data_type_; }
  uint8_t vbucketIdOrStatus() const { return vbucket_id_or_status_; }
  uint32_t opaque() const { return opaque_; }
  uint64_t cas() const { return cas_; }
protected:
  bool equals(const MessageImpl& rhs) const;
private:
  const uint8_t data_type_;
  const uint8_t vbucket_id_or_status_;
  const uint32_t opaque_;
  const uint64_t cas_;
};

class GetLikeRequestImpl : public MessageImpl,
                           public virtual Command,
                           public virtual GetLikeRequest {
public:

  // Command
  bool quiet() const override { return false; }
  const std::string& key() const override { return key_; }
protected:
  bool equals(const GetLikeRequest& rhs) const;
private:
  std::string key_;
  bool quiet_;
};

class GetRequestImpl : public GetLikeRequestImpl,
                       public GetRequest {
public:
  using GetLikeRequestImpl::GetLikeRequestImpl;

  // GetRequest
  bool operator==(const GetRequest& rhs) const override { return equals(rhs); }
};

class GetkRequestImpl : public GetLikeRequestImpl,
                        public GetkRequest {
public:
  using GetLikeRequestImpl::GetLikeRequestImpl;

  // GetkRequest
  bool operator==(const GetkRequest& rhs) const override { return equals(rhs); }
};

class DeleteRequestImpl : public GetLikeRequestImpl,
                          public DeleteRequest {
public:
  using GetLikeRequestImpl::GetLikeRequestImpl;

  // DeleteRequest
  bool operator==(const DeleteRequest& rhs) const override { return equals(rhs); }
};

class SetLikeRequestImpl : public MessageImpl,
                           public Command,
                           public virtual SetLikeRequest {
public:
  using MessageImpl::MessageImpl;

  // Command
  bool quiet() const override { return false; }
  const std::string& key() const override { return key_; }

  // SetLikeRequest
  const std::string& body() const override { return body_; }
  uint32_t flags() const override { return flags_; }
  uint32_t expiration() const override { return expiration_; }
protected:
  bool equals(const SetLikeRequest& rhs) const;
private:
  std::string key_;
  std::string body_;
  uint32_t flags_;
  uint32_t expiration_;
};

class SetRequestImpl : public SetLikeRequestImpl,
                       public SetRequest {
public:
  using SetLikeRequestImpl::SetLikeRequestImpl;

  // SetRequest
  bool operator==(const SetRequest& rhs) const override { return equals(rhs); }
};

class AddRequestImpl : public SetLikeRequestImpl,
                       public AddRequest {
public:
  using SetLikeRequestImpl::SetLikeRequestImpl;

  // AddRequest
  bool operator==(const AddRequest& rhs) const override { return equals(rhs); }
};

class ReplaceRequestImpl : public SetLikeRequestImpl,
                           public ReplaceRequest {
public:
  using SetLikeRequestImpl::SetLikeRequestImpl;

  // ReplaceRequest
  bool operator==(const ReplaceRequest& rhs) const override { return equals(rhs); }
};

class CounterLikeRequestImpl : public MessageImpl,
                               public virtual CounterLikeRequest {
public:
  using MessageImpl::MessageImpl;

  // Command
  bool quiet() const override { return false; }
  const std::string& key() const override { return key_; }

  // CounterLikeRequest
  uint64_t amount() const override { return amount_; }
  uint64_t initialValue() const override { return initial_value_; }
  uint32_t expiration() const override { return expiration_; }
protected:
  bool equals(const CounterLikeRequest& rhs) const;
private:
  std::string key_;
  uint64_t amount_;
  uint64_t initial_value_;
  uint32_t expiration_;
};

class IncrementRequestImpl : public CounterLikeRequestImpl,
                             public IncrementRequest {
public:
  using CounterLikeRequestImpl::CounterLikeRequestImpl;

  // IncrementRequest
  bool operator==(const IncrementRequest& rhs) const override { return equals(rhs); }
};

class DecrementRequestImpl : public CounterLikeRequestImpl,
                             public DecrementRequest {
public:
  using CounterLikeRequestImpl::CounterLikeRequestImpl;

  // DecrementRequest
  bool operator==(const DecrementRequest& rhs) const override { return equals(rhs); }
};

class DecoderImpl : public Decoder, Logger::Loggable<Logger::Id::memcached> {
public:
  DecoderImpl(DecoderCallbacks& callbacks) : callbacks_(callbacks) {}

  // Memcached::Decoder
  void onData(Buffer::Instance& data) override;
private:
  bool decode(Buffer::Instance& data);

  template<class Message>
  static std::unique_ptr<Message> decodeGetLike(uint8_t data_type, uint8_t vbucket_id_or_status, uint32_t opaque,
    uint64_t cas, uint16_t key_length, uint8_t extras_length, uint32_t body_length, Buffer::Instance& data);

  template<class Message>
  static std::unique_ptr<Message> decodeSetLike(uint8_t data_type, uint8_t vbucket_id_or_status, uint32_t opaque,
    uint64_t cas, uint16_t key_length, uint8_t extras_length, uint32_t body_length, Buffer::Instance& data);

  template<class Message>
  static std::unique_ptr<Message> decodeCounterLike(uint8_t data_type, uint8_t vbucket_id_or_status, uint32_t opaque,
    uint64_t cas, uint16_t key_length, uint8_t extras_length, uint32_t body_length, Buffer::Instance& data);

  template<class Message>
  static std::unique_ptr<Message> decodeAppendLike(uint8_t data_type, uint8_t vbucket_id_or_status, uint32_t opaque,
    uint64_t cas, uint16_t key_length, uint8_t extras_length, uint32_t body_length, Buffer::Instance& data);

  DecoderCallbacks& callbacks_;
};

class DecoderFactoryImpl : public DecoderFactory {
public:
  // DecoderFactory
  DecoderPtr create(DecoderCallbacks& callbacks) override {
    return std::make_unique<DecoderImpl>(callbacks);
  }
};

class AppendLikeRequestImpl : public MessageImpl,
                              public Command,
                              public virtual AppendLikeRequest {
public:
  using MessageImpl::MessageImpl;

  // Command
  bool quiet() const override { return false; }
  const std::string& key() const override { return key_; }

  // AppendLikeRequest
  const std::string& body() const override { return body_; }
  void key(const std::string& key) { key_ = key; }
  void body(const std::string& body) { body_ = body; }
protected:
  bool equals(const AppendLikeRequest& rhs) const;
private:
  std::string key_;
  std::string body_;
};

class AppendRequestImpl : public AppendLikeRequestImpl,
                          public AppendRequest {
public:
  using AppendLikeRequestImpl::AppendLikeRequestImpl;

  // AppendRequest
  bool operator==(const AppendRequest& rhs) const override { return equals(rhs); }
};

class PrependRequestImpl : public AppendLikeRequestImpl,
                           public PrependRequest {
public:
  using AppendLikeRequestImpl::AppendLikeRequestImpl;

  // PrependRequest
  bool operator==(const PrependRequest& rhs) const override { return equals(rhs); }
};

class VersionRequestImpl : public MessageImpl,
                           public virtual VersionRequest {
public:
  using MessageImpl::MessageImpl;

  // VersionRequest
  bool operator==(const VersionRequest& rhs) const override { return MessageImpl::equals(rhs); }
};

class NoopRequestImpl : public MessageImpl,
                           public virtual NoopRequest {
public:
  using MessageImpl::MessageImpl;

  // NoopRequest
  bool operator==(const NoopRequest& rhs) const override { return MessageImpl::equals(rhs); }
};

class ReplyImpl : public MessageImpl,
                  public virtual Reply {
public:
  using MessageImpl::MessageImpl;

  // VersionRequestImpl
  bool operator==(const Reply& rhs) const override { return MessageImpl::equals(rhs); }
};

class EncoderImpl : public Encoder, Logger::Loggable<Logger::Id::memcached> {
public:
  EncoderImpl() {}

  // Memcached::Encoder
  void encodeGet(const GetRequest& request, Buffer::Instance& out) override;
  void encodeGetk(const GetkRequest& request, Buffer::Instance& out) override;
  void encodeDelete(const DeleteRequest& request, Buffer::Instance& out) override;
  void encodeSet(const SetRequest& request, Buffer::Instance& out) override;
  void encodeAdd(const AddRequest& request, Buffer::Instance& out) override;
  void encodeReplace(const ReplaceRequest& request, Buffer::Instance& out) override;
  void encodeIncrement(const IncrementRequest& request, Buffer::Instance& out) override;
  void encodeDecrement(const DecrementRequest& request, Buffer::Instance& out) override;
  void encodeAppend(const AppendRequest& request, Buffer::Instance& out) override;
  void encodePrepend(const PrependRequest& request, Buffer::Instance& out) override;
  void encodeVersion(const VersionRequest& request, Buffer::Instance& out) override;
  void encodeNoop(const NoopRequest& request, Buffer::Instance& out) override;
private:
  void encodeGetLike(const GetLikeRequest& request, OpCode op_code, Buffer::Instance& out);
  void encodeSetLike(const SetLikeRequest& request, OpCode op_code, Buffer::Instance& out);
  void encodeCounterLike(const CounterLikeRequest& request, OpCode op_code, Buffer::Instance& out);
  void encodeAppendLike(const AppendLikeRequest& request, OpCode op_code, Buffer::Instance& out);
  void encodeRequestHeader(
    uint16_t key_length,
    uint8_t extras_length,
    uint32_t body_length,
    const MessageImpl& request,
    OpCode op,
    Buffer::Instance& out);
};

}
}
}
}
