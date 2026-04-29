#include "unordered_map.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("unordered_map supports basic operations", "[unordered_map]") {
    sjtu::unordered_map<int, int> map;

    REQUIRE(map.empty());
    map.insert(sjtu::pair<const int, int>{1, 100});
    map.insert(sjtu::pair<const int, int>{2, 200});

    REQUIRE(map[1] == 100);
    REQUIRE(map[2] == 200);

    map[3] = 300;
    map[2] = 150;

    REQUIRE(map[2] == 150);
    REQUIRE(map.at(3) == 300);
    REQUIRE(map.size() == 3);

    map.erase(2);

    REQUIRE(map.find(2) == map.end());
    REQUIRE(map.find(3) != map.end());
    REQUIRE(map.count(1) == 1);
    REQUIRE(map.count(2) == 0);
    REQUIRE(map.size() == 2);
}
