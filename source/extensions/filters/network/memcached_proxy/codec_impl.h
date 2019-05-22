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

class MessageImpl : public virtual Message {
public:
  MessageImpl(Message::Type type, Message::OpCode op_code, uint8_t data_type, uint8_t vbucket_id_or_status, uint32_t opaque, uint64_t cas, const std::string& key, const std::string& extras, const std::string& body) :
    type_(type), op_code_(op_code), data_type_(data_type), vbucket_id_or_status_(vbucket_id_or_status), opaque_(opaque), cas_(cas), key_(key), extras_(extras), body_(body) {}

  Message::Type type() const override { return type_; }
  Message::OpCode opCode() const override { return op_code_; }
  uint8_t dataType() const override { return data_type_; }
  uint8_t vbucketIdOrStatus() const override { return vbucket_id_or_status_; }
  uint32_t opaque() const override { return opaque_; }
  uint64_t cas() const override { return cas_; }
  const std::string& key() const override { return key_; }
  void key(const std::string& key) override { key_ = key; }
  const std::string& extras() const override { return extras_; }
  const std::string& body() const override { return body_; }
  void quiet(bool quiet) override;
  bool quiet() const override;
  bool operator==(const Message& rhs) const override;
private:
  const Message::Type type_;
  Message::OpCode op_code_;
  const uint8_t data_type_;
  const uint8_t vbucket_id_or_status_;
  const uint32_t opaque_;
  const uint64_t cas_;
  std::string key_;
  const std::string extras_;
  const std::string body_;
};

class DecoderImpl : public Decoder, Logger::Loggable<Logger::Id::memcached> {
public:
  DecoderImpl(DecoderCallbacks& callbacks) : callbacks_(callbacks) {}

  // Memcached::Decoder
  void onData(Buffer::Instance& data) override;
private:
  bool decode(Buffer::Instance& data);

  DecoderCallbacks& callbacks_;
};

class DecoderFactoryImpl : public DecoderFactory {
public:
  // DecoderFactory
  DecoderPtr create(DecoderCallbacks& callbacks) override {
    return std::make_unique<DecoderImpl>(callbacks);
  }
};

class BinaryEncoderImpl : public Encoder, Logger::Loggable<Logger::Id::memcached> {
public:
  BinaryEncoderImpl() {}

  // Memcached::Encoder
  void encodeMessage(const Message& request, Buffer::Instance& out) override;
};

}
}
}
}
