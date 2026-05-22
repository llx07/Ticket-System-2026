#include "common/fixed_string.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("FixedString converts to and from std::string", "[fixed_string]") {
    FixedString<21> username("alice");

    REQUIRE(username.to_string() == "alice");
    REQUIRE(static_cast<std::string>(username) == "alice");
    REQUIRE(username.size() == 5);
    REQUIRE_FALSE(username.empty());
}

TEST_CASE("FixedString clears remaining bytes", "[fixed_string]") {
    FixedString<8> text("abcdef");

    text = "xy";

    REQUIRE(text.to_string() == "xy");
    REQUIRE(text[2] == 0);
    REQUIRE(text[7] == 0);
}

TEST_CASE("FixedString truncates to leave null terminator", "[fixed_string]") {
    FixedString<5> text("abcdef");

    REQUIRE(text.to_string() == "abcd");
    REQUIRE(text[4] == 0);
}

TEST_CASE("FixedString compares lexicographically", "[fixed_string]") {
    FixedString<8> alice("alice");
    FixedString<8> bob("bob");
    FixedString<8> alice_copy("alice");

    REQUIRE(alice < bob);
    REQUIRE(alice == alice_copy);
    REQUIRE(alice != bob);
}
