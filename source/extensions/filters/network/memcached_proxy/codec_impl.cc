#include "extensions/filters/network/memcached_proxy/codec_impl.h"

#include <cstdint>
#include <list>
#include <memory>
#include <sstream>
#include <string>

#include "envoy/buffer/buffer.h"
#include "envoy/common/exception.h"

#include "common/common/assert.h"
#include "common/common/fmt.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MemcachedProxy {

std::string BufferHelper::drainString(Buffer::Instance& data, uint32_t length) {
  char* start = reinterpret_cast<char*>(data.linearize(length));
  std::string ret(start, length);
  data.drain(length);
  return ret;
}

bool MessageImpl::equals(const Message& rhs) const {
  return dataType() == rhs.dataType() &&
    vbucketIdOrStatus() == rhs.vbucketIdOrStatus() &&
    opaque() == rhs.opaque() &&
    cas() == rhs.cas();
}

bool GetLikeRequestImpl::equals(const GetLikeRequest& rhs) const {
  return MessageImpl::equals(rhs) &&
    key() == rhs.key();
}

bool SetLikeRequestImpl::equals(const SetLikeRequest& rhs) const {
  return MessageImpl::equals(rhs) &&
    key() == rhs.key() &&
    body() == rhs.body() &&
    expiration() == rhs.expiration() &&
    flags() == rhs.flags();
}

bool CounterLikeRequestImpl::equals(const CounterLikeRequest& rhs) const {
  return MessageImpl::equals(rhs) &&
    key() == rhs.key() &&
    amount() == rhs.amount() &&
    initialValue() == rhs.initialValue() &&
    expiration() == rhs.expiration();
}

bool AppendLikeRequestImpl::equals(const AppendLikeRequest& rhs) const {
  return MessageImpl::equals(rhs) &&
    key() == rhs.key() &&
    body() == rhs.body();
}

bool DecoderImpl::decode(Buffer::Instance& data) {
  ENVOY_LOG(info, "decoding {} bytes", data.length());
  if (data.length() < Message::HeaderLength) {
    return false;
  }

  auto type = static_cast<Type>(data.drainBEInt<uint8_t>());
  auto op_code = static_cast<OpCode>(data.drainBEInt<uint8_t>());
  auto key_length = data.drainBEInt<uint16_t>();
  auto extras_length = data.drainBEInt<uint8_t>();
  auto data_type = data.drainBEInt<uint8_t>();
  auto vbucket_id_or_status = data.drainBEInt<uint16_t>();
  auto body_length = data.drainBEInt<uint32_t>();
  auto opaque = data.drainBEInt<uint32_t>();
  auto cas = data.drainBEInt<uint64_t>();

  body_length -= (key_length + extras_length);

  switch (type) {
  case Message::Type::REQUEST: {

    switch (op_code) {
    case OpCode::OP_GET:
    case OpCode::OP_GETQ: {
      auto message = decodeGetLike<GetRequestImpl>(data_type, vbucket_id_or_status, opaque, cas, key_length, extras_length, body_length, data);
      ENVOY_LOG(info, "decoded `GET` key={}", message->key());
      callbacks_.decodeGet(std::move(message));
      break;
    }

    case OpCode::OP_GETK:
    case OpCode::OP_GETKQ: {
      auto message = decodeGetLike<GetkRequestImpl>(data_type, vbucket_id_or_status, opaque, cas, key_length, extras_length, body_length, data);
      ENVOY_LOG(info, "decoded `GETK` key={}", message->key());
      callbacks_.decodeGetk(std::move(message));
      break;
    }

    case OpCode::OP_DELETE:
    case OpCode::OP_DELETEQ: {
      auto message = decodeGetLike<DeleteRequestImpl>(data_type, vbucket_id_or_status, opaque, cas, key_length, extras_length, body_length, data);
      ENVOY_LOG(info, "decoded `DELETE` key={}", message->key());
      callbacks_.decodeDelete(std::move(message));
      break;
    }

    case OpCode::OP_SET:
    case OpCode::OP_SETQ: {
      auto message = decodeSetLike<SetRequestImpl>(data_type, vbucket_id_or_status, opaque, cas, key_length, extras_length, body_length, data);
      ENVOY_LOG(info, "decoded `SET` key={}, body={}", message->key(), message->body());
      callbacks_.decodeSet(std::move(message));
      break;
    }

    case OpCode::OP_ADD:
    case OpCode::OP_ADDQ: {
      auto message = decodeSetLike<AddRequestImpl>(data_type, vbucket_id_or_status, opaque, cas, key_length, extras_length, body_length, data);
      ENVOY_LOG(info, "decoded `ADD` key={}, body={}", message->key(), message->body());
      callbacks_.decodeAdd(std::move(message));
      break;
    }

    case OpCode::OP_REPLACE:
    case OpCode::OP_REPLACEQ: {
      auto message = decodeSetLike<ReplaceRequestImpl>(data_type, vbucket_id_or_status, opaque, cas, key_length, extras_length, body_length, data);
      ENVOY_LOG(info, "decoded `REPLACE` key={}, body={}", message->key(), message->body());
      callbacks_.decodeReplace(std::move(message));
      break;
    }

    case OpCode::OP_INCREMENT:
    case OpCode::OP_INCREMENTQ: {
      auto message = decodeCounterLike<IncrementRequestImpl>(data_type, vbucket_id_or_status, opaque, cas, key_length, extras_length, body_length, data);
      ENVOY_LOG(info, "decoded `INCREMENT` key={}, amount={}, initial_value={}", message->key(), message->amount(), message->initialValue());
      callbacks_.decodeIncrement(std::move(message));
      break;
    }

    case OpCode::OP_DECREMENT:
    case OpCode::OP_DECREMENTQ: {
      auto message = decodeCounterLike<DecrementRequestImpl>(data_type, vbucket_id_or_status, opaque, cas, key_length, extras_length, body_length, data);
      ENVOY_LOG(info, "decoded `DECREMENT` key={}, amount={}, initial_value={}", message->key(), message->amount(), message->initialValue());
      callbacks_.decodeDecrement(std::move(message));
      break;
    }

    case OpCode::OP_APPEND:
    case OpCode::OP_APPENDQ: {
      auto message = decodeAppendLike<AppendRequestImpl>(data_type, vbucket_id_or_status, opaque, cas, key_length, extras_length, body_length, data);
      ENVOY_LOG(info, "decoded `APPEND` key={}, body={}", message->key(), message->body());
      callbacks_.decodeAppend(std::move(message));
      break;
    }

    case OpCode::OP_PREPEND:
    case OpCode::OP_PREPENDQ: {
      auto message = decodeAppendLike<PrependRequestImpl>(data_type, vbucket_id_or_status, opaque, cas, key_length, extras_length, body_length, data);
      message->fromBuffer(key_length, extras_length, body_length, data);
      ENVOY_LOG(info, "decoded `PREPEND` key={}, body={}", message->key(), message->body());
      callbacks_.decodePrepend(std::move(message));
      break;
    }

    case OpCode::OP_VERSION: {
      auto message = std::make_unique<VersionRequestImpl>(data_type, vbucket_id_or_status, opaque, cas);
      ENVOY_LOG(info, "decoded `VERSION`");
      callbacks_.decodeVersion(std::move(message));
      break;
    }

    case OpCode::OP_NOOP: {
      auto message = std::make_unique<NoopRequestImpl>(data_type, vbucket_id_or_status, opaque, cas);
      ENVOY_LOG(info, "decoded `NOOP`");
      callbacks_.decodeNoop(std::move(message));
      break;
    }

    default:
      throw EnvoyException(fmt::format("invalid memcached op {}", static_cast<uint8_t>(op_code)));
    }
    }

    break;
  }

  case Message::Type::RESPONSE: {
    // TODO: parse generic response
    data.drain();
    break;
  }

  default:
    throw EnvoyException(fmt::format("invalid memcached type {}", static_cast<uint8_t>(type)));
  }
  }

  ENVOY_LOG(info, "{} bytes remaining after decoding", data.length());
  return true;
}

template<class Message>
std::unique_ptr<Message> decodeGetLike() {
  auto key = BufferHelper::drainString(data, key_length);
  return std::make_unique<Message>(data_type, vbucket_id_or_status, opaque, cas, key);
}

template<class Message>
std::unique_ptr<Message> decodeSetLike() {
  auto flags = data.drainBEInt<uint32_t>();
  auto expiration = data.drainBEInt<uint32_t>();
  auto key = BufferHelper::drainString(data, key_length);
  auto body = BufferHelper::drainString(data, body_length);
  return std::make_unique<Message>(data_type, vbucket_id_or_status, opaque, cas, flags, expiration, key, body);
}

template<class Message>
std::unique_ptr<Message> decodeCounterLike() {
  auto amount = data.drainBEInt<uint64_t>();
  auto initial_value = data.drainBEInt<uint64_t>();
  auto expiration = data.drainBEInt<uint32_t>();
  auto key = BufferHelper::drainString(data, key_length);
  return std::make_unique<Message>(data_type, vbucket_id_or_status, opaque, cas, amount, initial_value, expiration, key);
}

template<class Message>
std::unique_ptr<Message> decodeAppendLike() {
  auto key = BufferHelper::drainString(data, key_length);
  auto body = BufferHelper::drainString(data, body_length);
  return std::make_unique<Message>(data_type, vbucket_id_or_status, opaque, cas, key, body);
}

void DecoderImpl::onData(Buffer::Instance& data) {
  while (data.length() > 0 && decode(data)) {}
}

void EncoderImpl::encodeRequestHeader(
  uint16_t key_length,
  uint8_t extras_length,
  uint32_t body_length,
  const MessageImpl& message,
  OpCode op_code,
  Buffer::Instance& out) {
  out.writeByte(Message::RequestV1);
  out.writeByte(op_code);
  out.writeBEInt<uint16_t>(key_length);
  out.writeByte(extras_length);
  out.writeByte(message.dataType());
  out.writeBEInt<uint16_t>(message.vbucketIdOrStatus());
  out.writeBEInt<uint32_t>(body_length);
  out.writeBEInt<uint32_t>(message.opaque());
  out.writeBEInt<uint64_t>(message.cas());
}

void EncoderImpl::encodeGet(const GetRequest& request, Buffer::Instance& out) {
  encodeGetLike(request, request.quiet() ? OpCode::OP_GETQ : OpCode::OP_GET, out);
}

void EncoderImpl::encodeGetk(const GetkRequest& request, Buffer::Instance& out) {
  encodeGetLike(request, request.quiet() ? OpCode::OP_GETKQ : OpCode::OP_GETK, out);
}

void EncoderImpl::encodeDelete(const DeleteRequest& request, Buffer::Instance& out) {
  encodeGetLike(request, request.quiet() ? OpCode::OP_DELETEQ : OpCode::OP_DELETE, out);
}

void EncoderImpl::encodeGetLike(const GetLikeRequest& request, OpCode op_code, Buffer::Instance& out) {
  encodeRequestHeader(request.key().length(), 0, 0, request, op_code, out);
  out.add(request.key());
}

void EncoderImpl::encodeSet(const SetRequest& request, Buffer::Instance& out) {
  encodeSetLike(request, request.quiet() ? OpCode::OP_SETQ : OpCode::OP_SET, out);
}

void EncoderImpl::encodeAdd(const AddRequest& request, Buffer::Instance& out) {
  encodeSetLike(request, request.quiet() ? OpCode::OP_ADDQ : OpCode::OP_ADD, out);
}

void EncoderImpl::encodeReplace(const ReplaceRequest& request, Buffer::Instance& out) {
  encodeSetLike(request, request.quiet() ? OpCode::OP_REPLACEQ : OpCode::OP_REPLACE, out);
}

void EncoderImpl::encodeSetLike(const SetLikeRequest& request, OpCode op_code, Buffer::Instance& out) {
  encodeRequestHeader(request.key().length(), 8, request.body().length(), request, op_code, out);
  out.writeBEInt<uint32_t>(request.flags());
  out.writeBEInt<uint32_t>(request.expiration());
  out.add(request.key());
  out.add(request.body());
}

void EncoderImpl::encodeIncrement(const IncrementRequest& request, Buffer::Instance& out) {
  encodeCounterLike(request, request.quiet() ? OpCode::OP_INCREMENTQ : OpCode::OP_INCREMENT, out);
}

void EncoderImpl::encodeDecrement(const DecrementRequest& request, Buffer::Instance& out) {
  encodeCounterLike(request, request.quiet() ? OpCode::OP_DECREMENTQ : OpCode::OP_DECREMENT, out);
}

void EncoderImpl::encodeCounterLike(const CounterLikeRequest& request, OpCode op_code, Buffer::Instance& out) {
  encodeRequestHeader(request.key().length(), 8, 0, request, op_code, out);
  out.writeBEInt<uint64_t>(request.amount());
  out.writeBEInt<uint64_t>(request.initialValue());
  out.writeBEInt<uint32_t>(request.expiration());
  out.add(request.key());
}

void EncoderImpl::encodeAppend(const AppendRequest& request, Buffer::Instance& out) {
  encodeAppendLike(request, request.quiet() ? OpCode::OP_APPENDQ : OpCode::OP_APPEND, out);
}

void EncoderImpl::encodePrepend(const PrependRequest& request, Buffer::Instance& out) {
  encodeAppendLike(request, request.quiet() ? OpCode::OP_PREPENDQ : OpCode::OP_PREPEND, out);
}

void EncoderImpl::encodeAppendLike(const AppendLikeRequest& request, OpCode op_code, Buffer::Instance& out) {
  encodeRequestHeader(request.key().length(), 0, request.body().length(), request, op_code, out);
  out.add(request.key());
  out.add(request.body());
}

void EncoderImpl::encodeVersion(const VersionRequest& request, Buffer::Instance& out) {
  encodeRequestHeader(0, 0, 0, request, OpCode::OP_VERSION, out);
}

}
}
}
}
