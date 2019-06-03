#pragma once

#include <cstdint>
#include <list>
#include <string>

#include "extensions/filters/network/memcached_proxy/codec_impl.h"
#include "extensions/filters/network/memcached_proxy/conn_pool.h"

#include "test/test_common/printers.h"

#include "gmock/gmock.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MemcachedProxy {

class MockEncoder : public Encoder {
public:
  MockEncoder();
  ~MockEncoder();

  MOCK_METHOD2(encodeMessage, void(const Message& message, Buffer::Instance& out));

private:
  BinaryEncoderImpl real_encoder_;
};

class MockDecoder : public Decoder {
public:
  MockDecoder();
  ~MockDecoder();

  MOCK_METHOD1(onData, void(Buffer::Instance& data));
};

namespace ConnPool {

class MockClient : public Client {
public:
  MockClient();
  ~MockClient();

  void raiseEvent(Network::ConnectionEvent event) {
    for (Network::ConnectionCallbacks* callbacks : callbacks_) {
      callbacks->onEvent(event);
    }
  }

  void runHighWatermarkCallbacks() {
    for (auto* callback : callbacks_) {
      callback->onAboveWriteBufferHighWatermark();
    }
  }

  void runLowWatermarkCallbacks() {
    for (auto* callback : callbacks_) {
      callback->onBelowWriteBufferLowWatermark();
    }
  }

  MOCK_METHOD1(addConnectionCallbacks, void(Network::ConnectionCallbacks& callbacks));
  MOCK_METHOD0(close, void());
  MOCK_METHOD2(makeRequest, PoolRequest*(const Message& request, PoolCallbacks& callbacks));

  std::list<Network::ConnectionCallbacks*> callbacks_;
};

class MockPoolRequest : public PoolRequest {
public:
  MockPoolRequest();
  ~MockPoolRequest();

  MOCK_METHOD0(cancel, void());
};

class MockPoolCallbacks : public PoolCallbacks {
public:
  MockPoolCallbacks();
  ~MockPoolCallbacks();

  void onResponse(MessagePtr&& message) override { onResponse_(message); }

  MOCK_METHOD1(onResponse_, void(MessagePtr& message));
  MOCK_METHOD0(onFailure, void());
};

}

}
}
}
}
