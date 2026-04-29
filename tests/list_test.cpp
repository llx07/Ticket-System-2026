#include "list.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("list supports basic operations", "[list]") {
    sjtu::list<int> values;

    REQUIRE(values.empty());
    values.push_back(2);
    values.push_front(1);
    values.push_back(4);

    auto it = values.begin();
    ++it;
    ++it;
    values.insert(it, 3);

    REQUIRE(values.size() == 4);
    REQUIRE(values.front() == 1);
    REQUIRE(values.back() == 4);

    int expected = 1;
    for (auto iter = values.begin(); iter != values.end(); ++iter) {
        REQUIRE(*iter == expected);
        ++expected;
    }
}

TEST_CASE("list erases and splices nodes", "[list]") {
    sjtu::list<int> values;
    sjtu::list<int> other;

    values.push_back(1);
    values.push_back(2);
    values.push_back(4);
    other.push_back(3);

    auto erase_it = values.begin();
    ++erase_it;
    values.erase(erase_it);

    auto pos = values.end();
    --pos;
    values.splice(pos, other, other.begin());

    REQUIRE(other.empty());
    REQUIRE(values.size() == 3);

    auto iter = values.begin();
    REQUIRE(*iter == 1);
    ++iter;
    REQUIRE(*iter == 3);
    ++iter;
    REQUIRE(*iter == 4);
    ++iter;
    REQUIRE(iter == values.end());
}
