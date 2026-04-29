#include <filesystem>
#include <fstream>
#include <string>

#include "vector.hpp"

template <class Key, class T>
class BPlusTree {
    static constexpr int PAGE_SIZE = 4096;

    struct KeyValuePair {
        Key first;
        T second;
        friend bool operator<(const KeyValuePair &lhs,
                              const KeyValuePair &rhs) {
            if (lhs.first < rhs.first) return true;
            if (rhs.first < lhs.first) return false;
            return lhs.second < rhs.second;
        };
        friend bool operator==(const KeyValuePair &lhs,
                               const KeyValuePair &rhs) {
            return !(lhs < rhs) && !(rhs < lhs);
        };
    };

    static constexpr int M =
        (PAGE_SIZE - 16) / (sizeof(KeyValuePair) + sizeof(int));
    static constexpr int L = (PAGE_SIZE - 16) / (sizeof(KeyValuePair));
    struct InternalPage {
        int is_leaf{0};
        int size;
        int parent;
        KeyValuePair data[M];
        int children[M + 1];
    };
    struct LeafPage {
        int is_leaf{1};
        int size;
        int parent;
        int next;
        KeyValuePair data[L];
    };
    struct MetaData {
        int free_head{};
        int page_used{};
        int root{};
    } metadata;

    static_assert(sizeof(InternalPage) <= PAGE_SIZE);
    static_assert(sizeof(LeafPage) <= PAGE_SIZE);

    std::fstream file;

    // insert x in to the sorted arr[0..len-1]
    // return the index of x
    template <class U>
    int insert_val(U arr[], int &len, const U &x) {
        int pos = len;
        while (pos > 0 && x < arr[pos - 1]) {
            arr[pos] = arr[pos - 1];
            --pos;
        }
        arr[pos] = x;
        ++len;
        return pos;
    }

    template <class U>
    int find_val(U arr[], int len, const U &x) {
        for (int i = 0; i < len; ++i) {
            if (arr[i] == x) {
                return i;
            }
        }
        return -1;
    }

    template <class U>
    bool erase_val(U arr[], int &len, const U &x) {
        int pos = 0;
        while (pos < len && arr[pos] < x) {
            ++pos;
        }
        if (pos == len || x < arr[pos]) {
            return false;
        }
        for (int i = pos + 1; i < len; ++i) {
            arr[i - 1] = arr[i];
        }
        --len;
        return true;
    }

    void read_metadata() {
        file.seekg(0);
        file.read(reinterpret_cast<char *>(&metadata), sizeof(MetaData));
    }
    void write_metadata() {
        file.seekp(0);
        file.write(reinterpret_cast<const char *>(&metadata), sizeof(metadata));
    }
    int get_addr(long idx) { return (idx - 1) * PAGE_SIZE + sizeof(MetaData); }

    // obtain an unused idx for R/W
    int new_free_page() {
        if (metadata.free_head) {
            int idx = metadata.free_head;
            file.seekg(get_addr(metadata.free_head));
            file.read(reinterpret_cast<char *>(&metadata.free_head),
                      sizeof(int));
            return idx;
        } else {
            return ++metadata.page_used;
        }
    };
    // mark idx as an unused page
    void erase_page(int idx) {
        file.seekp(get_addr(idx));
        file.write(reinterpret_cast<const char *>(&metadata.free_head),
                   sizeof(int));
        metadata.free_head = idx;
    }

    int is_leaf_page(int idx) {
        int is_leaf;
        file.seekg(get_addr(idx));
        file.read(reinterpret_cast<char *>(&is_leaf), sizeof(int));
        return is_leaf;
    };

    template <class U>
    U read_page(int idx) {
        U page;
        file.seekg(get_addr(idx));
        file.read(reinterpret_cast<char *>(&page), sizeof(U));
        return page;
    }
    template <class U>
    void write_page(int idx, const U &page) {
        file.seekp(get_addr(idx));
        file.write(reinterpret_cast<const char *>(&page), sizeof(U));
    }

    int get_leaf_idx(const KeyValuePair &kv_pair) {
        int idx = metadata.root;
        while (!is_leaf_page(idx)) {
            InternalPage cur_page = read_page<InternalPage>(idx);
            int child = cur_page.size;
            for (int i = 0; i < cur_page.size; ++i) {
                if (kv_pair < cur_page.data[i]) {
                    child = i;
                    break;
                }
            }
            idx = cur_page.children[child];
        }
        return idx;
    }

    void set_parent(int idx, int p) {
        if (is_leaf_page(idx)) {
            LeafPage page = read_page<LeafPage>(idx);
            page.parent = p;
            write_page(idx, page);
        } else {
            InternalPage page = read_page<InternalPage>(idx);
            page.parent = p;
            write_page(idx, page);
        }
    };
    void insert_internal(int idx, const KeyValuePair &kv_pair, int left_idx,
                         int right_idx) {
        if (!idx) {
            InternalPage root_page;
            metadata.root = new_free_page();
            root_page.size = 1;
            root_page.parent = 0;
            root_page.children[0] = left_idx;
            root_page.children[1] = right_idx;
            root_page.data[0] = kv_pair;

            set_parent(left_idx, metadata.root);
            set_parent(right_idx, metadata.root);
            write_page(metadata.root, root_page);
            return;
        }

        InternalPage page = read_page<InternalPage>(idx);
        if (page.size < M) {
            int i = insert_val(page.data, page.size, kv_pair);
            for (int j = page.size - 1; j > i; j--) {
                page.children[j + 1] = page.children[j];
            }
            page.children[i] = left_idx;
            page.children[i + 1] = right_idx;
            write_page(idx, page);
        } else {
            KeyValuePair temp_data[M + 1];
            int temp_children[M + 2];
            for (int i = 0; i < M; i++) {
                temp_data[i] = page.data[i];
            }
            int len = M;
            int pos = insert_val(temp_data, len, kv_pair);
            for (int i = 0; i <= M; i++) {
                temp_children[i] = page.children[i];
            }
            for (int i = M; i > pos; i--) {
                temp_children[i + 1] = temp_children[i];
            }
            temp_children[pos] = left_idx;
            temp_children[pos + 1] = right_idx;

            int new_idx = new_free_page();
            InternalPage new_page;
            new_page.parent = page.parent;
            new_page.size = (M + 1) / 2;
            page.size = M - new_page.size;

            for (int i = 0; i < page.size; i++) {
                page.data[i] = temp_data[i];
            }
            for (int i = 0; i <= page.size; i++) {
                // here we don't set parent because they are already correct.
                page.children[i] = temp_children[i];
            }
            for (int i = 0; i < new_page.size; i++) {
                new_page.data[i] = temp_data[page.size + 1 + i];
            }
            for (int i = 0; i <= new_page.size; ++i) {
                new_page.children[i] = temp_children[page.size + 1 + i];
                set_parent(new_page.children[i], new_idx);
            }

            write_page(idx, page);
            write_page(new_idx, new_page);
            insert_internal(page.parent, temp_data[page.size], idx, new_idx);
        }
    }

    // update the minimum of idx to kv_pair
    void update_first_key(int idx, int parent_idx,
                          const KeyValuePair &kv_pair) {
        while (parent_idx) {
            InternalPage parent_page = read_page<InternalPage>(parent_idx);
            int child = 0;
            while (parent_page.children[child] != idx) {
                ++child;
            }
            if (child) {
                parent_page.data[child - 1] = kv_pair;
                write_page(parent_idx, parent_page);
                return;
            }
            idx = parent_idx;
            parent_idx = parent_page.parent;
        }
    }

    // requirements: del_ch_pos is not the first child (we always merge right
    // into left)
    void erase_internal(int idx, int del_ch_pos) {
        InternalPage page = read_page<InternalPage>(idx);
        if (page.parent == 0) {  // Case 1: root
            for (int i = del_ch_pos + 1; i <= page.size; ++i) {
                page.children[i - 1] = page.children[i];
            }
            int data_pos = del_ch_pos - 1;
            for (int i = data_pos + 1; i < page.size; ++i) {
                page.data[i - 1] = page.data[i];
            }
            --page.size;
            if (page.size) {
                write_page(idx, page);
            } else {
                metadata.root = page.children[0];
                set_parent(metadata.root, 0);
                erase_page(idx);
            }
        } else if (page.size > M / 2) {  // Case 2: no underflow
            for (int i = del_ch_pos + 1; i <= page.size; ++i) {
                page.children[i - 1] = page.children[i];
            }
            int data_pos = del_ch_pos - 1;
            for (int i = data_pos + 1; i < page.size; ++i) {
                page.data[i - 1] = page.data[i];
            }
            --page.size;
            write_page(idx, page);
        } else {  // Case 3: underflow
            InternalPage parent_page = read_page<InternalPage>(page.parent);
            int self_pos =
                find_val(parent_page.children, parent_page.size + 1, idx);

            if (self_pos > 0) {  // left sibling
                int sib_idx = parent_page.children[self_pos - 1];
                InternalPage sib_page = read_page<InternalPage>(sib_idx);
                // Case 3.1 (L): left sibling no underflow
                if (sib_page.size > M / 2) {
                    int borrowed_child = sib_page.children[sib_page.size];
                    KeyValuePair s_key = sib_page.data[sib_page.size - 1];
                    KeyValuePair p_key = parent_page.data[self_pos - 1];

                    for (int i = del_ch_pos - 1; i >= 0; --i) {
                        page.children[i + 1] = page.children[i];
                    }
                    page.children[0] = borrowed_child;
                    for (int i = del_ch_pos - 2; i >= 0; --i) {
                        page.data[i + 1] = page.data[i];
                    }
                    page.data[0] = p_key;

                    --sib_page.size;
                    parent_page.data[self_pos - 1] = s_key;
                    set_parent(borrowed_child, idx);
                    write_page(idx, page);
                    write_page(sib_idx, sib_page);
                    write_page(page.parent, parent_page);
                }
                // Case 3.2 (L): left sibling underflow, merge
                else {
                    int pos = sib_page.size;
                    sib_page.data[pos++] = parent_page.data[self_pos - 1];
                    for (int i = 0; i < page.size; ++i) {
                        if (i == del_ch_pos - 1) continue;
                        sib_page.data[pos++] = page.data[i];
                    }
                    int child_pos = sib_page.size + 1;
                    for (int i = 0; i <= page.size; ++i) {
                        if (i == del_ch_pos) continue;
                        sib_page.children[child_pos++] = page.children[i];
                        set_parent(page.children[i], sib_idx);
                    }
                    sib_page.size = pos;
                    write_page(sib_idx, sib_page);
                    erase_page(idx);
                    erase_internal(sib_page.parent, self_pos);
                }
            } else {
                int sib_idx = parent_page.children[self_pos + 1];
                InternalPage sib_page = read_page<InternalPage>(sib_idx);
                // Case 3.1 (R): right sibling no underflow
                if (sib_page.size > M / 2) {
                    int borrowed_child = sib_page.children[0];
                    KeyValuePair s_key = sib_page.data[0];
                    KeyValuePair p_key = parent_page.data[self_pos];

                    for (int i = del_ch_pos + 1; i <= page.size; ++i) {
                        page.children[i - 1] = page.children[i];
                    }
                    int data_pos = del_ch_pos - 1;
                    for (int i = data_pos + 1; i < page.size; ++i) {
                        page.data[i - 1] = page.data[i];
                    }
                    page.data[page.size - 1] = p_key;
                    page.children[page.size] = borrowed_child;

                    for (int i = 1; i <= sib_page.size; ++i) {
                        sib_page.children[i - 1] = sib_page.children[i];
                    }
                    for (int i = 1; i < sib_page.size; ++i) {
                        sib_page.data[i - 1] = sib_page.data[i];
                    }
                    --sib_page.size;
                    parent_page.data[self_pos] = s_key;
                    set_parent(borrowed_child, idx);
                    write_page(idx, page);
                    write_page(sib_idx, sib_page);
                    write_page(page.parent, parent_page);
                }
                // Case 3.2 (R): right sibling underflow, merge
                else {
                    int pos = page.size - 1;
                    for (int i = del_ch_pos; i < page.size; ++i) {
                        page.children[i] = page.children[i + 1];
                    }
                    int data_pos = del_ch_pos - 1;
                    for (int i = data_pos + 1; i < page.size; ++i) {
                        page.data[i - 1] = page.data[i];
                    }

                    page.data[pos++] = parent_page.data[self_pos];
                    for (int i = 0; i < sib_page.size; ++i) {
                        page.data[pos++] = sib_page.data[i];
                    }
                    int child_pos = page.size;
                    for (int i = 0; i <= sib_page.size; ++i) {
                        page.children[child_pos++] = sib_page.children[i];
                        set_parent(sib_page.children[i], idx);
                    }
                    page.size = pos;
                    write_page(idx, page);
                    erase_page(sib_idx);
                    erase_internal(page.parent, self_pos + 1);
                }
            }
        }
    }

   public:
    explicit BPlusTree(const std::string &filename) {
        if (std::filesystem::exists(filename)) {
            file.open(filename,
                      std::ios::in | std::ios::out | std::ios::binary);
            read_metadata();
        } else {
            file.open(filename, std::ios::in | std::ios::out |
                                    std::ios::binary | std::ios::trunc);
            metadata = MetaData{};
            write_metadata();
        }
    };
    ~BPlusTree() {
        write_metadata();
        file.close();
    }

    BPlusTree(const BPlusTree &) = delete;
    BPlusTree &operator=(const BPlusTree &) = delete;

    // insert (key, value) in to tree
    // requirement: (key, value) does not exist
    void insert(const Key &key, const T &value) {
        auto kv_pair = KeyValuePair{key, value};

        if (!metadata.root) {
            metadata.root = new_free_page();
            LeafPage page;
            page.size = 1;
            page.parent = 0;
            page.next = 0;
            page.data[0] = kv_pair;
            write_page(metadata.root, page);
            return;
        }
        int idx = get_leaf_idx(kv_pair);
        LeafPage leaf_page = read_page<LeafPage>(idx);
        if (leaf_page.size < L) {
            insert_val(leaf_page.data, leaf_page.size, kv_pair);
            write_page(idx, leaf_page);
            return;
        }
        KeyValuePair temp_data[L + 1];
        for (int i = 0; i < L; ++i) {
            temp_data[i] = leaf_page.data[i];
        }

        int len = L;
        insert_val(temp_data, len, kv_pair);

        LeafPage new_page;
        int new_idx = new_free_page();
        new_page.parent = leaf_page.parent;
        new_page.next = leaf_page.next;
        leaf_page.next = new_idx;
        new_page.size = (L + 1) / 2;
        leaf_page.size = (L + 1) - new_page.size;
        for (int i = 0; i < leaf_page.size; ++i) {
            leaf_page.data[i] = temp_data[i];
        }
        for (int i = 0; i < new_page.size; ++i) {
            new_page.data[i] = temp_data[leaf_page.size + i];
        }
        write_page(idx, leaf_page);
        write_page(new_idx, new_page);
        insert_internal(leaf_page.parent, new_page.data[0], idx, new_idx);
    }
    // erase (key, value) in the tree
    // if (key, value) does not exist before, do nothing
    void erase(const Key &key, const T &value) {
        if (!metadata.root) {
            return;
        }

        auto kv_pair = KeyValuePair{key, value};
        int idx = get_leaf_idx(kv_pair);
        LeafPage leaf_page = read_page<LeafPage>(idx);

        // erase the corresponding element and update separators
        if (leaf_page.data[0] == kv_pair) {
            erase_val(leaf_page.data, leaf_page.size, kv_pair);
            update_first_key(idx, leaf_page.parent, leaf_page.data[0]);
        } else if (!erase_val(leaf_page.data, leaf_page.size, kv_pair)) {
            return;
        }

        if (!leaf_page.parent) {  // Case 1: root
            if (leaf_page.size) {
                write_page(idx, leaf_page);
            } else {
                metadata.root = 0;
                erase_page(idx);
            }
        } else if (leaf_page.size > (L + 1) / 2) {  // Case 2: no underflow
            write_page(idx, leaf_page);
        } else {  // Case 3: underflow
            InternalPage parent_page =
                read_page<InternalPage>(leaf_page.parent);
            int self_pos =
                find_val(parent_page.children, parent_page.size + 1, idx);

            if (self_pos > 0) {  // left sibling
                int sib_idx = parent_page.children[self_pos - 1];
                LeafPage sib_page = read_page<LeafPage>(sib_idx);
                // Case 3.1 (L): left sibling no underflow
                if (sib_page.size > (L + 1) / 2) {
                    for (int i = leaf_page.size - 1; i >= 0; --i) {
                        leaf_page.data[i + 1] = leaf_page.data[i];
                    }
                    leaf_page.data[0] = sib_page.data[--sib_page.size];
                    ++leaf_page.size;
                    update_first_key(idx, leaf_page.parent, leaf_page.data[0]);
                    write_page(idx, leaf_page);
                    write_page(sib_idx, sib_page);
                }
                // Case 3.2 (L): left sibling underflow, merge
                else {
                    for (int i = 0; i < leaf_page.size; i++) {
                        sib_page.data[sib_page.size++] = leaf_page.data[i];
                    }
                    sib_page.next = leaf_page.next;
                    write_page(sib_idx, sib_page);
                    erase_page(idx);
                    erase_internal(sib_page.parent, self_pos);
                }
            } else {
                int sib_idx = parent_page.children[self_pos + 1];
                LeafPage sib_page = read_page<LeafPage>(sib_idx);
                // Case 3.1 (R): right sibling no underflow
                if (sib_page.size > (L + 1) / 2) {
                    leaf_page.data[leaf_page.size++] = sib_page.data[0];
                    for (int i = 1; i < sib_page.size; i++) {
                        sib_page.data[i - 1] = sib_page.data[i];
                    }
                    --sib_page.size;
                    update_first_key(sib_idx, sib_page.parent,
                                     sib_page.data[0]);
                    write_page(idx, leaf_page);
                    write_page(sib_idx, sib_page);
                }
                // Case 3.2 (R): right sibling underflow, merge
                else {
                    for (int i = 0; i < sib_page.size; i++) {
                        leaf_page.data[leaf_page.size++] = sib_page.data[i];
                    }
                    leaf_page.next = sib_page.next;
                    write_page(idx, leaf_page);
                    erase_page(sib_idx);
                    erase_internal(leaf_page.parent, self_pos + 1);
                }
            }
        }
    }

    // find all entries with the given key, sorted by values
    sjtu::vector<T> find_all(const Key &key) {
        sjtu::vector<T> result;
        if (!metadata.root) {
            return result;
        }

        int idx = metadata.root;
        while (!is_leaf_page(idx)) {
            InternalPage page = read_page<InternalPage>(idx);
            int child = page.size;
            for (int i = 0; i < page.size; ++i) {
                if (!(page.data[i].first < key)) {
                    child = i;
                    break;
                }
            }
            idx = page.children[child];
        }

        while (idx) {
            LeafPage page = read_page<LeafPage>(idx);
            for (int i = 0; i < page.size; ++i) {
                if (page.data[i].first < key) {
                    continue;
                }
                if (key < page.data[i].first) {
                    return result;
                }
                result.push_back(page.data[i].second);
            }
            idx = page.next;
        }
        return result;
    }
};
