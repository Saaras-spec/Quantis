#include <gtest/gtest.h>

#include "core/reference_aggregator.hpp"

using quantis::Event;
using quantis::ReferenceAggregator;

static Event make(const std::string& svc, std::int64_t ts, std::uint32_t lat, std::uint16_t status = 200) {
    return Event{svc, ts, 0, lat, status};
}

TEST(ReferenceAggregator, KnownPercentiles) {
    ReferenceAggregator agg;
    for (std::uint32_t i = 1; i <= 100; ++i) agg.add(make("a", 1000, i));

    auto s = agg.query("a", 0, 2000);
    EXPECT_EQ(s.count, 100u);
    EXPECT_DOUBLE_EQ(s.avg_latency_ms, 50.5);
    EXPECT_DOUBLE_EQ(s.p50_ms, 50.0);
    EXPECT_DOUBLE_EQ(s.p95_ms, 95.0);
    EXPECT_DOUBLE_EQ(s.p99_ms, 99.0);
}

TEST(ReferenceAggregator, CountsErrors) {
    ReferenceAggregator agg;
    agg.add(make("a", 10, 5, 200));
    agg.add(make("a", 20, 5, 500));
    agg.add(make("a", 30, 5, 503));
    agg.add(make("a", 40, 5, 404));  // 4xx is not counted as error here

    auto s = agg.query("a", 0, 100);
    EXPECT_EQ(s.count, 4u);
    EXPECT_EQ(s.errors, 2u);
    EXPECT_DOUBLE_EQ(s.error_rate(), 0.5);
}

TEST(ReferenceAggregator, FiltersServiceAndHalfOpenWindow) {
    ReferenceAggregator agg;
    agg.add(make("a", 100, 10));
    agg.add(make("a", 200, 20));  // exactly at end_ms: excluded
    agg.add(make("b", 150, 99));  // other service: excluded

    auto s = agg.query("a", 100, 200);
    EXPECT_EQ(s.count, 1u);
    EXPECT_DOUBLE_EQ(s.avg_latency_ms, 10.0);
}

TEST(ReferenceAggregator, EmptyWindowIsZeros) {
    ReferenceAggregator agg;
    auto s = agg.query("nothing", 0, 1000);
    EXPECT_EQ(s.count, 0u);
    EXPECT_DOUBLE_EQ(s.error_rate(), 0.0);
}