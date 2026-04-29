
#include "b_plus_tree.hpp"

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
