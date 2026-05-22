#include "parser.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("parser parses key value arguments", "[parser]") {
    Command cmd = parse_command("[456] login -u Alice -p pass123");

    REQUIRE(cmd.has('u'));
    REQUIRE(cmd.has('p'));
    REQUIRE(cmd.arg('u') == "Alice");
    REQUIRE(cmd.arg('p') == "pass123");
    REQUIRE_FALSE(cmd.has('c'));
}

TEST_CASE("parser parses complex command", "[parser]") {
    Command cmd = parse_command(
        "[666] add_train -m 1000 -i HAPPY_TRAIN -s "
        "上院|中院|下院 -p 114|514 -d 06-01|08-17");

    REQUIRE(cmd.timestamp == 666);
    REQUIRE(cmd.name == "add_train");
    REQUIRE(cmd.arg('m') == "1000");
    REQUIRE(cmd.arg('i') == "HAPPY_TRAIN");
    REQUIRE(cmd.arg('s') == "上院|中院|下院");
    REQUIRE(cmd.arg('p') == "114|514");
    REQUIRE(cmd.arg('d') == "06-01|08-17");
}