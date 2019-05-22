#pragma once

#include <chrono>
#include <cstdint>
#include <list>
#include <memory>
#include <string>

#include "envoy/access_log/access_log.h"
#include "envoy/common/time.h"
#include "envoy/config/filter/network/memcached_proxy/v2/memcached_proxy.pb.h"
#include "envoy/event/timer.h"
#include "envoy/network/connection.h"
#include "envoy/network/drain_decision.h"
#include "envoy/network/filter.h"
#include "envoy/runtime/runtime.h"
#include "envoy/stats/scope.h"
#include "envoy/stats/stats_macros.h"

#include "common/buffer/buffer_impl.h"
#include "common/common/logger.h"
#include "common/network/filter_impl.h"
#include "common/protobuf/utility.h"
#include "common/singleton/const_singleton.h"

#include "extensions/filters/network/memcached_proxy/codec.h"
#include "extensions/filters/network/memcached_proxy/conn_pool.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MemcachedProxy {

/**
 * All memcached proxy stats. @see stats_macros.h
 */
// clang-format off
#define ALL_MEMCACHED_PROXY_STATS(COUNTER, GAUGE) \
  COUNTER(downstream_cx_rx_bytes_total)                                                            \
  GAUGE  (downstream_cx_rx_bytes_buffered)                                                         \
  COUNTER(downstream_cx_tx_bytes_total)                                                            \
  GAUGE  (downstream_cx_tx_bytes_buffered)                                                         \
  COUNTER(downstream_cx_protocol_error)                                                            \
// clang-format on

/**
 * Struct definition for all memcached proxy stats. @see stats_macros.h
 */
struct MemcachedProxyStats {
  ALL_MEMCACHED_PROXY_STATS(GENERATE_COUNTER_STRUCT, GENERATE_GAUGE_STRUCT)
};

/**
 * A sniffing filter for memcached traffic. The current implementation makes a copy of read/written
 * data, decodes it, and generates stats.
 */
class ProxyFilter : public Network::ReadFilter,
                    public DecoderCallbacks,
                    public ConnPool::PoolCallbacks,
                    public Network::ConnectionCallbacks,
                    Logger::Loggable<Logger::Id::memcached> {
public:
  ProxyFilter(const std::string& stat_prefix, Stats::Scope& scope,
            ConnPool::Instance& conn_pool,
            // Runtime::Loader& runtime,
             // const Network::DrainDecision& drain_decision, Runtime::RandomGenerator& generator,
              // TimeSource& time_source,
              DecoderFactory& factory, EncoderPtr&& encoder);
  ~ProxyFilter() = default;

  // Network::ReadFilter
  Network::FilterStatus onData(Buffer::Instance& data, bool end_stream) override;
  Network::FilterStatus onNewConnection() override { return Network::FilterStatus::Continue; }
  void initializeReadFilterCallbacks(Network::ReadFilterCallbacks& callbacks) override {
    read_callbacks_ = &callbacks;
    read_callbacks_->connection().addConnectionCallbacks(*this);
    read_callbacks_->connection().setConnectionStats({stats_.downstream_cx_rx_bytes_total_,
                                                      stats_.downstream_cx_rx_bytes_buffered_,
                                                      stats_.downstream_cx_tx_bytes_total_,
                                                      stats_.downstream_cx_tx_bytes_buffered_,
                                                      nullptr, nullptr});
  }

  // Network::ConnectionCallbacks
  void onEvent(Network::ConnectionEvent event) override;
  void onAboveWriteBufferHighWatermark() override {}
  void onBelowWriteBufferLowWatermark() override {}

  // MemcachedProxy::DecoderCallback
  void decodeMessage(MessagePtr&& message) override;

  // MemcachedProxy::ConnPool::PoolCallbacks
  void onResponse(MessagePtr&&) override;
  void onFailure() override {}
private:
  MemcachedProxyStats generateStats(const std::string& prefix, Stats::Scope& scope) {
    return MemcachedProxyStats{ALL_MEMCACHED_PROXY_STATS(POOL_COUNTER_PREFIX(scope, prefix),
                                                 POOL_GAUGE_PREFIX(scope, prefix))};
  }

  std::string stat_prefix_;
  // Stats::Scope& scope_;
  MemcachedProxyStats stats_;
  // Runtime::Loader& runtime_;
  // const Network::DrainDecision& drain_decision_;
  // Runtime::RandomGenerator& generator_;
  Buffer::OwnedImpl read_buffer_;
  Buffer::OwnedImpl write_buffer_;
  Network::ReadFilterCallbacks* read_callbacks_{};
  Event::TimerPtr drain_close_timer_;
  // TimeSource& time_source_;
  ConnPool::Instance& conn_pool_;
  DecoderPtr decoder_;
  EncoderPtr encoder_;
};

}
}
}
}
