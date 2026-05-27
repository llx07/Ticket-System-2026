#ifndef SJTU_FIXED_STRING_HPP
#define SJTU_FIXED_STRING_HPP

#include <iostream>
#include <string>

template <int N>
struct FixedString {
    char data[N];

    FixedString() = default;
    FixedString(const std::string &str) { assign(str); }
    FixedString(const char *str) { assign(str); }

    void clear() {
        for (int i = 0; i < N; ++i) data[i] = 0;
    }

    void assign(const std::string &str) {
        int i = 0;
        for (; i + 1 < N && i < static_cast<int>(str.size()); ++i) {
            data[i] = str[i];
        }
        for (; i < N; ++i) data[i] = 0;
    }

    void assign(const char *str) {
        int i = 0;
        for (; i + 1 < N && str[i] != 0; ++i) {
            data[i] = str[i];
        }
        for (; i < N; ++i) data[i] = 0;
    }

    std::string to_string() const {
        std::string result;
        for (int i = 0; i < N && data[i] != 0; ++i) {
            result += data[i];
        }
        return result;
    }

    bool empty() const { return data[0] == 0; }

    int size() const {
        int len = 0;
        while (len < N && data[len] != 0) ++len;
        return len;
    }

    const char *c_str() const { return data; }

    char &operator[](int index) { return data[index]; }

    const char &operator[](int index) const { return data[index]; }

    FixedString &operator=(const std::string &str) {
        assign(str);
        return *this;
    }

    FixedString &operator=(const char *str) {
        assign(str);
        return *this;
    }

    operator std::string() const { return to_string(); }

    friend bool operator<(const FixedString &lhs, const FixedString &rhs) {
        for (int i = 0; i < N; ++i) {
            if (lhs.data[i] != rhs.data[i]) {
                return lhs.data[i] < rhs.data[i];
            }
        }
        return false;
    }

    friend bool operator==(const FixedString &lhs, const FixedString &rhs) {
        for (int i = 0; i < N; ++i) {
            if (lhs.data[i] != rhs.data[i]) return false;
        }
        return true;
    }

    friend bool operator!=(const FixedString &lhs, const FixedString &rhs) {
        return !(lhs == rhs);
    }

    friend std::ostream &operator<<(std::ostream &os,
                                    const FixedString &str) {
        for (int i = 0; i < N && str.data[i] != 0; ++i) {
            os << str.data[i];
        }
        return os;
    }
};

#endif
