#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <random>
#include <vector>

#include "sketch/log_histogram.hpp"

using quantis::LogHistogram;

namespace {
double exact_nearest_rank(std::vector<std::uint32_t> v, double p) {
    std::sort(v.begin(), v.end());
    auto rank = static_cast<std::size_t>(std::ceil(p / 100.0 * static_cast<double>(v.size())));
    rank = std::clamp<std::size_t>(rank, 1, v.size());
    return static_cast<double>(v[rank - 1]);
}

void expect_close(double est, double exact, double rel) {
    if (exact == 0.0) {
        EXPECT_EQ(est, 0.0);
    } else {
        EXPECT_LE(std::fabs(est - exact), exact * rel);
    }
}
}  // namespace

TEST(LogHistogram, EmptyReturnsZero) {
    LogHistogram h;
    EXPECT_EQ(h.count(), 0u);
    EXPECT_EQ(h.quantile(99.0), 0.0);
}

TEST(LogHistogram, ZerosAreExact) {
    LogHistogram h;
    for (int i = 0; i < 10; ++i) h.add(0);
    EXPECT_EQ(h.quantile(50.0), 0.0);
    EXPECT_EQ(h.quantile(99.0), 0.0);
}

TEST(LogHistogram, WithinRelativeErrorOfExact) {
    constexpr double kErr = 0.01;
    LogHistogram h(kErr);
    std::mt19937_64 rng(42);
    std::lognormal_distribution<double> dist(4.0, 1.0);
    std::vector<std::uint32_t> values;
    for (int i = 0; i < 200'000; ++i) {
        auto v = static_cast<std::uint32_t>(dist(rng));
        values.push_back(v);
        h.add(v);
    }
    for (double p : {50.0, 90.0, 95.0, 99.0, 99.9}) {
        expect_close(h.quantile(p), exact_nearest_rank(values, p), kErr * 1.001);
    }
}

TEST(LogHistogram, MergeEqualsSingleHistogram) {
    LogHistogram all, a, b;
    std::mt19937_64 rng(7);
    std::uniform_int_distribution<std::uint32_t> dist(0, 5000);
    for (int i = 0; i < 50'000; ++i) {
        auto v = dist(rng);
        all.add(v);
        (i % 2 == 0 ? a : b).add(v);
    }
    a.merge(b);
    EXPECT_EQ(a.count(), all.count());
    for (double p : {50.0, 95.0, 99.0}) {
        EXPECT_DOUBLE_EQ(a.quantile(p), all.quantile(p));
    }
}
