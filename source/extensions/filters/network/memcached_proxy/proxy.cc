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
  ConnPool::Instance& conn_pool,
  Stats::Scope& scope,
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

void ProxyFilter::decodeGet(GetRequestPtr&& request) {
  stats_.op_get_.inc();
  ENVOY_LOG(trace, "decoded `GET` key={}", request->key());
}

void ProxyFilter::decodeGetk(GetkRequestPtr&& request) {
  stats_.op_getk_.inc();
  ENVOY_LOG(trace, "decoded `GETK` key={}", request->key());
}

void ProxyFilter::decodeDelete(DeleteRequestPtr&& request) {
  stats_.op_delete_.inc();
  ENVOY_LOG(trace, "decoded `DELETE` key={}", request->key());
}

void ProxyFilter::decodeSet(SetRequestPtr&& request) {
  stats_.op_set_.inc();
  ENVOY_LOG(trace, "decoded `SET` key={}, body={}", request->key(), request->body());
}

void ProxyFilter::decodeAdd(AddRequestPtr&& request) {
  stats_.op_add_.inc();
  ENVOY_LOG(trace, "decoded `ADD` key={}, body={}", request->key(), request->body());
}

void ProxyFilter::decodeReplace(ReplaceRequestPtr&& request) {
  stats_.op_replace_.inc();
  ENVOY_LOG(trace, "decoded `REPLACE` key={}, body={}", request->key(), request->body());
}

void ProxyFilter::decodeIncrement(IncrementRequestPtr&& request) {
  stats_.op_increment_.inc();
  ENVOY_LOG(trace, "decoded `INCREMENT` key={}, amount={}, initial_value={}", request->key(), request->amount(), request->initialValue());
}

void ProxyFilter::decodeDecrement(DecrementRequestPtr&& request) {
  stats_.op_decrement_.inc();
  ENVOY_LOG(trace, "decoded `DECREMENT` key={}, amount={}, initial_value={}", request->key(), request->amount(), request->initialValue());
}

void ProxyFilter::decodeAppend(AppendRequestPtr&& request) {
  stats_.op_append_.inc();
  ENVOY_LOG(trace, "decoded `APPEND` key={}, body={}", request->key(), request->body());
}

void ProxyFilter::decodePrepend(PrependRequestPtr&& request) {
  stats_.op_prepend_.inc();
  ENVOY_LOG(trace, "decoded `PREPEND` key={}, body={}", request->key(), request->body());
}

void ProxyFilter::decodeVersion(VersionRequestPtr&&) {
  stats_.op_version_.inc();
  ENVOY_LOG(trace, "decoded `VERSION`");
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

Network::FilterStatus ProxyFilter::onData(Buffer::Instance& data, bool) {
  ENVOY_LOG(info, "downstream -> upstream => bytes={}", data.length());
  read_buffer_.add(data);

  try {
    decoder_->onData(read_buffer_);
  } catch (EnvoyException& e) {
    ENVOY_LOG(info, "memcached decoding error: {}", e.what());
    stats_.decoding_error_.inc();
  }

  // blindly forward the data upstream
  return Network::FilterStatus::Continue;
}

Network::FilterStatus ProxyFilter::onWrite(Buffer::Instance& data, bool) {
  ENVOY_LOG(info, "upstream -> downstream => bytes={}", data.length());
  write_buffer_.add(data);

  try {
    decoder_->onData(write_buffer_);
  } catch (EnvoyException& e) {
    ENVOY_LOG(info, "memcached decoding error: {}", e.what());
    stats_.decoding_error_.inc();
  }

  return Network::FilterStatus::Continue;
}

}
}
}
}
