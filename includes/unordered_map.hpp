#ifndef SJTU_UNORDERED_MAP_HPP
#define SJTU_UNORDERED_MAP_HPP


#include "exceptions.hpp"
#include "utility.hpp"

namespace sjtu {

template <class T>
class hash;

template <>
class hash<int> {
   public:
    size_t operator()(const int &value) const {   
        size_t k = static_cast<size_t>(value);
    
        k ^= k << 13;
        k ^= k >> 17;
        k ^= k << 5;
        return k;
    }
};

template <class Key, class T, class Hash = hash<Key> >
class unordered_map {
   public:
    using value_type = pair<const Key, T>;

   private:
    static constexpr size_t DEFAULT_BUCKET_COUNT = 16;
    static constexpr double LOAD_FACTOR = 0.75;

    struct Node {
        value_type value;
        Node *nxt;
        Node(const Key &k, const T &v) : value(k, v), nxt(nullptr){};
    };

    Node **buckets;
    size_t bucket_cnt;
    size_t sz;
    Hash hash_func;

    size_t bucket_index(const Key &key) const {
        return hash_func(key) % bucket_cnt;
    }

   public:
    class const_iterator;
    class iterator {
        friend class unordered_map;
        friend class const_iterator;

       private:
        Node *node;
        explicit iterator(Node *node_) : node(node_) {}

       public:
        iterator() : node(nullptr) {}

        value_type &operator*() const {
            if (!node) throw invalid_iterator{};
            return node->value;
        }
        value_type *operator->() const { return &operator*(); }

        bool operator==(const iterator &rhs) const { return node == rhs.node; }
        bool operator!=(const iterator &rhs) const { return !(*this == rhs); }
        bool operator==(const const_iterator &rhs) const {
            return node == rhs.node;
        }
        bool operator!=(const const_iterator &rhs) const {
            return !(*this == rhs);
        }
    };

    class const_iterator {
        friend class unordered_map;

       private:
        const Node *node;
        explicit const_iterator(const Node *node_) : node(node_) {}

       public:
        const_iterator() : node(nullptr) {}
        explicit const_iterator(const iterator &other) : node(other.node) {}

        const value_type &operator*() const {
            if (!node) throw invalid_iterator{};
            return node->value;
        }
        const value_type *operator->() const { return &operator*(); }

        bool operator==(const const_iterator &rhs) const {
            return node == rhs.node;
        }
        bool operator!=(const const_iterator &rhs) const {
            return !(*this == rhs);
        }
    };

    unordered_map()
        : buckets(nullptr), bucket_cnt(0), sz(0), hash_func(Hash()) {
        bucket_cnt = DEFAULT_BUCKET_COUNT;
        buckets = new Node *[bucket_cnt];
        for (size_t i = 0; i < bucket_cnt; ++i) {
            buckets[i] = nullptr;
        }
    }

    // not implemented
    unordered_map(const unordered_map &other) = delete;
    unordered_map &operator=(const unordered_map &other) = delete;

    ~unordered_map() {
        clear();
        delete[] buckets;
    }

    bool empty() const { return sz == 0; }
    size_t size() const { return sz; }

    void clear() {
        for (size_t i = 0; i < bucket_cnt; i++) {
            Node *curr = buckets[i];
            while (curr != nullptr) {
                Node *nxt = curr->nxt;
                delete curr;
                curr = nxt;
            }
            buckets[i] = nullptr;
        }
        sz = 0;
    }

    void rehash() {
        size_t old_bucket_cnt = bucket_cnt;
        size_t new_bucket_cnt = bucket_cnt * 2;
        Node **old_buckets = buckets;
        buckets = new Node *[new_bucket_cnt];
        bucket_cnt = new_bucket_cnt;
        for (size_t i = 0; i < bucket_cnt; i++) {
            buckets[i] = nullptr;
        }

        for (size_t i = 0; i < old_bucket_cnt; i++) {
            Node *curr = old_buckets[i];
            while (curr != nullptr) {
                Node *nxt = curr->nxt;
                size_t idx = bucket_index(curr->value.first);
                curr->nxt = buckets[idx];
                buckets[idx] = curr;
                curr = nxt;
            }
        }

        delete[] old_buckets;
    }

    pair<iterator, bool> insert(const value_type &value) {
        auto it = find(value.first);
        if (it != end()) {
            return pair<iterator, bool>(it, false);
        }
        if ((sz + 1.0) / bucket_cnt > LOAD_FACTOR) {
            rehash();
        }
        size_t idx = bucket_index(value.first);

        Node *new_node = new Node(value.first, value.second);
        new_node->nxt = buckets[idx];
        buckets[idx] = new_node;
        ++sz;
        return pair<iterator, bool>(iterator(new_node), true);
    }

    void erase(iterator pos) {
        if (pos == end()) throw invalid_iterator{};
        size_t idx = bucket_index(pos.node->value.first);
        Node *curr = buckets[idx];
        Node *prev = nullptr;

        while (curr != nullptr) {
            if (curr == pos.node) {
                if (prev == nullptr) {
                    buckets[idx] = curr->nxt;
                } else {
                    prev->nxt = curr->nxt;
                }
                delete curr;
                --sz;
                return;
            }
            prev = curr;
            curr = curr->nxt;
        }
        throw invalid_iterator{};
    }

    size_t erase(const Key &key) {
        iterator it = find(key);
        if (it == end()) return 0;
        erase(it);
        return 1;
    }

    iterator end() { return iterator(nullptr); }
    const_iterator end() const { return const_iterator(nullptr); }
    const_iterator cend() const { return const_iterator(nullptr); }

    iterator find(const Key &key) {
        size_t idx = bucket_index(key);
        Node *curr = buckets[idx];
        while (curr != nullptr) {
            if (curr->value.first == key) {
                return iterator(curr);
            }
            curr = curr->nxt;
        }
        return end();
    }

    const_iterator find(const Key &key) const {
        size_t idx = bucket_index(key);
        Node *curr = buckets[idx];
        while (curr != nullptr) {
            if (curr->value.first == key) {
                return const_iterator(curr);
            }
            curr = curr->nxt;
        }
        return cend();
    }

    size_t count(const Key &key) const { return find(key) != cend() ? 1 : 0; }

    T &at(const Key &key) {
        iterator it = find(key);
        if (it == end()) throw index_out_of_bound{};
        return it->second;
    }

    const T &at(const Key &key) const {
        const_iterator it = find(key);
        if (it == cend()) throw index_out_of_bound{};
        return it->second;
    }

    T &operator[](const Key &key) {
        iterator it = find(key);
        if (it != end()) {
            return it->second;
        }
        return insert(value_type(key, T())).first->second;
    }
};

}  // namespace sjtu

#endif
