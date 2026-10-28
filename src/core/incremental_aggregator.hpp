#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <unordered_map>

#include "core/event.hpp"
#include "core/reference_aggregator.hpp"

namespace quantis {

// Keeps only per-(service, window) summaries, never individual events.
class IncrementalAggregator {
public:
    explicit IncrementalAggregator(std::int64_t window_ms);

    void add(const Event& e);

    // Returns stats for all windows whose start time is in [start_ms, end_ms).
    // start_ms and end_ms should be multiples of window_ms.
    // For now, p50/p95/p99 stay 0 (percentile sketches come in Step 2).
    WindowStats query(const std::string& service,
                      std::int64_t start_ms,
                      std::int64_t end_ms) const;

private:
    struct Bucket {
        std::uint64_t count = 0;
        std::uint64_t errors = 0;
        std::uint64_t latency_sum = 0;
    };

    std::int64_t window_start(std::int64_t ts) const;

    std::int64_t window_ms_;
    // service name -> (window start time -> bucket), windows kept in order
    std::unordered_map<std::string, std::map<std::int64_t, Bucket>> data_;
};

}  // namespace quantis
