#ifndef SJTU_ALGORITHM_HPP
#define SJTU_ALGORITHM_HPP

inline int log2floor(int n) {
    int res = 0;
    while (n) {
        ++res;
        n >>= 1;
    }
    return res;
}

template <class T>
inline void swap(T& a, T& b) {
    T c = b;
    b = a;
    a = c;
}

template <typename Iterator, typename Comp>
inline void insertion_sort(Iterator first, Iterator last, Comp comp) {
    if (last - first <= 1) {
        return;
    }
    for (Iterator i = first + 1; i < last; ++i) {
        auto value = *i;
        Iterator j = i;
        while (j > first && comp(value, *(j - 1))) {
            *j = *(j - 1);
            --j;
        }
        *j = value;
    }
}

template <typename Iterator, typename Comp>
inline void sift_down(Iterator first, int node, int count, Comp comp) {
    while (true) {
        int child = node * 2 + 1;
        if (child >= count) {
            break;
        }
        if (child + 1 < count && comp(*(first + child), *(first + child + 1))) {
            ++child;
        }
        if (!comp(*(first + node), *(first + child))) {
            break;
        }

        swap(*(first + node), *(first + child));
        node = child;
    }
}

template <typename Iterator, typename Comp>
inline void make_heap(Iterator first, Iterator last, Comp comp) {
    auto n = last - first;
    if (n <= 1) {
        return;
    }
    auto start = (n - 2) / 2;
    while (true) {
        sift_down(first, start, n, comp);
        if (start == 0) {
            break;
        }
        --start;
    }
}
template <typename Iterator, typename Comp>
inline void heap_sort(Iterator first, Iterator last, Comp comp) {
    auto n = last - first;
    if (n <= 1) {
        return;
    }
    make_heap(first, last, comp);
    auto end = n - 1;
    while (end > 0) {
        swap(*first, *(first + end));
        sift_down(first, 0, --end, comp);
    }
}

template <typename Iterator, typename Comp>
inline Iterator partition(Iterator first, Iterator last, Comp comp) {
    Iterator mid = first + (last - first) / 2;
    Iterator last_minus_one = last - 1;

    // median-of-three:
    if (comp(*mid, *first)) swap(*mid, *first);
    if (comp(*last_minus_one, *mid)) swap(*last_minus_one, *mid);
    if (comp(*mid, *first)) swap(*mid, *first);

    auto pivot = *mid;

    Iterator left = first;
    Iterator right = last - 1;

    while (true) {
        while (comp(*left, pivot)) ++left;
        while (comp(pivot, *right)) --right;
        if (!(left < right)) return left;
        swap(*left, *right);
        ++left;
        --right;
    }
}

template <typename Iterator, typename Comp>
inline void intro_sort(Iterator first, Iterator last, int depth_limit, Comp comp) {
    while (last - first > 16) {
        if (depth_limit == 0) {
            heap_sort(first, last, comp);
            return;
        }
        --depth_limit;
        Iterator mid = partition(first, last, comp);
        intro_sort(mid, last, depth_limit, comp);
        last = mid;
    }
}

template <typename Iterator, typename Comp>
inline void sort(Iterator first, Iterator last, Comp comp) {
    if (last - first <= 1) {
        return;
    }
    intro_sort(first, last, log2floor(last - first), comp);
    insertion_sort(first, last, comp);
}

template <typename Iterator>
inline void sort(Iterator first, Iterator last) {
    sort(first, last, [](const auto& x, const auto& y) { return x < y; });
}

template <typename Iterator, typename T>
inline Iterator find(Iterator first, Iterator last, const T& value) {
    for (Iterator it = first; it != last; ++it) {
        if (*it == value) {
            return it;
        }
    }
    return last;
}

template <typename Iterator, typename Comp>
inline Iterator min_element(Iterator first, Iterator last, Comp comp) {
    if (first == last) {
        return last;
    }

    Iterator result = first;
    for (Iterator it = first + 1; it != last; ++it) {
        if (comp(*it, *result)) {
            result = it;
        }
    }
    return result;
}

template <typename Iterator>
inline Iterator min_element(Iterator first, Iterator last) {
    return min_element(first, last,
                       [](const auto& x, const auto& y) { return x < y; });
}

template <typename Iterator, typename Comp>
inline Iterator max_element(Iterator first, Iterator last, Comp comp) {
    if (first == last) {
        return last;
    }

    Iterator result = first;
    for (Iterator it = first + 1; it != last; ++it) {
        if (comp(*result, *it)) {
            result = it;
        }
    }
    return result;
}

template <typename Iterator>
inline Iterator max_element(Iterator first, Iterator last) {
    return max_element(first, last,
                       [](const auto& x, const auto& y) { return x < y; });
}

template <typename Iterator>
inline void reverse(Iterator first, Iterator last) {
    while (first < last) {
        --last;
        if (!(first < last)) {
            break;
        }
        swap(*first, *last);
        ++first;
    }
}

#endif  // SJTU_ALGORITHM_HPP