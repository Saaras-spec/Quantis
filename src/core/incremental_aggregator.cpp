#include "core/incremental_aggregator.hpp"

namespace quantis {

IncrementalAggregator::IncrementalAggregator(std::int64_t window_ms)
    : window_ms_(window_ms > 0 ? window_ms : 1) {}

std::int64_t IncrementalAggregator::window_start(std::int64_t ts) const {
    const std::int64_t rem = ((ts % window_ms_) + window_ms_) % window_ms_;
    return ts - rem;
}

void IncrementalAggregator::add(const Event& e) {
    Bucket& b = data_[e.service][window_start(e.timestamp_ms)];
    ++b.count;
    b.latency_sum += e.latency_ms;
    if (e.is_error()) ++b.errors;
}

WindowStats IncrementalAggregator::query(const std::string& service,
                                         std::int64_t start_ms,
                                         std::int64_t end_ms) const {
    WindowStats s;
    if (start_ms >= end_ms) return s;

    const auto svc = data_.find(service);
    if (svc == data_.end()) return s;

    std::uint64_t latency_sum = 0;
    const auto& windows = svc->second;
    for (auto it = windows.lower_bound(start_ms);
         it != windows.end() && it->first < end_ms; ++it) {
        s.count += it->second.count;
        s.errors += it->second.errors;
        latency_sum += it->second.latency_sum;
    }

    if (s.count > 0) {
        s.avg_latency_ms =
            static_cast<double>(latency_sum) / static_cast<double>(s.count);
    }
    return s;
}

}  // namespace quantis
