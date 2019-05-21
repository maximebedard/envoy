#pragma once

#include <list>
#include <memory>
#include <string>
#include <vector>

#include "envoy/buffer/buffer.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MemcachedProxy {


enum class OpCode {
  OP_GET = 0x00,
  OP_GETQ = 0x09,
  OP_GETK = 0x0c,
  OP_GETKQ = 0x0d,
  OP_DELETE = 0x04,
  OP_DELETEQ = 0x14,
  OP_SET = 0x01,
  OP_SETQ = 0x11,
  OP_ADD = 0x02,
  OP_ADDQ = 0x12,
  OP_REPLACE = 0x03,
  OP_REPLACEQ = 0x13,
  OP_INCREMENT = 0x05,
  OP_INCREMENTQ = 0x15,
  OP_DECREMENT = 0x06,
  OP_DECREMENTQ = 0x16,
  OP_APPEND = 0x0e,
  OP_APPENDQ = 0x19,
  OP_PREPEND = 0x0f,
  OP_PREPENDQ = 0x1a,
  OP_VERSION = 0x0b,
};

enum class Type {
  V1_REQUEST = 0x80,
  V1_RESPONSE = 0x81,
};

/*
 * Base class for all memcached messages
 */
// class Message {
// public:
//   virtual ~Message() = default;

//   constexpr static uint8_t HeaderLength = 24;

//   // virtual uint8_t dataType() const PURE;
//   // virtual uint8_t vbucketIdOrStatus() const PURE;
//   // virtual uint32_t opaque() const PURE;
//   // virtual uint64_t cas() const PURE;

//   // TODO: double dispatch encoding. Will enable support for binary encoder + ascii encoder.
//   // void encode(Encoder& encoder, Buffer::Instance& out);
// };

class Command {
public:
  virtual ~Command() = default;

  virtual bool quiet() const PURE;
  // virtual void setQuiet(bool quiet);
  virtual const std::string key() const PURE;
};

/**
 * Base class for all get like requests (GET, GETK)
 */
class GetLikeRequest : public virtual Command {
public:
  virtual ~GetLikeRequest() = default;
};

/**
 * Memcached OP_GET message.
 */
class GetRequest : public virtual GetLikeRequest {
public:
  virtual ~GetRequest() = default;
  virtual bool operator==(const GetRequest& rhs) const PURE;
};

typedef std::unique_ptr<GetRequest> GetRequestPtr;

/**
 * Memcached OP_GETK message.
 */
class GetkRequest : public virtual GetLikeRequest {
public:
  virtual ~GetkRequest() = default;
  virtual bool operator==(const GetkRequest& rhs) const PURE;
};

typedef std::unique_ptr<GetkRequest> GetkRequestPtr;

/**
 * Memcached OP_DELETE message.
 */
class DeleteRequest : public virtual GetLikeRequest {
public:
  virtual ~DeleteRequest() = default;
  virtual bool operator==(const DeleteRequest& rhs) const PURE;
};

typedef std::unique_ptr<DeleteRequest> DeleteRequestPtr;

/**
 * Base class for all set like requests (SET, ADD, REPLACE)
 */
class SetLikeRequest : public virtual Command {
public:
  virtual ~SetLikeRequest() = default;
  virtual const std::string& body() const PURE;
  virtual uint32_t flags() const PURE;
  virtual uint32_t expiration() const PURE;
};

/**
 * Memcached OP_SET message.
 */
class SetRequest : public virtual SetLikeRequest {
public:
  virtual ~SetRequest() = default;
  virtual bool operator==(const SetRequest& rhs) const PURE;
};

typedef std::unique_ptr<SetRequest> SetRequestPtr;

/**
 * Memcached OP_ADD message.
 */
class AddRequest : public virtual SetLikeRequest {
public:
  virtual ~AddRequest() = default;
  virtual bool operator==(const AddRequest& rhs) const PURE;
};

typedef std::unique_ptr<AddRequest> AddRequestPtr;

/**
 * Memcached OP_REPLACE message.
 */
class ReplaceRequest : public virtual SetLikeRequest {
public:
  virtual ~ReplaceRequest() = default;
  virtual bool operator==(const ReplaceRequest& rhs) const PURE;
};

typedef std::unique_ptr<ReplaceRequest> ReplaceRequestPtr;

/**
 * Base class for all counter like requests (INCREMENT, DECREMENT)
 */
class CounterLikeRequest : public virtual Command {
public:
  virtual ~CounterLikeRequest() = default;
  virtual uint64_t amount() const PURE;
  virtual uint64_t initialValue() const PURE;
  virtual uint32_t expiration() const PURE;
};

/**
 * Memcached OP_INCREMENT message.
 */
class IncrementRequest : public virtual CounterLikeRequest {
public:
  virtual ~IncrementRequest() = default;
  virtual bool operator==(const IncrementRequest& rhs) const PURE;
};

typedef std::unique_ptr<IncrementRequest> IncrementRequestPtr;

/**
 * Memcached OP_DECREMENT message.
 */
class DecrementRequest : public virtual CounterLikeRequest {
public:
  virtual ~DecrementRequest() = default;
  virtual bool operator==(const DecrementRequest& rhs) const PURE;
};

typedef std::unique_ptr<DecrementRequest> DecrementRequestPtr;

/**
 * Base class for all append like requests (APPEND, PREPEND)
 */
class AppendLikeRequest : public virtual Command {
public:
  virtual ~AppendLikeRequest() = default;
  virtual const std::string& body() const PURE;
};

/**
 * Memcached OP_APPEND message.
 */
class AppendRequest : public virtual AppendLikeRequest {
public:
  virtual ~AppendRequest() = default;
  virtual bool operator==(const AppendRequest& rhs) const PURE;
};

typedef std::unique_ptr<AppendRequest> AppendRequestPtr;

/**
 * Memcached OP_PREPEND message.
 */
class PrependRequest : public virtual AppendLikeRequest {
public:
  virtual ~PrependRequest() = default;
  virtual bool operator==(const PrependRequest& rhs) const PURE;
};

typedef std::unique_ptr<PrependRequest> PrependRequestPtr;

/**
 * Memcached OP_VERSION message.
 */
class VersionRequest {
public:
  virtual ~VersionRequest() = default;
  virtual bool operator==(const VersionRequest& rhs) const PURE;
};

typedef std::unique_ptr<VersionRequest> VersionRequestPtr;

/**
 * Memcached OP_NOOP message.
 */
class NoopRequest {
public:
  virtual ~NoopRequest() = default;
  virtual bool operator==(const NoopRequest& rhs) const PURE;
};

typedef std::unique_ptr<NoopRequest> NoopRequestPtr;

/**
 * Memcached REPLY message.
 */
class Reply {
public:
  virtual ~Reply() = default;
  virtual bool operator==(const Reply& rhs) const PURE;
};

typedef std::unique_ptr<Reply> ReplyPtr;

/**
 * General callbacks for dispatching decoded memcached messages to a sink.
 */
class DecoderCallbacks {
public:
  virtual ~DecoderCallbacks() = default;

  // TODO: collapse GET, GETK, DELETE, SET, ADD, REPLACE, INCR, DECR, APPEND, PREPEND into a command structure??.
  virtual void decodeGet(GetRequestPtr&& message) PURE;
  virtual void decodeGetk(GetkRequestPtr&& message) PURE;
  virtual void decodeDelete(DeleteRequestPtr&& message) PURE;
  virtual void decodeSet(SetRequestPtr&& message) PURE;
  virtual void decodeAdd(AddRequestPtr&& message) PURE;
  virtual void decodeReplace(ReplaceRequestPtr&& message) PURE;
  virtual void decodeIncrement(IncrementRequestPtr&& message) PURE;
  virtual void decodeDecrement(DecrementRequestPtr&& message) PURE;
  virtual void decodeAppend(AppendRequestPtr&& message) PURE;
  virtual void decodePrepend(PrependRequestPtr&& message) PURE;
  virtual void decodeVersion(VersionRequestPtr&& message) PURE;
  virtual void decodeNoop(NoopRequestPtr&& message) PURE;
  virtual void decodeReply(ReplyPtr&& message) PURE;
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
  virtual ~DecoderFactory() {}

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

  virtual void encodeGet(const GetRequest& message, Buffer::Instance& out) PURE;
  virtual void encodeGetk(const GetkRequest& message, Buffer::Instance& out) PURE;
  virtual void encodeDelete(const DeleteRequest& message, Buffer::Instance& out) PURE;
  virtual void encodeSet(const SetRequest& message, Buffer::Instance& out) PURE;
  virtual void encodeAdd(const AddRequest& message, Buffer::Instance& out) PURE;
  virtual void encodeReplace(const ReplaceRequest& message, Buffer::Instance& out) PURE;
  virtual void encodeIncrement(const IncrementRequest& message, Buffer::Instance& out) PURE;
  virtual void encodeDecrement(const DecrementRequest& message, Buffer::Instance& out) PURE;
  virtual void encodeAppend(const AppendRequest& message, Buffer::Instance& out) PURE;
  virtual void encodePrepend(const PrependRequest& message, Buffer::Instance& out) PURE;
  virtual void encodeVersion(const VersionRequest& message, Buffer::Instance& out) PURE;
  virtual void encodeNoop(const NoopRequest& message, Buffer::Instance& out) PURE;
  virtual void encodeReply(const Reply& message, Buffer::Instance& out) PURE;
};

typedef std::unique_ptr<Encoder> EncoderPtr;

} // MemcachedProxy
} // NetworkFilters
} // Extensions
} // Envoy
