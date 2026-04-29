#ifndef B_PLUS_TREE_HPP
#define B_PLUS_TREE_HPP

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
        (PAGE_SIZE - 12) / (sizeof(KeyValuePair) + sizeof(int));
    static constexpr int L = (PAGE_SIZE - 12) / (sizeof(KeyValuePair));
    struct InternalPage {
        int is_leaf{0};
        int size;
        KeyValuePair data[M];
        int children[M + 1];
    };
    struct LeafPage {
        int is_leaf{1};
        int size;
        int next;
        KeyValuePair data[L];
    };
    struct PathEntry {
        int idx;
        int ch_pos;
    };
    struct MetaData {
        int free_head{};
        int page_used{};
        int root{};
    } metadata;

    static_assert(sizeof(InternalPage) <= PAGE_SIZE);
    static_assert(sizeof(LeafPage) <= PAGE_SIZE);
    static_assert(M >= 3);
    static_assert(L >= 3);

    std::fstream file;

    template <class U>
    void move_right(U arr[], int first, int last) {
        for (int i = last; i > first; i--) {
            arr[i] = arr[i - 1];
        }
    }

    template <class U>
    void move_left(U arr[], int first, int last) {
        for (int i = first + 1; i < last; i++) {
            arr[i - 1] = arr[i];
        }
    }

    template <class U>
    void copy_range(U dst[], int dst_first, const U src[], int src_first,
                    int count) {
        for (int i = 0; i < count; i++) {
            dst[dst_first + i] = src[src_first + i];
        }
    }

    // insert x in to the sorted arr[0..len-1]
    // return the index of x
    template <class U>
    int insert_val(U arr[], int &len, const U &x) {
        int left = 0, right = len;
        while (left < right) {
            int mid = (left + right) / 2;
            if (x < arr[mid]) {
                right = mid;
            } else {
                left = mid + 1;
            }
        }
        int pos = left;
        move_right(arr, pos, len);
        arr[pos] = x;
        ++len;
        return pos;
    }

    // arr must be sorted!
    template <class U>
    bool erase_val(U arr[], int &len, const U &x) {
        int left = 0, right = len;
        while (left < right) {
            int mid = (left + right) / 2;
            if (arr[mid] < x) {
                left = mid + 1;
            } else {
                right = mid;
            }
        }
        int pos = left;
        if (pos == len || x < arr[pos]) {
            return false;
        }
        move_left(arr, pos, len);
        --len;
        return true;
    }

    template <class U>
    int find_val(U arr[], int len, const U &x) {
        for (int i = 0; i < len; i++) {
            if (arr[i] == x) {
                return i;
            }
        }
        return -1;
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

    InternalPage read_internal_after_type(int is_leaf) {
        InternalPage page;
        page.is_leaf = is_leaf;
        file.read(reinterpret_cast<char *>(&page) + sizeof(int),
                  sizeof(InternalPage) - sizeof(int));
        return page;
    }

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

    int get_leaf_idx(const KeyValuePair &kv_pair, PathEntry path[],
                     int &path_size) {
        path_size = 0;
        int idx = metadata.root;
        while (true) {
            int is_leaf = is_leaf_page(idx);
            if (is_leaf) {
                break;
            }
            InternalPage cur_page = read_internal_after_type(is_leaf);
            int left = 0, right = cur_page.size;
            while (left < right) {
                int mid = (left + right) / 2;
                if (kv_pair < cur_page.data[mid]) {
                    right = mid;
                } else {
                    left = mid + 1;
                }
            }
            int child = left;
            path[path_size++] = {idx, child};
            idx = cur_page.children[child];
        }
        return idx;
    }

    void insert_internal_recursive(PathEntry path[], int path_pos,
                                   const KeyValuePair &kv_pair, int left_idx,
                                   int right_idx) {
        if (path_pos < 0) {  // Case 1: root
            InternalPage root_page;
            metadata.root = new_free_page();
            root_page.size = 1;
            root_page.children[0] = left_idx;
            root_page.children[1] = right_idx;
            root_page.data[0] = kv_pair;
            write_page(metadata.root, root_page);
            return;
        }

        int idx = path[path_pos].idx;
        int ch_pos = path[path_pos].ch_pos;
        InternalPage page = read_page<InternalPage>(idx);
        // Case 2: no overflow
        if (page.size < M) {
            move_right(page.data, ch_pos, page.size);
            move_right(page.children, ch_pos + 1, page.size + 1);
            page.data[ch_pos] = kv_pair;
            page.children[ch_pos] = left_idx;
            page.children[ch_pos + 1] = right_idx;
            ++page.size;
            write_page(idx, page);
            return;
        }
        // Case 3: overflow, split
        KeyValuePair temp_data[M + 1];
        int temp_children[M + 2];
        copy_range(temp_data, 0, page.data, 0, ch_pos);
        temp_data[ch_pos] = kv_pair;
        copy_range(temp_data, ch_pos + 1, page.data, ch_pos, M - ch_pos);

        copy_range(temp_children, 0, page.children, 0, ch_pos + 1);
        temp_children[ch_pos] = left_idx;
        temp_children[ch_pos + 1] = right_idx;
        copy_range(temp_children, ch_pos + 2, page.children, ch_pos + 1,
                   M - ch_pos);

        int new_idx = new_free_page();
        InternalPage new_page;
        new_page.size = (M + 1) / 2;
        page.size = M - new_page.size;

        copy_range(page.data, 0, temp_data, 0, page.size);
        copy_range(page.children, 0, temp_children, 0, page.size + 1);
        copy_range(new_page.data, 0, temp_data, page.size + 1,
                   new_page.size);
        copy_range(new_page.children, 0, temp_children, page.size + 1,
                   new_page.size + 1);

        write_page(idx, page);
        write_page(new_idx, new_page);
        insert_internal_recursive(path, path_pos - 1, temp_data[page.size], idx,
                                  new_idx);
    }

    void insert_internal(PathEntry path[], int path_size, KeyValuePair kv_pair,
                         int left_idx, int right_idx) {
        insert_internal_recursive(path, path_size - 1, kv_pair, left_idx,
                                  right_idx);
    }

    // update the minimum of idx to kv_pair
    void update_first_key(PathEntry path[], int path_size,
                          const KeyValuePair &kv_pair) {
        for (int i = path_size - 1; i >= 0; i--) {
            if (path[i].ch_pos) {
                InternalPage page = read_page<InternalPage>(path[i].idx);
                page.data[path[i].ch_pos - 1] = kv_pair;
                write_page(path[i].idx, page);
                return;
            }
        }
    }

    void remove_child(InternalPage &page, int del_ch_pos) {
        move_left(page.children, del_ch_pos, page.size + 1);
        int data_pos = del_ch_pos - 1;
        move_left(page.data, data_pos, page.size);
        --page.size;
    }

    // requirements: del_ch_pos is not the first child (we always merge right
    // into left)
    void erase_internal(PathEntry path[], int path_pos, int del_ch_pos) {
        int idx = path[path_pos].idx;
        InternalPage page = read_page<InternalPage>(idx);
        if (path_pos == 0) {  // Case 1: root
            remove_child(page, del_ch_pos);
            if (page.size) {
                write_page(idx, page);
            } else {
                metadata.root = page.children[0];
                erase_page(idx);
            }
        } else if (page.size > M / 2) {  // Case 2: no underflow
            remove_child(page, del_ch_pos);
            write_page(idx, page);
        } else {  // Case 3: underflow
            InternalPage parent_page =
                read_page<InternalPage>(path[path_pos - 1].idx);
            int self_pos = path[path_pos - 1].ch_pos;

            if (self_pos > 0) {  // left sibling
                int sib_idx = parent_page.children[self_pos - 1];
                InternalPage sib_page = read_page<InternalPage>(sib_idx);
                // Case 3.1 (L): left sibling no underflow
                if (sib_page.size > M / 2) {
                    int borrowed_child = sib_page.children[sib_page.size];
                    KeyValuePair s_key = sib_page.data[sib_page.size - 1];
                    KeyValuePair p_key = parent_page.data[self_pos - 1];

                    move_right(page.children, 0, del_ch_pos);
                    page.children[0] = borrowed_child;
                    move_right(page.data, 0, del_ch_pos - 1);
                    page.data[0] = p_key;

                    --sib_page.size;
                    parent_page.data[self_pos - 1] = s_key;
                    write_page(idx, page);
                    write_page(sib_idx, sib_page);
                    write_page(path[path_pos - 1].idx, parent_page);
                }
                // Case 3.2 (L): left sibling underflow, merge
                else {
                    int pos = sib_page.size;
                    sib_page.data[pos++] = parent_page.data[self_pos - 1];
                    for (int i = 0; i < page.size; i++) {
                        if (i == del_ch_pos - 1) continue;
                        sib_page.data[pos++] = page.data[i];
                    }
                    int child_pos = sib_page.size + 1;
                    for (int i = 0; i <= page.size; i++) {
                        if (i == del_ch_pos) continue;
                        sib_page.children[child_pos++] = page.children[i];
                    }
                    sib_page.size = pos;
                    write_page(sib_idx, sib_page);
                    erase_page(idx);
                    erase_internal(path, path_pos - 1, self_pos);
                }
            } else {
                int sib_idx = parent_page.children[self_pos + 1];
                InternalPage sib_page = read_page<InternalPage>(sib_idx);
                // Case 3.1 (R): right sibling no underflow
                if (sib_page.size > M / 2) {
                    int borrowed_child = sib_page.children[0];
                    KeyValuePair s_key = sib_page.data[0];
                    KeyValuePair p_key = parent_page.data[self_pos];

                    move_left(page.children, del_ch_pos, page.size + 1);
                    int data_pos = del_ch_pos - 1;
                    move_left(page.data, data_pos, page.size);
                    page.data[page.size - 1] = p_key;
                    page.children[page.size] = borrowed_child;

                    move_left(sib_page.children, 0, sib_page.size + 1);
                    move_left(sib_page.data, 0, sib_page.size);
                    --sib_page.size;
                    parent_page.data[self_pos] = s_key;
                    write_page(idx, page);
                    write_page(sib_idx, sib_page);
                    write_page(path[path_pos - 1].idx, parent_page);
                }
                // Case 3.2 (R): right sibling underflow, merge
                else {
                    int pos = page.size - 1;
                    move_left(page.children, del_ch_pos, page.size + 1);
                    int data_pos = del_ch_pos - 1;
                    move_left(page.data, data_pos, page.size);

                    page.data[pos++] = parent_page.data[self_pos];
                    for (int i = 0; i < sib_page.size; i++) {
                        page.data[pos++] = sib_page.data[i];
                    }
                    int child_pos = page.size;
                    for (int i = 0; i <= sib_page.size; i++) {
                        page.children[child_pos++] = sib_page.children[i];
                    }
                    page.size = pos;
                    write_page(idx, page);
                    erase_page(sib_idx);
                    erase_internal(path, path_pos - 1, self_pos + 1);
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
            page.next = 0;
            page.data[0] = kv_pair;
            write_page(metadata.root, page);
            return;
        }
        
        PathEntry path[64];
        int path_size;
        int idx = get_leaf_idx(kv_pair, path, path_size);

        LeafPage leaf_page = read_page<LeafPage>(idx);
        // Case 1: no overflow
        if (leaf_page.size < L) {
            insert_val(leaf_page.data, leaf_page.size, kv_pair);
            write_page(idx, leaf_page);
            return;
        }

        // Case 2: overflow, split
        KeyValuePair temp_data[L + 1];
        copy_range(temp_data, 0, leaf_page.data, 0, L);

        int len = L;
        insert_val(temp_data, len, kv_pair);

        LeafPage new_page;
        int new_idx = new_free_page();
        new_page.next = leaf_page.next;
        leaf_page.next = new_idx;
        new_page.size = (L + 1) / 2;
        leaf_page.size = (L + 1) - new_page.size;
        copy_range(leaf_page.data, 0, temp_data, 0, leaf_page.size);
        copy_range(new_page.data, 0, temp_data, leaf_page.size,
                   new_page.size);
        write_page(idx, leaf_page);
        write_page(new_idx, new_page);
        insert_internal(path, path_size, new_page.data[0], idx, new_idx);
    }
    // erase (key, value) in the tree
    // if (key, value) does not exist before, do nothing
    void erase(const Key &key, const T &value) {
        if (!metadata.root) {
            return;
        }

        auto kv_pair = KeyValuePair{key, value};
        PathEntry path[64];
        int path_size;
        int idx = get_leaf_idx(kv_pair, path, path_size);
        LeafPage leaf_page = read_page<LeafPage>(idx);

        // erase the corresponding element and update separators
        if (leaf_page.data[0] == kv_pair) {
            erase_val(leaf_page.data, leaf_page.size, kv_pair);
            if (leaf_page.size) {
                update_first_key(path, path_size, leaf_page.data[0]);
            }
        } else if (!erase_val(leaf_page.data, leaf_page.size, kv_pair)) {
            return;
        }

        if (!path_size) {  // Case 1: root
            if (leaf_page.size) {
                write_page(idx, leaf_page);
            } else {
                metadata.root = 0;
                erase_page(idx);
            }
        } else if (leaf_page.size >= (L + 1) / 2) {  // Case 2: no underflow
            write_page(idx, leaf_page);
        } else {  // Case 3: underflow
            InternalPage parent_page =
                read_page<InternalPage>(path[path_size - 1].idx);
            int self_pos = path[path_size - 1].ch_pos;

            if (self_pos > 0) {  // left sibling
                int sib_idx = parent_page.children[self_pos - 1];
                LeafPage sib_page = read_page<LeafPage>(sib_idx);
                // Case 3.1 (L): left sibling no underflow
                if (sib_page.size > (L + 1) / 2) {
                    move_right(leaf_page.data, 0, leaf_page.size);
                    leaf_page.data[0] = sib_page.data[--sib_page.size];
                    ++leaf_page.size;
                    update_first_key(path, path_size, leaf_page.data[0]);
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
                    erase_internal(path, path_size - 1, self_pos);
                }
            } else {
                int sib_idx = parent_page.children[self_pos + 1];
                LeafPage sib_page = read_page<LeafPage>(sib_idx);
                // Case 3.1 (R): right sibling no underflow
                if (sib_page.size > (L + 1) / 2) {
                    leaf_page.data[leaf_page.size++] = sib_page.data[0];
                    move_left(sib_page.data, 0, sib_page.size);
                    --sib_page.size;
                    parent_page.data[self_pos] = sib_page.data[0];
                    write_page(idx, leaf_page);
                    write_page(sib_idx, sib_page);
                    write_page(path[path_size - 1].idx, parent_page);
                }
                // Case 3.2 (R): right sibling underflow, merge
                else {
                    for (int i = 0; i < sib_page.size; i++) {
                        leaf_page.data[leaf_page.size++] = sib_page.data[i];
                    }
                    leaf_page.next = sib_page.next;
                    write_page(idx, leaf_page);
                    erase_page(sib_idx);
                    erase_internal(path, path_size - 1, self_pos + 1);
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
        while (true) {
            int is_leaf = is_leaf_page(idx);
            if (is_leaf) {
                break;
            }
            InternalPage page = read_internal_after_type(is_leaf);
            int left = 0, right = page.size;
            while (left < right) {
                int mid = (left + right) / 2;
                if (page.data[mid].first < key) {
                    left = mid + 1;
                } else {
                    right = mid;
                }
            }
            int child = left;
            idx = page.children[child];
        }

        while (idx) {
            LeafPage page = read_page<LeafPage>(idx);
            for (int i = 0; i < page.size; i++) {
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

#endif
