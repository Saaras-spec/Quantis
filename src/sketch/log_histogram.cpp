#include "sketch/log_histogram.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace quantis {

LogHistogram::LogHistogram(double relative_error)
    : gamma_((1.0 + relative_error) / (1.0 - relative_error)),
      log_gamma_(std::log(gamma_)) {
    if (!(relative_error > 0.0 && relative_error < 1.0)) {
        throw std::invalid_argument("relative_error must be in (0, 1)");
    }
}

void LogHistogram::add(std::uint32_t value) {
    ++total_;
    if (value == 0) {
        ++zero_;
        return;
    }
    const auto idx = static_cast<std::size_t>(
        std::ceil(std::log(static_cast<double>(value)) / log_gamma_));
    if (idx >= counts_.size()) counts_.resize(idx + 1, 0);
    ++counts_[idx];
}

void LogHistogram::merge(const LogHistogram& other) {
    if (gamma_ != other.gamma_) {
        throw std::invalid_argument("cannot merge histograms with different accuracy");
    }
    if (other.counts_.size() > counts_.size()) counts_.resize(other.counts_.size(), 0);
    for (std::size_t i = 0; i < other.counts_.size(); ++i) counts_[i] += other.counts_[i];
    zero_ += other.zero_;
    total_ += other.total_;
}

double LogHistogram::quantile(double p) const {
    if (total_ == 0) return 0.0;

    auto rank = static_cast<std::uint64_t>(
        std::ceil(p / 100.0 * static_cast<double>(total_)));
    rank = std::clamp<std::uint64_t>(rank, 1, total_);

    if (rank <= zero_) return 0.0;

    std::uint64_t cumulative = zero_;
    for (std::size_t i = 0; i < counts_.size(); ++i) {
        cumulative += counts_[i];
        if (cumulative >= rank) {
            // Midpoint-style estimate that keeps the error within the bound.
            return 2.0 * std::pow(gamma_, static_cast<double>(i)) / (gamma_ + 1.0);
        }
    }
    return 0.0;  // unreachable: cumulative reaches total_ >= rank
}

}  // namespace quantis
