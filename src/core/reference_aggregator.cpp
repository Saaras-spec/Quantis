#include "core/reference_aggregator.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace quantis {

namespace {
// Nearest-rank percentile: the smallest value such that at least p% of
// values are <= it. `sorted` must be non-empty and ascending.
double nearest_rank(const std::vector<std::uint32_t>& sorted, double p) {
    const auto n = sorted.size();
    auto rank = static_cast<std::size_t>(std::ceil(p / 100.0 * static_cast<double>(n)));
    rank = std::clamp<std::size_t>(rank, 1, n);
    return static_cast<double>(sorted[rank - 1]);
}
}  // namespace

void ReferenceAggregator::add(const Event& e) { events_.push_back(e); }

WindowStats ReferenceAggregator::query(const std::string& service, std::int64_t start_ms, std::int64_t end_ms) const {
    WindowStats s;
    std::vector<std::uint32_t> latencies;

    for (const auto& e : events_) {
        if (e.service != service) continue;
        if (e.timestamp_ms < start_ms || e.timestamp_ms >= end_ms) continue;
        latencies.push_back(e.latency_ms);
        ++s.count;
        if (e.is_error()) ++s.errors;
    }
    if (latencies.empty()) return s;

    std::sort(latencies.begin(), latencies.end());
    const double sum = std::accumulate(latencies.begin(), latencies.end(), 0.0);
    s.avg_latency_ms = sum / static_cast<double>(latencies.size());
    s.p50_ms = nearest_rank(latencies, 50.0);
    s.p95_ms = nearest_rank(latencies, 95.0);
    s.p99_ms = nearest_rank(latencies, 99.0);
    return s;
}

}  // namespace quantis