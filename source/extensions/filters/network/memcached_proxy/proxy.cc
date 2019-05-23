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
  const Network::DrainDecision& drain_decision,
  // Runtime::RandomGenerator& generator,
  // TimeSource& time_source,
  DecoderFactory& factory,
  EncoderPtr&& encoder)
    : stat_prefix_(stat_prefix),
      // scope_(scope),
      stats_(generateStats(stat_prefix, scope)),
      conn_pool_(conn_pool),
      // runtime_(runtime),
      drain_decision_(drain_decision),
      // generator_(generator),
      // time_source_(time_source),
      decoder_(factory.create(*this)), encoder_(std::move(encoder)) {}

void ProxyFilter::decodeMessage(MessagePtr&& message) {
  pending_requests_.emplace_back(*this);
  PendingRequest& request = pending_requests_.back();
  auto handle = conn_pool_.makeRequest(message->key(), *message, request);
  if (handle) {
    // The splitter can immediately respond and destroy the pending request. Only store the handle
    // if the request is still alive.
    request.request_handle_ = std::move(handle);
  }
}


void ProxyFilter::onEvent(Network::ConnectionEvent event) {
  if (event == Network::ConnectionEvent::RemoteClose ||
      event == Network::ConnectionEvent::LocalClose) {
    while (!pending_requests_.empty()) {
      if (pending_requests_.front().request_handle_ != nullptr) {
        pending_requests_.front().request_handle_->cancel();
      }
      pending_requests_.pop_front();
    }
  }
}

void ProxyFilter::onResponse(PendingRequest& request, MessagePtr&& response) {
  ASSERT(!pending_requests_.empty());
  request.pending_response_ = std::move(response);
  request.request_handle_ = nullptr;

  // The response we got might not be in order, so flush out what we can. (A new response may
  // unlock several out of order responses).
  while (!pending_requests_.empty() && pending_requests_.front().pending_response_) {
    encoder_->encodeMessage(*pending_requests_.front().pending_response_, write_buffer_);
    pending_requests_.pop_front();
  }

  if (write_buffer_.length() > 0) {
    read_callbacks_->connection().write(write_buffer_, false);
  }

  // Check for drain close only if there are no pending responses.
  if (pending_requests_.empty() && drain_decision_.drainClose()) {
    stats_.downstream_cx_drain_close_.inc();
    read_callbacks_->connection().close(Network::ConnectionCloseType::FlushWrite);
  }
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
    // read_callbacks_->connection().write(encoder_buffer_, false);
    // read_callbacks_->connection().close(Network::ConnectionCloseType::NoFlush);
    return Network::FilterStatus::StopIteration;
  }
}


ProxyFilter::PendingRequest::PendingRequest(ProxyFilter& parent) : parent_(parent) {
  parent.stats_.downstream_rq_total_.inc();
  parent.stats_.downstream_rq_active_.inc();
}

ProxyFilter::PendingRequest::~PendingRequest() {
  parent_.stats_.downstream_rq_active_.dec();
}

}
}
}
}
