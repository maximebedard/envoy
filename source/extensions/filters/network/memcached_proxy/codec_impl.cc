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
  if (length > 0) {
    char* start = reinterpret_cast<char*>(data.linearize(length));
    std::string ret(start, length);
    data.drain(length);
    return ret;
  }
  return "";
}

bool MessageImpl::operator==(const Message& rhs) const {
  return type() == rhs.type() &&
    opCode() == rhs.opCode() &&
    dataType() == rhs.dataType() &&
    vbucketIdOrStatus() == rhs.vbucketIdOrStatus() &&
    opaque() == rhs.opaque() &&
    cas() == rhs.cas() &&
    key() == rhs.key() &&
    extras() == rhs.extras() &&
    body() == rhs.body();
}

bool MessageImpl::quiet() const {
  switch (op_code_) {
  case Message::OpCode::GETQ:
  case Message::OpCode::GETKQ:
  case Message::OpCode::DELETEQ:
  case Message::OpCode::SETQ:
  case Message::OpCode::ADDQ:
  case Message::OpCode::REPLACEQ:
  case Message::OpCode::INCREMENTQ:
  case Message::OpCode::DECREMENTQ:
  case Message::OpCode::APPENDQ:
  case Message::OpCode::PREPENDQ:
    return true;
  default:
    return false;
  }
}

void MessageImpl::quiet(bool) {
  // todo
}

bool DecoderImpl::decode(Buffer::Instance& data) {
  ENVOY_LOG(trace, "decoding {} bytes", data.length());
  if (data.length() < Message::HeaderLength) {
    return false;
  }

  // TODO: make sure casting are safe.
  auto type = static_cast<Message::Type>(data.drainBEInt<uint8_t>());
  auto op_code = static_cast<Message::OpCode>(data.drainBEInt<uint8_t>());
  auto key_length = data.drainBEInt<uint16_t>();
  auto extras_length = data.drainBEInt<uint8_t>();
  auto data_type = data.drainBEInt<uint8_t>();
  auto vbucket_id_or_status = data.drainBEInt<uint16_t>();
  auto body_length = data.drainBEInt<uint32_t>();
  auto opaque = data.drainBEInt<uint32_t>();
  auto cas = data.drainBEInt<uint64_t>();

  body_length -= (key_length + extras_length);

  auto extras = BufferHelper::drainString(data, extras_length);
  auto key = BufferHelper::drainString(data, key_length);
  auto body = BufferHelper::drainString(data, body_length);

  auto message = std::make_unique<MessageImpl>(type, op_code, data_type, vbucket_id_or_status, opaque, cas, key, extras, body);
  ENVOY_LOG(info, "decoded {} {} key={}, cas={}, body={}",
    message->type() == Message::Type::V1_RESPONSE ? "<" : ">",
    Message::opCodeName(message->opCode()),
    message->key().empty() ? "N/A" : message->key(),
    message->cas(),
    message->body().empty() ? "N/A" : message->body());
  callbacks_.decodeMessage(std::move(message));

  ENVOY_LOG(trace, "{} bytes remaining after decoding", data.length());
  return true;
}

void DecoderImpl::onData(Buffer::Instance& data) {
  while (data.length() > 0 && decode(data)) {}
}

void BinaryEncoderImpl::encodeMessage(const Message& message, Buffer::Instance& out) {
  out.writeByte(static_cast<uint8_t>(message.type()));
  out.writeByte(static_cast<uint8_t>(message.opCode()));
  out.writeBEInt<uint16_t>(message.key().length());
  out.writeByte(message.extras().length());
  out.writeByte(message.dataType());
  out.writeBEInt<uint16_t>(message.vbucketIdOrStatus());
  out.writeBEInt<uint32_t>(message.key().length() + message.extras().length() + message.body().length());
  out.writeBEInt<uint32_t>(message.opaque());
  out.writeBEInt<uint64_t>(message.cas());
  out.add(message.extras());
  out.add(message.key());
  out.add(message.body());
}

}
}
}
}
