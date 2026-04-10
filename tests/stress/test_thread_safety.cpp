#include "catch_amalgamated.hpp"
#include <atomic>

TEST_CASE("Placeholder Test", "[threading]")
{
    std::atomic<int> counter{0};
    counter.fetch_add(1);
    REQUIRE(counter.load() == 1);
}