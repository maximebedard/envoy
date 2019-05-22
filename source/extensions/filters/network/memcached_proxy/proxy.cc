#include "extensions/filters/network/memcached_proxy/proxy.h"

#include <chrono>
#include <cstdint>
#include <string>

#include "envoy/common/exception.h"
#include "envoy/event/dispatcher.h"
#include "envoy/filesystem/filesystem.h"
#include "envoy/runtime/runtime.h"
#include "envoy/stats/scope.h"

#include "common/common/assert.h"
#include "common/common/fmt.h"
#include "common/common/utility.h"

#include "extensions/filters/network/well_known_names.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MemcachedProxy {

ProxyFilter::ProxyFilter(
  const std::string& stat_prefix,
  Stats::Scope& scope,
  ConnPool::Instance& conn_pool,
  // Runtime::Loader& runtime,
  // const Network::DrainDecision& drain_decision,
  // Runtime::RandomGenerator& generator,
  // TimeSource& time_source,
  DecoderFactory& factory,
  EncoderPtr&& encoder)
    : stat_prefix_(stat_prefix),
      // scope_(scope),
      stats_(generateStats(stat_prefix, scope)),
      conn_pool_(conn_pool),
      // runtime_(runtime),
      // drain_decision_(drain_decision),
      // generator_(generator),
      // time_source_(time_source),
      decoder_(factory.create(*this)), encoder_(std::move(encoder)) {}

void ProxyFilter::decodeMessage(MessagePtr&& message) {
  if (message->key().empty()) {
    throw EnvoyException("key can't be empty");
  }
  conn_pool_.makeRequest(message->key(), *message, *this);
}


void ProxyFilter::onEvent(Network::ConnectionEvent event) {
  if (event == Network::ConnectionEvent::RemoteClose ||
      event == Network::ConnectionEvent::LocalClose) {
    if (drain_close_timer_) {
      drain_close_timer_->disableTimer();
      drain_close_timer_.reset();
    }
  }

  // if (event == Network::ConnectionEvent::RemoteClose && !active_query_list_.empty()) {
  //   stats_.cx_destroy_local_with_active_rq_.inc();
  // }

  // if (event == Network::ConnectionEvent::LocalClose && !active_query_list_.empty()) {
  //   stats_.cx_destroy_remote_with_active_rq_.inc();
  // }
}

void ProxyFilter::onResponse(MessagePtr&& message) {
  encoder_->encodeMessage(*message, write_buffer_);

  if (write_buffer_.length() > 0) {
    read_callbacks_->connection().write(write_buffer_, false);
  }

  read_callbacks_->connection().close(Network::ConnectionCloseType::FlushWrite);
  // encoder_->encode(message, encoder_buffer_);
}

Network::FilterStatus ProxyFilter::onData(Buffer::Instance& data, bool) {
  try {
    decoder_->onData(data);
    return Network::FilterStatus::Continue;
  } catch (ProtocolError&) {
    stats_.downstream_cx_protocol_error_.inc();
    // Common::Redis::RespValue error;
    // error.type(Common::Redis::RespType::Error);
    // error.asString() = "downstream protocol error";
    // encoder_->encode(error, encoder_buffer_);
    // callbacks_->connection().write(encoder_buffer_, false);
    // callbacks_->connection().close(Network::ConnectionCloseType::NoFlush);
    return Network::FilterStatus::StopIteration;
  }
}

// Network::FilterStatus ProxyFilter::onWrite(Buffer::Instance& data, bool) {
//   ENVOY_LOG(info, "upstream -> downstream => bytes={}", data.length());
//   write_buffer_.add(data);

//   try {
//     decoder_->onData(write_buffer_);
//   } catch (EnvoyException& e) {
//     ENVOY_LOG(info, "memcached decoding error: {}", e.what());
//     stats_.decoding_error_.inc();
//   }

//   return Network::FilterStatus::Continue;
// }

}
}
}
}
