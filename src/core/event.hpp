#pragma once

#include <cstdint>
#include <string>

namespace quantis {

struct Event {
    std::string service;
    std::int64_t timestamp_ms;  // event time, ms since epoch
    std::uint64_t request_id;
    std::uint32_t latency_ms;
    std::uint16_t status_code;

    bool is_error() const { return status_code >= 500; }
};

}  // namespace quantis