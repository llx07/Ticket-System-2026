
#include "b_plus_tree.hpp"
#include "map.hpp"

#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <string>

template <class T, class... Args>
sjtu::vector<T> make_vector(Args... args) {
    sjtu::vector<T> result;
    (result.push_back(args), ...);
    return result;
}

class TempDb {
   private:
    std::filesystem::path path;

   public:
    TempDb() {
        path =
            std::filesystem::temp_directory_path() / ("b_plus_tree_test.dat");
        std::filesystem::remove(path);
    }

    ~TempDb() { std::filesystem::remove(path); }

    std::string string() const { return path.string(); }
};

TEST_CASE("find_all returns empty for missing keys", "[b_plus_tree]") {
    TempDb db;
    BPlusTree<int, int> tree(db.string());

    REQUIRE(tree.find_all(42).empty());

    tree.insert(1, 10);
    tree.insert(3, 30);

    REQUIRE(tree.find_all(2).empty());
}

TEST_CASE("insert stores values and find_all returns them sorted",
          "[b_plus_tree]") {
    TempDb db;
    BPlusTree<int, int> tree(db.string());

    tree.insert(7, 30);
    tree.insert(7, 10);
    tree.insert(7, 20);
    tree.insert(3, 300);
    tree.insert(9, 900);

    REQUIRE(tree.find_all(7) == make_vector<int>(10, 20, 30));
    REQUIRE(tree.find_all(3) == make_vector<int>(300));
    REQUIRE(tree.find_all(9) == make_vector<int>(900));
}

TEST_CASE("find_all works after leaf splits", "[b_plus_tree]") {
    TempDb db;
    BPlusTree<int, int> tree(db.string());

    for (int value = 799; value >= 0; --value) {
        tree.insert(5, value);
    }
    for (int key = 0; key < 20; ++key) {
        tree.insert(100 + key, key);
    }

    const auto values = tree.find_all(5);

    REQUIRE(values.size() == 800);
    for (int i = 0; i < 800; ++i) {
        REQUIRE(values[i] == i);
    }
}

TEST_CASE("erase removes one value and ignores missing pairs",
          "[b_plus_tree]") {
    TempDb db;
    BPlusTree<int, int> tree(db.string());

    tree.insert(7, 30);
    tree.insert(7, 10);
    tree.insert(7, 20);
    tree.insert(3, 300);

    tree.erase(7, 20);
    tree.erase(7, 40);
    tree.erase(9, 900);

    REQUIRE(tree.find_all(7) == make_vector<int>(10, 30));
    REQUIRE(tree.find_all(3) == make_vector<int>(300));
    REQUIRE(tree.find_all(9).empty());
}

TEST_CASE("erase works after leaf splits and merges", "[b_plus_tree]") {
    TempDb db;
    BPlusTree<int, int> tree(db.string());

    for (int value = 0; value < 900; ++value) {
        tree.insert(5, value);
    }
    for (int value = 100; value < 800; ++value) {
        tree.erase(5, value);
    }

    const auto values = tree.find_all(5);

    REQUIRE(values.size() == 200);
    for (int i = 0; i < 100; ++i) {
        REQUIRE(values[i] == i);
    }
    for (int i = 100; i < 200; ++i) {
        REQUIRE(values[i] == i + 700);
    }
}

TEST_CASE("mixed insert erase stress test", "[b_plus_tree]") {
    constexpr int value_limit = 2048;
    constexpr int key_limit = 97;
    constexpr int operation_count = 1000000;

    TempDb db;
    BPlusTree<int, int> tree(db.string());
    sjtu::map<int, int> active;

    for (int i = 0; i < operation_count; ++i) {
        const int key = (i * 37 + 11) % key_limit;
        const int value = (i * 1009 + 17) % value_limit;
        const int code = key * value_limit + value;

        if ((i * 17 + 3) % 10 < 6) {
            if (!active.count(code)) {
                tree.insert(key, value);
                active.insert(sjtu::pair<const int, int>(code, 1));
            }
        } else {
            tree.erase(key, value);
            auto it = active.find(code);
            if (it != active.end()) {
                active.erase(it);
            }
        }

        if (i % 997 == 0) {
            const int check_key = (i * 19 + 5) % key_limit;
            sjtu::vector<int> expected;
            for (int check_value = 0; check_value < value_limit;
                 ++check_value) {
                if (active.count(check_key * value_limit + check_value)) {
                    expected.push_back(check_value);
                }
            }
            REQUIRE(tree.find_all(check_key) == expected);
        }
    }

    for (int key = 0; key < key_limit; ++key) {
        sjtu::vector<int> expected;
        for (int value = 0; value < value_limit; ++value) {
            if (active.count(key * value_limit + value)) {
                expected.push_back(value);
            }
        }
        REQUIRE(tree.find_all(key) == expected);
    }
}
