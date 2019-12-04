#define CATCH_CONFIG_MAIN // This tells Catch to provide a main() - only do this in one cpp file

#include "catch2/catch.hpp"
#include "jg/dense_hash_map.hpp"

#include <string>

TEST_CASE("dense_hash_map", "[factorial]")
{
    jg::dense_hash_map<std::string, int> m;

    SECTION("test") { m.emplace("test", 42); }
}
