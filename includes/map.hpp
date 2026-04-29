/**
 * implement a container like std::map
 */
#ifndef SJTU_MAP_HPP
#define SJTU_MAP_HPP

#include <cstddef>
#include <functional>  // only for std::less<T>

#include "exceptions.hpp"
#include "utility.hpp"

namespace sjtu {

template <class Key, class T, class Compare = std::less<Key> >
class map {
public:
    /**
     * the internal type of data.
     * it should have a default constructor, a copy constructor.
     * You can use sjtu::map as value_type by typedef.
     */
    typedef pair<const Key, T> value_type;

private:
    enum class Color { RED, BLACK };

    struct Node {
        value_type data;
        Node* parent;
        Node* children[2];
        Color color;

        Node(const value_type& d, Node* p = nullptr, Color c = Color::RED)
            : data(d), parent(p), children{nullptr, nullptr}, color(c) {
        }
    };

    Node* root;
    Node *min_node, *max_node;
    size_t sz;
    Compare comp;

    inline int _get_dir(Node* x) {
        return x == x->parent->children[1];
    }

    Node* _copy_tree(Node* other_node, Node* parent) {
        if (!other_node) return nullptr;
        Node* node = nullptr;
        node = new Node(other_node->data, parent, other_node->color);
        node->children[0] = _copy_tree(other_node->children[0], node);
        node->children[1] = _copy_tree(other_node->children[1], node);
        return node;
    }
    void _destroy_tree(Node* n) {
        if (!n) return;
        _destroy_tree(n->children[0]);
        _destroy_tree(n->children[1]);
        delete n;
    }

    // rotate x->children[d] to upper.
    void _rotate(Node* x, int d) {
        Node* y = x->children[d];
        // edge 1: x -> x->children[d]
        x->children[d] = y->children[!d];
        if (y->children[!d]) {
            y->children[!d]->parent = x;
        }
        // edge 2: x->parent -> y
        y->parent = x->parent;
        if (!x->parent) {
            root = y;
        } else {
            x->parent->children[_get_dir(x)] = y;
        }
        // edge 3: y -> x
        y->children[!d] = x;
        x->parent = y;
    }

    void _insert_fixup(Node* node) {
        while (true) {
            Node* parent = node->parent;
            if (!parent) {
                node->color = Color::BLACK;
                break;
            }
            if (parent->color == Color::BLACK) {
                break;
            }
            Node* gparent = node->parent->parent;
            int d = _get_dir(parent);
            Node* uncle = gparent->children[!d];

            if (uncle && uncle->color == Color::RED) {
                parent->color = Color::BLACK;
                uncle->color = Color::BLACK;
                gparent->color = Color::RED;
                node = gparent;
            } else {
                if (_get_dir(node) != d) {  // rotate to same direction
                    node = node->parent;
                    _rotate(node, !d);
                    parent = node->parent;
                }
                parent->color = Color::BLACK;
                gparent->color = Color::RED;
                _rotate(gparent, d);
            }
        }
        root->color = Color::BLACK;
    }

    void _erase_fixup(Node* x, Node* p) {
        while (x != root && (!x || x->color == Color::BLACK)) {
            int d = (x == p->children[1]);  // here x mayby nullptr.
            Node* s = p->children[!d];      // sibling node

            if (s->color == Color::RED) {  // sibling is red, rotate
                s->color = Color::BLACK;
                p->color = Color::RED;
                _rotate(p, !d);
                s = p->children[!d];
            }
            // here sibling must be black.

            // Case 1: all children are black
            if ((!s->children[0] || s->children[0]->color == Color::BLACK) &&
                (!s->children[1] || s->children[1]->color == Color::BLACK)) {
                s->color = Color::RED;
                x = p;
                p = x->parent;
            }
            // Case 2: at least one red children
            else {
                if (!s->children[!d] ||
                    s->children[!d]->color == Color::BLACK) {
                    // the inner children is red, rotate to outer
                    if (s->children[d]) s->children[d]->color = Color::BLACK;
                    s->color = Color::RED;
                    _rotate(s, d);
                    s = p->children[!d];
                }
                s->color = p->color;
                p->color = Color::BLACK;
                if (s->children[!d]) s->children[!d]->color = Color::BLACK;
                _rotate(p, !d);
                break;
            }
        }
        if (x) {
            x->color = Color::BLACK;
        }
    }

    void _transplant(Node* u, Node* v) {
        if (!u->parent) {
            root = v;
        } else {
            u->parent->children[_get_dir(u)] = v;
        }
        if (v) {
            v->parent = u->parent;
        }
    }

public:
    /**
     * see BidirectionalIterator at CppReference for help.
     *
     * if there is anything wrong throw invalid_iterator.
     *     like it = map.begin(); --it;
     *       or it = map.end(); ++end();
     */
    class const_iterator;
    class iterator {
        friend class map;
        friend class const_iterator;

    private:
        map* m;
        Node* node;

    public:
        iterator(map* m_ = nullptr, Node* node_ = nullptr)
            : m(m_), node(node_) {
        }

        iterator operator++(int) {
            iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        iterator& operator++() {
            if (!node) throw invalid_iterator{};  // at end()
            if (node->children[1]) {
                node = node->children[1];
                while (node->children[0]) node = node->children[0];
            } else {
                Node* p = node->parent;
                while (p && node == p->children[1]) {
                    node = p;
                    p = p->parent;
                }
                node = p;
            }
            return *this;
        }

        iterator operator--(int) {
            iterator tmp = *this;
            --(*this);
            return tmp;
        }

        iterator& operator--() {
            if (!node) {                                       // at end()
                if (!m || !m->root) throw invalid_iterator{};  // is empty
                // otherwise, set this to the largest node
                node = m->max_node;
            } else if (node->children[0]) {
                node = node->children[0];
                while (node->children[1]) node = node->children[1];
            } else {
                Node* p = node->parent;
                while (p && node == p->children[0]) {
                    node = p;
                    p = p->parent;
                }
                if (!p) throw invalid_iterator{};  // no smaller element exists.
                node = p;
            }
            return *this;
        }

        value_type& operator*() const {
            if (!node) throw invalid_iterator{};
            return node->data;
        }

        /**
         * a operator to check whether two iterators are same (pointing to the
         * same memory).
         */
        bool operator==(const iterator& rhs) const {
            return m == rhs.m && node == rhs.node;
        }
        bool operator==(const const_iterator& rhs) const {
            return m == rhs.m && node == rhs.node;
        }

        /**
         * some other operator for iterator.
         */
        bool operator!=(const iterator& rhs) const {
            return !(*this == rhs);
        }
        bool operator!=(const const_iterator& rhs) const {
            return !(*this == rhs);
        }

        /**
         * for the support of it->first.
         * See
         * <http://kelvinh.github.io/blog/2013/11/20/overloading-of-member-access-operator-dash-greater-than-symbol-in-cpp/>
         * for help.
         */
        value_type* operator->() const noexcept {
            return node ? &(node->data) : nullptr;
        }
    };
    class const_iterator {
        friend class map;
        friend class iterator;
        // it should has similar member method as iterator.
        //  and it should be able to construct from an iterator.
    private:
        const map* m;
        const Node* node;

    public:
        const_iterator(const map* m_ = nullptr, const Node* node_ = nullptr)
            : m(m_), node(node_) {
        }
        const_iterator(const iterator& other) : m(other.m), node(other.node) {
        }

        const_iterator operator++(int) {
            const_iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        const_iterator& operator++() {
            if (!node) throw invalid_iterator();
            if (node->children[1]) {
                node = node->children[1];
                while (node->children[0]) node = node->children[0];
            } else {
                Node* p = node->parent;
                while (p && node == p->children[1]) {
                    node = p;
                    p = p->parent;
                }
                node = p;
            }
            return *this;
        }

        const_iterator operator--(int) {
            const_iterator tmp = *this;
            --(*this);
            return tmp;
        }

        const_iterator& operator--() {
            if (!node) {
                if (!m || !m->root) throw invalid_iterator();
                node = m->max_node;
            } else if (node->children[0]) {
                node = node->children[0];
                while (node->children[1]) node = node->children[1];
            } else {
                Node* p = node->parent;
                while (p && node == p->children[0]) {
                    node = p;
                    p = p->parent;
                }
                if (!p) throw invalid_iterator();
                node = p;
            }
            return *this;
        }

        const value_type& operator*() const {
            if (!node) throw invalid_iterator();
            return node->data;
        }

        bool operator==(const iterator& rhs) const {
            return m == rhs.m && node == rhs.node;
        }
        bool operator==(const const_iterator& rhs) const {
            return m == rhs.m && node == rhs.node;
        }
        bool operator!=(const iterator& rhs) const {
            return !(*this == rhs);
        }
        bool operator!=(const const_iterator& rhs) const {
            return !(*this == rhs);
        }

        const value_type* operator->() const noexcept {
            return node ? &(node->data) : nullptr;
        }
    };

    map() : root(nullptr), min_node(nullptr), max_node(nullptr), sz(0) {
    }

    map(const map& other)
        : root(nullptr),
          min_node(nullptr),
          max_node(nullptr),
          sz(0),
          comp(other.comp) {  // here we still use other.comp even if its
                              // useless in current scenario
        if (other.root) {
            root = _copy_tree(other.root, nullptr);
            sz = other.sz;
            min_node = root;
            while (min_node->children[0]) min_node = min_node->children[0];
            max_node = root;
            while (max_node->children[1]) max_node = max_node->children[1];
        }
    }

    map& operator=(const map& other) {
        if (this == &other) {
            return *this;
        }
        clear();
        comp = other.comp;  // here we still use other.comp even if its useless
                            // in current scenario
        if (other.root) {
            root = _copy_tree(other.root, nullptr);
            sz = other.sz;
            min_node = root;
            while (min_node->children[0]) min_node = min_node->children[0];
            max_node = root;
            while (max_node->children[1]) max_node = max_node->children[1];
        }
        return *this;
    }

    ~map() {
        clear();
    }

    /**
     * access specified element with bounds checking
     * Returns a reference to the mapped value of the element with key
     * equivalent to key. If no such element exists, an exception of type
     * `index_out_of_bound'
     */
    T& at(const Key& key) {
        iterator it = find(key);
        if (it == end()) throw index_out_of_bound{};
        return it->second;
    }

    const T& at(const Key& key) const {
        const_iterator it = find(key);
        if (it == cend()) throw index_out_of_bound{};
        return it->second;
    }

    /**
     * access specified element
     * Returns a reference to the value that is mapped to a key equivalent to
     * key, performing an insertion if such key does not already exist.
     */
    T& operator[](const Key& key) {
        iterator it = find(key);
        if (it != end()) {
            return it->second;
        }
        // here we require T is DefaultConstructible according to cpp standard.
        pair<iterator, bool> res = insert(value_type(key, T{}));
        return res.first->second;
    }

    /**
     * behave like at() throw index_out_of_bound if such key does not exist.
     * note that this behaves diffrently from std::map.
     */
    const T& operator[](const Key& key) const {
        const_iterator it = find(key);
        if (it == cend()) {
            throw index_out_of_bound{};
        }
        return it->second;
    }

    /**
     * return a iterator to the beginning
     */
    iterator begin() {
        return iterator(this, min_node);
    }

    const_iterator cbegin() const {
        return const_iterator(this, min_node);
    }

    /**
     * return a iterator to the end
     * in fact, it returns past-the-end.
     */
    iterator end() {
        return iterator{this, nullptr};
    }

    const_iterator cend() const {
        return const_iterator{this, nullptr};
    }

    /**
     * checks whether the container is empty
     * return true if empty, otherwise false.
     */
    bool empty() const {
        return sz == 0;
    }

    /**
     * returns the number of elements.
     */
    size_t size() const {
        return sz;
    }

    /**
     * clears the contents
     */
    void clear() {
        _destroy_tree(root);
        root = nullptr;
        min_node = nullptr;
        max_node = nullptr;
        sz = 0;
    }

    /**
     * insert an element.
     * return a pair, the first of the pair is
     *   the iterator to the new element (or the element that prevented the
     * insertion), the second one is true if insert successfully, or false.
     */
    pair<iterator, bool> insert(const value_type& value) {
        Node* y = nullptr;
        Node* x = root;
        int dir = 0;
        while (x) {
            y = x;
            if (comp(value.first, x->data.first)) {
                dir = 0;
                x = x->children[0];
            } else if (comp(x->data.first, value.first)) {
                dir = 1;
                x = x->children[1];
            } else {
                // same element
                return pair<iterator, bool>{iterator{this, x}, false};
            }
        }
        Node* node = new Node(value, y, Color::RED);
        if (!y) {  // the tree was empty before insert
            root = node;
            min_node = node;
            max_node = node;
        } else {
            y->children[dir] = node;
            if (dir == 0 && y == min_node) {
                min_node = node;  // node is the smallest node
            }
            if (dir == 1 && y == max_node) {
                max_node = node;  // node is the biggest node
            }
        }
        ++sz;
        _insert_fixup(node);
        return pair<iterator, bool>{iterator{this, node}, true};
    }

    /**
     * erase the element at pos.
     *
     * throw if pos pointed to a bad element (pos == this->end() || pos points
     * an element out of this)
     */
    void erase(iterator pos) {
        if (pos.m != this || !pos.node) throw invalid_iterator();

        if (pos.node == min_node) {
            iterator next_it = pos;
            ++next_it;
            min_node = next_it.node;
        }
        if (pos.node == max_node) {
            iterator prev_it = pos;
            if (!min_node || prev_it == begin()) {
                max_node = nullptr;
            } else {
                --prev_it;
                max_node = prev_it.node;
            }
        }

        Node* node = pos.node;
        Color col = node->color;
        Node* d = nullptr;  // the possible double black node
        Node* p = nullptr;  // its parent

        if (!node->children[0]) {
            d = node->children[1];
            p = node->parent;
            _transplant(node, node->children[1]);
        } else if (!node->children[1]) {
            d = node->children[0];
            p = node->parent;
            _transplant(node, node->children[0]);
        } else {
            Node* suf = node->children[1];
            while (suf->children[0]) suf = suf->children[0];
            col = suf->color;

            // swap the pos of suf and node
            d = suf->children[1];

            if (suf->parent == node) {
                p = suf;
            } else {
                p = suf->parent;
                _transplant(suf, suf->children[1]);
                suf->children[1] = node->children[1];
                suf->children[1]->parent = suf;
            }
            _transplant(node, suf);
            suf->children[0] = node->children[0];
            suf->children[0]->parent = suf;
            suf->color = node->color;
        }
        if (col == Color::BLACK) {
            _erase_fixup(d, p);
        }
        sz--;
        delete node;
    }

    /**
     * Returns the number of elements with key
     *   that compares equivalent to the specified argument,
     *   which is either 1 or 0
     *     since this container does not allow duplicates.
     * The default method of check the equivalence is !(a < b || b > a)
     */
    size_t count(const Key& key) const {
        return find(key) == cend() ? 0 : 1;
    }

    /**
     * Finds an element with key equivalent to key.
     * key value of the element to search for.
     * Iterator to an element with key equivalent to key.
     *   If no such element is found, past-the-end (see end()) iterator is
     * returned.
     */
    iterator find(const Key& key) {
        Node* current = root;
        while (current) {
            if (comp(key, current->data.first)) {
                current = current->children[0];
            } else if (comp(current->data.first, key)) {
                current = current->children[1];
            } else {
                return iterator{this, current};
            }
        }
        return end();
    }

    const_iterator find(const Key& key) const {
        Node* current = root;
        while (current) {
            if (comp(key, current->data.first)) {
                current = current->children[0];
            } else if (comp(current->data.first, key)) {
                current = current->children[1];
            } else {
                return const_iterator{this, current};
            }
        }
        return cend();
    }
};

}  // namespace sjtu

#endif
