#pragma once

#include <cstdint>
#include <vector>

namespace quantis {

// Mergeable quantile sketch with a guaranteed relative error
// (DDSketch-style log-spaced buckets). For any value v >= 1 the estimate
// is within `relative_error` of the true value; zero is tracked exactly.
class LogHistogram {
public:
    explicit LogHistogram(double relative_error = 0.01);

    void add(std::uint32_t value);

    // Both histograms must use the same relative_error (throws otherwise).
    void merge(const LogHistogram& other);

    std::uint64_t count() const { return total_; }

    // p in [0, 100]. Nearest-rank, like the reference aggregator.
    // Returns 0 when empty.
    double quantile(double p) const;

private:
    double gamma_;
    double log_gamma_;
    std::uint64_t zero_ = 0;
    std::uint64_t total_ = 0;
    std::vector<std::uint64_t> counts_;  // counts_[i] covers (gamma^(i-1), gamma^i]
};

}  // namespace quantis
