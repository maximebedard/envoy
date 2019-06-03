#pragma once

#include <list>
#include <memory>
#include <string>
#include <vector>
#include <map>

#include "envoy/buffer/buffer.h"
#include "common/common/macros.h"
#include "common/common/assert.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MemcachedProxy {

class Message {
public:
  virtual ~Message() = default;

#define OP_CODE_ENUM_VALUES(FUNC) \
  FUNC(GET, 0x00) \
  FUNC(GETQ, 0x09) \
  FUNC(GETK, 0x0c) \
  FUNC(GETKQ, 0x0d) \
  FUNC(DELETE, 0x04) \
  FUNC(DELETEQ, 0x14) \
  FUNC(SET, 0x01) \
  FUNC(SETQ, 0x11) \
  FUNC(ADD, 0x02) \
  FUNC(ADDQ, 0x12) \
  FUNC(REPLACE, 0x03) \
  FUNC(REPLACEQ, 0x13) \
  FUNC(INCREMENT, 0x05) \
  FUNC(INCREMENTQ, 0x15) \
  FUNC(DECREMENT, 0x06) \
  FUNC(DECREMENTQ, 0x16) \
  FUNC(APPEND, 0x0e) \
  FUNC(APPENDQ, 0x19) \
  FUNC(PREPEND, 0x0f) \
  FUNC(PREPENDQ, 0x1a) \
  FUNC(VERSION, 0x0b)

#define TYPE_ENUM_VALUES(FUNC) \
  FUNC(V1_REQUEST, 0x80) \
  FUNC(V1_RESPONSE, 0x81)

#define DECLARE_ENUM(name, value) name = value,
  enum class OpCode { OP_CODE_ENUM_VALUES(DECLARE_ENUM) };

  // TODO: maybe break responses into a different struct???
  enum class Type { TYPE_ENUM_VALUES(DECLARE_ENUM) };
#undef DECLARE_ENUM

  constexpr static uint8_t HeaderLength = 24;

  virtual Type type() const PURE;
  virtual OpCode opCode() const PURE;
  virtual uint8_t dataType() const PURE;
  virtual uint8_t vbucketIdOrStatus() const PURE;
  virtual uint32_t opaque() const PURE;
  virtual uint64_t cas() const PURE;

  virtual const std::string& extras() const PURE; // TODO: tagged union if we ever need to be aware of the flags???
  virtual const std::string& body() const PURE;

  virtual const std::string& key() const PURE;
  virtual void key(const std::string& key) PURE;
  virtual bool quiet() const PURE;
  virtual void quiet(bool quiet) PURE;

  virtual bool operator==(const Message& rhs) const PURE;

//   // TODO: fix offset error :(
  static const std::string& opCodeName(OpCode op_code) {
    return opCodeNames().at(static_cast<uint8_t>(op_code));
  }

//   static const std::string& typeName(Type type) {
//     size_t i = static_cast<size_t>(type);
//     ASSERT(i < typeNames().size());
//     return typeNames()[i];
//   }

private:
#define DECLARE_STRING(K, V) std::make_pair(V, #K),
  typedef std::map<uint8_t, std::string> OpCodeNames;
  static const OpCodeNames& opCodeNames() {
    CONSTRUCT_ON_FIRST_USE(OpCodeNames, {OP_CODE_ENUM_VALUES(DECLARE_STRING)});
  }
#undef DECLARE_STRING

//   static const std::vector<std::string>& typeNames() {
//     CONSTRUCT_ON_FIRST_USE(std::vector<std::string>, {TYPE_ENUM_VALUES(DECLARE_STRING)});
//   }
// #undef DECLARE_STRINGS
};

typedef std::unique_ptr<Message> MessagePtr;

/**
 * General callbacks for dispatching decoded memcached messages to a sink.
 */
class DecoderCallbacks {
public:
  virtual ~DecoderCallbacks() = default;

  virtual void decodeMessage(MessagePtr&& message) PURE;
};

/**
 * Memcached message decoder.
 */
class Decoder {
public:
  virtual ~Decoder() = default;

  virtual void onData(Buffer::Instance& data) PURE;
};

typedef std::unique_ptr<Decoder> DecoderPtr;

/**
 * A factory for a memcached decoder.
 */
class DecoderFactory {
public:
  virtual ~DecoderFactory() = default;

  /**
   * Create a decoder given a set of decoder callbacks.
   */
  virtual DecoderPtr create(DecoderCallbacks& callbacks) PURE;
};

/**
 * Memcached message encoder.
 */
class Encoder {
public:
  virtual ~Encoder() = default;

  virtual void encodeMessage(const Message& message, Buffer::Instance& out) PURE;
};

typedef std::unique_ptr<Encoder> EncoderPtr;

/**
 * A redis protocol error.
 */
class ProtocolError : public EnvoyException {
public:
  ProtocolError(const std::string& error) : EnvoyException(error) {}
};

} // MemcachedProxy
} // NetworkFilters
} // Extensions
} // Envoy
