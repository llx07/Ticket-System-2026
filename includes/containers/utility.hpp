#ifndef SJTU_UTILITY_HPP
#define SJTU_UTILITY_HPP

namespace sjtu {

template <class T1, class T2>
class pair {
   public:
    T1 first;
    T2 second;
    constexpr pair() : first(), second() {}
    pair(const pair &other) = default;
    pair(pair &&other) = default;
    pair &operator=(const pair &other) = default;
    pair &operator=(pair &&other) = default;
    pair(const T1 &x, const T2 &y) : first(x), second(y) {}
    template <class U1, class U2>
    pair(U1 &&x, U2 &&y) : first(x), second(y) {}
    template <class U1, class U2>
    explicit pair(const pair<U1, U2> &other)
        : first(other.first), second(other.second) {}
    template <class U1, class U2>
    explicit pair(pair<U1, U2> &&other)
        : first(other.first), second(other.second) {}

    friend bool operator==(const pair &lhs, const pair &rhs) {
        return lhs.first == rhs.first && lhs.second == rhs.second;
    }

    friend bool operator!=(const pair &lhs, const pair &rhs) {
        return !(lhs == rhs);
    }

    friend bool operator<(const pair &lhs, const pair &rhs) {
        if (lhs.first < rhs.first) return true;
        if (rhs.first < lhs.first) return false;
        return lhs.second < rhs.second;
    }

    friend bool operator>(const pair &lhs, const pair &rhs) {
        return rhs < lhs;
    }

    friend bool operator<=(const pair &lhs, const pair &rhs) {
        return !(rhs < lhs);
    }

    friend bool operator>=(const pair &lhs, const pair &rhs) {
        return !(lhs < rhs);
    }
};

}  // namespace sjtu

#endif
