
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
        ++len;
        for (int i = 0; i < len; i++) {
            if (i == len - 1) {
                arr[i] = x;
                return i;
            }
            if (x < arr[i]) {
                for (int j = len - 2; j >= i; j--) {
                    arr[j + 1] = arr[j];
                }
                arr[i] = x;
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
    int get_addr(int idx) { return (idx - 1) * PAGE_SIZE + sizeof(MetaData); }

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
    U read_data_page(int idx) {
        U page;
        file.seekg(get_addr(idx));
        file.read(reinterpret_cast<char *>(&page), sizeof(U));
        return page;
    }
    template <class U>
    void write_data_page(int idx, const U &page) {
        file.seekp(get_addr(idx));
        file.write(reinterpret_cast<const char *>(&page), sizeof(U));
    }

    void set_parent(int idx, int p) {
        if (is_leaf_page(idx)) {
            LeafPage page = read_data_page<LeafPage>(idx);
            page.parent = p;
            write_data_page(idx, page);
        } else {
            InternalPage page = read_data_page<InternalPage>(idx);
            page.parent = p;
            write_data_page(idx, page);
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
            write_data_page(metadata.root, root_page);
            return;
        }

        InternalPage page = read_data_page<InternalPage>(idx);
        if (page.size < M) {
            int i = insert_val(page.data, page.size, kv_pair);
            for (int j = page.size - 1; j > i; j--) {
                page.children[j + 1] = page.children[j];
            }
            page.children[i] = left_idx;
            page.children[i + 1] = right_idx;
            write_data_page(idx, page);
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

            write_data_page(idx, page);
            write_data_page(new_idx, new_page);
            insert_internal(page.parent, temp_data[page.size], idx, new_idx);
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
            write_data_page(metadata.root, page);
            return;
        }
        int idx = metadata.root;
        while (!is_leaf_page(idx)) {
            InternalPage cur_page = read_data_page<InternalPage>(idx);
            for (int i = 0; i < cur_page.size; i++) {
                if (kv_pair < cur_page.data[i]) {
                    idx = cur_page.children[i];
                    break;
                }

                if (i == cur_page.size - 1) {
                    idx = cur_page.children[i + 1];
                    break;
                }
            }
        }

        LeafPage leaf_page = read_data_page<LeafPage>(idx);
        if (leaf_page.size < L) {
            insert_val(leaf_page.data, leaf_page.size, kv_pair);
            write_data_page(idx, leaf_page);
            return;
        }

        LeafPage new_page;
        int new_idx = new_free_page();
        new_page.parent = leaf_page.parent;
        new_page.next = leaf_page.next;
        leaf_page.next = new_idx;
        new_page.size = (L + 1) / 2;
        bool used_kv_pair = false;
        for (int left = new_page.size, i = leaf_page.size - 1;
             i >= 0 && left;) {
            if (!used_kv_pair && leaf_page.data[i] < kv_pair) {
                new_page.data[--left] = kv_pair;
                used_kv_pair = true;
            } else {
                new_page.data[--left] = leaf_page.data[i--];
            }
        }
        leaf_page.size = L + 1 - new_page.size;
        if (!used_kv_pair) {
            --leaf_page.size;
            insert_val(leaf_page.data, leaf_page.size, kv_pair);
        }

        write_data_page(idx, leaf_page);
        write_data_page(new_idx, new_page);
        insert_internal(leaf_page.parent, new_page.data[0], idx, new_idx);
    }
    // erase (key, value) in the tree
    // if (key, value) does not exist before, do nothing
    void erase(const Key &key, const T &value);

    // find all entries with the given key, sorted by values
    sjtu::vector<T> find_all(const Key &key) {
        sjtu::vector<T> result;
        if (!metadata.root) {
            return result;
        }

        int idx = metadata.root;
        while (!is_leaf_page(idx)) {
            InternalPage page = read_data_page<InternalPage>(idx);
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
            LeafPage page = read_data_page<LeafPage>(idx);
            for (int i = 0; i < page.size; ++i) {
                if (page.data[i].first < key) continue;
                if (key < page.data[i].first) return result;
                result.push_back(page.data[i].second);
            }
            idx = page.next;
        }
        return result;
    }
};
