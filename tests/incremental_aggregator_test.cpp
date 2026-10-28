#include <gtest/gtest.h>

#include <random>
#include <string>
#include <vector>

#include "core/incremental_aggregator.hpp"
#include "core/reference_aggregator.hpp"

using namespace quantis;

TEST(IncrementalAggregator, MatchesReferenceOnRandomEvents) {
    constexpr std::int64_t kWindow = 60'000;
    constexpr std::int64_t kWindows = 10;
    constexpr int kEvents = 200'000;

    const std::vector<std::string> services = {"api", "auth", "orders"};

    std::mt19937_64 rng(12345);
    std::uniform_int_distribution<std::int64_t> ts_dist(0, kWindow * kWindows - 1);
    std::uniform_int_distribution<std::uint32_t> lat_dist(1, 2000);
    std::uniform_int_distribution<int> svc_dist(0, 2);
    std::uniform_int_distribution<int> err_dist(0, 99);

    ReferenceAggregator ref;
    IncrementalAggregator inc(kWindow);

    for (int i = 0; i < kEvents; ++i) {
        Event e{services[svc_dist(rng)], ts_dist(rng),
                static_cast<std::uint64_t>(i), lat_dist(rng),
                static_cast<std::uint16_t>(err_dist(rng) < 5 ? 500 : 200)};
        ref.add(e);
        inc.add(e);
    }

    auto check = [&](const std::string& svc, std::int64_t a, std::int64_t b) {
        auto r = ref.query(svc, a * kWindow, b * kWindow);
        auto n = inc.query(svc, a * kWindow, b * kWindow);
        EXPECT_EQ(n.count, r.count) << svc << " [" << a << "," << b << ")";
        EXPECT_EQ(n.errors, r.errors) << svc << " [" << a << "," << b << ")";
        EXPECT_NEAR(n.avg_latency_ms, r.avg_latency_ms, 1e-9)
            << svc << " [" << a << "," << b << ")";
    };

    for (const auto& s : services) check(s, 0, kWindows);

    std::uniform_int_distribution<std::int64_t> pick(0, kWindows - 1);
    for (int i = 0; i < 20; ++i) {
        std::int64_t a = pick(rng);
        std::uniform_int_distribution<std::int64_t> end_pick(a + 1, kWindows);
        std::int64_t b = end_pick(rng);
        for (const auto& s : services) check(s, a, b);
    }

    EXPECT_EQ(inc.query("nope", 0, kWindow).count, 0u);
}

TEST(IncrementalAggregator, EmptyAndInvertedRangesGiveZeros) {
    IncrementalAggregator inc(1000);
    inc.add(Event{"a", 500, 1, 10, 200});

    EXPECT_EQ(inc.query("a", 5000, 6000).count, 0u);
    EXPECT_EQ(inc.query("a", 1000, 1000).count, 0u);
    EXPECT_EQ(inc.query("a", 2000, 1000).count, 0u);
}

TEST(IncrementalAggregator, WindowBoundariesAreHalfOpen) {
    IncrementalAggregator inc(1000);
    inc.add(Event{"a", 999, 1, 10, 200});
    inc.add(Event{"a", 1000, 2, 20, 200});

    EXPECT_EQ(inc.query("a", 0, 1000).count, 1u);
    EXPECT_EQ(inc.query("a", 1000, 2000).count, 1u);
    EXPECT_EQ(inc.query("a", 0, 2000).count, 2u);
    EXPECT_DOUBLE_EQ(inc.query("a", 0, 2000).avg_latency_ms, 15.0);
}

TEST(IncrementalAggregator, NegativeTimestampsLandInTheRightWindow) {
    IncrementalAggregator inc(1000);
    inc.add(Event{"a", -1, 1, 10, 200});

    EXPECT_EQ(inc.query("a", -1000, 0).count, 1u);
    EXPECT_EQ(inc.query("a", 0, 1000).count, 0u);
}
