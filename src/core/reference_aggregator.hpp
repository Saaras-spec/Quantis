#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "core/event.hpp"

namespace quantis {

struct WindowStats {
    std::uint64_t count = 0;
    std::uint64_t errors = 0;
    double avg_latency_ms = 0.0;
    double p50_ms = 0.0;
    double p95_ms = 0.0;
    double p99_ms = 0.0;

    double error_rate() const {
        return count == 0 ? 0.0 : static_cast<double>(errors) / static_cast<double>(count);
    }
};

// Stores every event and answers queries by brute force.
// Slow on purpose: its only job is to be obviously correct.
class ReferenceAggregator {
public:
    void add(const Event& e);

    // Window is half-open: [start_ms, end_ms)
    WindowStats query(const std::string& service, std::int64_t start_ms, std::int64_t end_ms) const;

private:
    std::vector<Event> events_;
};

}  // namespace quantis