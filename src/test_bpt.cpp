#include <iostream>
#include <string>

#include "b_plus_tree.hpp"

template <int size>
struct char_array {
    char data[size];
    friend bool operator<(const char_array& lhs, const char_array& rhs) {
        for (int i = 0; i < size; i++) {
            if (lhs.data[i] != rhs.data[i]) {
                return lhs.data[i] < rhs.data[i];
            }
        }
        return false;
    }
};

using S = char_array<64>;

S read() {
    std::string s;
    std::cin >> s;
    S arr;
    for (int i = 0; i < s.size(); i++) {
        arr.data[i] = s[i];
    }
    for (int i = s.size(); i < 64; i++) {
        arr.data[i] = 0;
    }
    return arr;
};

int main() {
    std::ios::sync_with_stdio(0);
    std::cin.tie(0);

    BPlusTree<S, int> bpt("data.dat");
    int n;
    std::cin >> n;
    while (n--) {
        std::string op;
        std::cin >> op;
        if (op == "insert") {
            S s = read();
            int v;
            std::cin >> v;
            bpt.insert(s, v);
        } else if (op == "find") {
            S s = read();
            auto result = bpt.find_all(s);
            if (result.empty()) {
                std::cout << "null\n";
            } else {
                for (int x : result) {
                    std::cout << x << ' ';
                }
                std::cout << '\n';
            }
        } else {
            S s = read();
            int v;
            std::cin >> v;
            bpt.erase(s, v);
        }
    }
    return 0;
}