#include <string>

#include "common/buffer/buffer_impl.h"

#include "extensions/filters/network/memcached_proxy/codec_impl.h"

#include "test/test_common/printers.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

using testing::Eq;
using testing::NiceMock;
using testing::Pointee;

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MemcachedProxy {

class TestDecoderCallbacks : public DecoderCallbacks {
public:
  void decodeMessage(MessagePtr&& message) override { decodeMessage_(message); }

  MOCK_METHOD1(decodeMessage_, void(MessagePtr& message));
};

class MemcachedCodecImplTest : public testing::Test {
public:
  Buffer::OwnedImpl output_;
  BinaryEncoderImpl encoder_;
  NiceMock<TestDecoderCallbacks> callbacks_;
  DecoderImpl decoder_{callbacks_};
};

TEST_F(MemcachedCodecImplTest, MessageEquality) {
  // {
  //   GetRequestImpl g1(1, 1, 1, 1);
  //   GetRequestImpl g2(2, 2, 2, 2);
  //   EXPECT_FALSE(g1 == g2);
  // }

  // {
  //   GetRequestImpl g1(1, 1, 1, 1);
  //   g1.key("foo");
  //   GetRequestImpl g2(1, 1, 1, 1);
  //   g2.key("bar");
  //   EXPECT_FALSE(g1 == g2);
  // }

  // {
  //   GetRequestImpl g1(1, 1, 1, 1);
  //   g1.key("foo");
  //   GetRequestImpl g2(1, 1, 1, 1);
  //   g2.key("foo");
  //   EXPECT_TRUE(g1 == g2);
  // }
}

TEST_F(MemcachedCodecImplTest, MessageRoundtrip) {
  MessageImpl msg(Message::Type::V1_REQUEST, Message::OpCode::GET, 0, 0, 0, 0, "foo", "", "");

  encoder_.encodeMessage(msg, output_);
  EXPECT_CALL(callbacks_, decodeMessage_(Pointee(Eq(msg))));
  decoder_.onData(output_);
}

} // namespace MemcachedProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
