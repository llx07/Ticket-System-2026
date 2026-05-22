#ifndef SJTU_LIST_HPP
#define SJTU_LIST_HPP

#include "exceptions.hpp"

namespace sjtu {

template <class T>
class list {
   private:
    struct Node {
        T *data;
        Node *prev;
        Node *next;

        Node() : data(nullptr), prev(this), next(this) {}
        explicit Node(const T &value)
            : data(new T(value)), prev(nullptr), next(nullptr) {}
        ~Node() { delete data; }
    };

    Node sentinel;
    size_t sz;

    void insert_node_before(Node *pos, Node *node) {
        node->prev = pos->prev;
        node->next = pos;
        pos->prev->next = node;
        pos->prev = node;
        ++sz;
    }

    Node *erase_node(Node *node) {
        Node *next_node = node->next;
        node->prev->next = node->next;
        node->next->prev = node->prev;
        delete node;
        --sz;
        return next_node;
    }

   public:
    class const_iterator;
    class iterator {
        friend class list;
        friend class const_iterator;

       private:
        Node *node;

        explicit iterator(Node *node_) : node(node_) {}

       public:
        iterator() : node(nullptr) {}

        T &operator*() const { return *node->data; }
        T *operator->() const { return node->data; }

        iterator &operator++() {
            node = node->next;
            return *this;
        }
        iterator operator++(int) {
            iterator old = *this;
            ++(*this);
            return old;
        }
        iterator &operator--() {
            node = node->prev;
            return *this;
        }
        iterator operator--(int) {
            iterator old = *this;
            --(*this);
            return old;
        }

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
        friend class list;

       private:
        Node *node;

        explicit const_iterator(Node *node_) : node(node_) {}

       public:
        const_iterator() : node(nullptr) {}
        explicit const_iterator(const iterator &other) : node(other.node) {}

        const T &operator*() const { return *node->data; }
        const T *operator->() const { return &operator*(); }

        const_iterator &operator++() {
            node = node->next;
            return *this;
        }
        const_iterator operator++(int) {
            const_iterator old = *this;
            ++(*this);
            return old;
        }
        const_iterator &operator--() {
            node = node->prev;
            return *this;
        }
        const_iterator operator--(int) {
            const_iterator old = *this;
            --(*this);
            return old;
        }

        bool operator==(const const_iterator &rhs) const {
            return node == rhs.node;
        }
        bool operator!=(const const_iterator &rhs) const {
            return !(*this == rhs);
        }
    };

    list() : sentinel(), sz(0) {}

    list(const list &other) : list() {
        for (const_iterator it = other.cbegin(); it != other.cend(); ++it) {
            push_back(*it);
        }
    }

    ~list() { clear(); }

    list &operator=(const list &other) {
        if (this == &other) return *this;
        clear();
        for (const_iterator it = other.cbegin(); it != other.cend(); ++it) {
            push_back(*it);
        }
        return *this;
    }

    iterator begin() { return iterator(sentinel.next); }
    const_iterator begin() const { return const_iterator(sentinel.next); }
    const_iterator cbegin() const {
        return const_iterator(sentinel.next);
    }

    iterator end() { return iterator(const_cast<Node *>(&sentinel)); }
    const_iterator end() const {
        return const_iterator(const_cast<Node *>(&sentinel));
    }
    const_iterator cend() const {
        return const_iterator(const_cast<Node *>(&sentinel));
    }

    bool empty() const { return sz == 0; }
    size_t size() const { return sz; }

    T &front() {
        if (empty()) throw container_is_empty{};
        return *sentinel.next->data;
    }
    const T &front() const {
        if (empty()) throw container_is_empty{};
        return *sentinel.next->data;
    }
    T &back() {
        if (empty()) throw container_is_empty{};
        return *sentinel.prev->data;
    }
    const T &back() const {
        if (empty()) throw container_is_empty{};
        return *sentinel.prev->data;
    }

    void clear() {
        Node *cur = sentinel.next;
        while (cur != &sentinel) {
            Node *next_node = cur->next;
            delete cur;
            cur = next_node;
        }
        sentinel.prev = sentinel.next = &sentinel;
        sz = 0;
    }

    iterator insert(iterator pos, const T &value) {
        Node *node = new Node(value);
        insert_node_before(pos.node, node);
        return iterator(node);
    }

    iterator erase(iterator pos) {
        return iterator(erase_node(pos.node));
    }

    void push_front(const T &value) { insert(begin(), value); }
    void push_back(const T &value) { insert(end(), value); }

    void pop_front() {
        if (empty()) throw container_is_empty{};
        erase(begin());
    }
    void pop_back() {
        if (empty()) throw container_is_empty{};
        iterator it = end();
        --it;
        erase(it);
    }

    void splice(iterator pos, list &other, iterator it) {
        if (!pos.node || !it.node || it.node == &other.sentinel) {
            throw invalid_iterator{};
        }
        if (this == &other &&
            (pos.node == it.node || pos.node == it.node->next)) {
            return;
        }

        Node *node = it.node;
        node->prev->next = node->next;
        node->next->prev = node->prev;
        --other.sz;

        node->prev = pos.node->prev;
        node->next = pos.node;
        pos.node->prev->next = node;
        pos.node->prev = node;
        ++sz;
    }
};

}  // namespace sjtu

#endif
