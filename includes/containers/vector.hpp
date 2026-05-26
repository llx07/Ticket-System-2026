#ifndef SJTU_VECTOR_HPP
#define SJTU_VECTOR_HPP

#include "exceptions.hpp"

namespace sjtu {

/**
 * a data_ container like std::vector
 * store data_ in a successive memory and support random access.
 */
template <typename T>
class vector {
   private:
    T* data_;
    size_t capacity_;
    size_t size_;

    void reallocate() {
        if (capacity_ == 0) {
            capacity_ = 2;
        } else {
            capacity_ = capacity_ + (capacity_ >> 1);
        }
        T* new_data = static_cast<T*>(operator new(capacity_ * sizeof(T)));
        for (size_t i = 0; i < size_; i++) {
            new (&new_data[i]) T(static_cast<T&&>(data_[i]));
        }
        for (size_t i = 0; i < size_; i++) {
            data_[i].~T();
        }
        operator delete(data_);
        data_ = new_data;
    }

   public:
    /**
     * a type for actions of the elements of a vector, and you should write
     *   a class named const_iterator with same interfaces.
     */
    /**
     * you can see RandomAccessIterator at CppReference for help.
     */
    class const_iterator;
    class iterator {
        // The following code is written for the C++ type_traits library.
        // Type traits is a C++ feature for describing certain properties of a
        // type. For instance, for an iterator, iterator::value_type is the type
        // that the iterator points to. STL algorithms and containers may use
        // these type_traits (e.g. the following typedef) to work properly. In
        // particular, without the following code,
        // @code{std::sort(iter, iter1);} would not compile.
        // See these websites for more information:
        // https://en.cppreference.com/w/cpp/header/type_traits
        // About value_type:
        // https://blog.csdn.net/u014299153/article/details/72419713 About
        // iterator_category: https://en.cppreference.com/w/cpp/iterator
       public:
        using difference_type = std::ptrdiff_t;
        using value_type = T;
        using pointer = T*;
        using reference = T&;
        using iterator_category = std::output_iterator_tag;

       private:
        const vector* vector_belong_;
        pointer ptr_;

       public:
        iterator(const vector* vector_belong, pointer ptr)
            : vector_belong_(vector_belong), ptr_(ptr) {}
        /**
         * return a new iterator which pointer n-next elements
         * as well as operator-
         */
        iterator operator+(const int& n) const {
            iterator it = *this;
            it += n;
            return it;
        }
        iterator operator-(const int& n) const {
            iterator it = *this;
            it -= n;
            return it;
        }
        // return the distance between two iterators,
        // if these two iterators point to different vectors, throw
        // invaild_iterator.
        int operator-(const iterator& rhs) const {
            if (vector_belong_ != rhs.vector_belong_) {
                throw invalid_iterator{};
            }
            return static_cast<int>(ptr_ - rhs.ptr_);
        }
        iterator& operator+=(const int& n) {
            ptr_ += n;
            return *this;
        }
        iterator& operator-=(const int& n) {
            ptr_ -= n;
            return *this;
        }
        iterator operator++(int) {
            iterator old = *this;
            operator++();
            return old;
        }
        iterator& operator++() {
            ++ptr_;
            return *this;
        }
        iterator operator--(int) {
            iterator old = *this;
            operator--();
            return old;
        }
        iterator& operator--() {
            --ptr_;
            return *this;
        }
        T& operator*() const { return *ptr_; }
        /**
         * a operator to check whether two iterators are same (pointing to the
         * same memory address).
         */
        bool operator==(const iterator& rhs) const { return ptr_ == rhs.ptr_; }
        bool operator==(const const_iterator& rhs) const {
            return ptr_ == rhs.ptr_;
        }
        /**
         * some other operator for iterator.
         */
        bool operator!=(const iterator& rhs) const { return ptr_ != rhs.ptr_; }
        bool operator!=(const const_iterator& rhs) const {
            return ptr_ != rhs.ptr_;
        }

        auto operator<=>(const iterator& rhs) const {
            return ptr_ <=> rhs.ptr_;
        }
        auto operator<=>(const const_iterator& rhs) const {
            return ptr_ <=> rhs.ptr_;
        }
    };
    /**
     * has same function as iterator, just for a const object.
     */
    class const_iterator {
       public:
        using difference_type = std::ptrdiff_t;
        using value_type = const T;
        using pointer = const T*;
        using reference = const T&;
        using iterator_category = std::output_iterator_tag;

       private:
        const vector* vector_belong_;
        pointer ptr_;

       public:
        const_iterator(const vector* vector_belong, pointer ptr)
            : vector_belong_(vector_belong), ptr_(ptr) {}
        /**
         * return a new iterator which pointer n-next elements
         * as well as operator-
         */
        const_iterator operator+(const int& n) const {
            const_iterator it = *this;
            it += n;
            return it;
        }
        const_iterator operator-(const int& n) const {
            const_iterator it = *this;
            it -= n;
            return it;
        }
        // return the distance between two iterators,
        // if these two iterators point to different vectors, throw
        // invaild_iterator.
        int operator-(const const_iterator& rhs) const {
            if (vector_belong_ != rhs.vector_belong_) {
                throw invalid_iterator{};
            }
            return ptr_ - rhs.ptr_;
        }
        const_iterator& operator+=(const int& n) {
            ptr_ += n;
            return *this;
        }
        const_iterator& operator-=(const int& n) {
            ptr_ -= n;
            return *this;
        }
        const_iterator operator++(int) {
            iterator old = *this;
            operator++();
            return old;
        }
        const_iterator& operator++() {
            ++ptr_;
            return *this;
        }
        const_iterator operator--(int) {
            const_iterator old = *this;
            operator--();
            return old;
        }
        const_iterator& operator--() {
            --ptr_;
            return *this;
        }
        const T& operator*() const { return *ptr_; }
        /**
         * a operator to check whether two iterators are same (pointing to the
         * same memory address).
         */
        bool operator==(const iterator& rhs) const { return ptr_ == rhs.ptr_; }
        bool operator==(const const_iterator& rhs) const {
            return ptr_ == rhs.ptr_;
        }
        /**
         * some other operator for iterator.
         */
        bool operator!=(const iterator& rhs) const { return ptr_ != rhs.ptr_; }
        bool operator!=(const const_iterator& rhs) const {
            return ptr_ != rhs.ptr_;
        }

        auto operator<=>(const iterator& rhs) const {
            return ptr_ <=> rhs.ptr_;
        }
        auto operator<=>(const const_iterator& rhs) const {
            return ptr_ <=> rhs.ptr_;
        }
    };
    /**
     * Constructs
     * At least two: default constructor, copy constructor
     */
    vector() : data_(nullptr), capacity_(0), size_(0) {}
    vector(const vector& other) {
        if (other.size_ == 0) {
            capacity_ = size_ = 0;
            data_ = nullptr;
            return;
        }
        capacity_ = other.capacity_;
        size_ = other.size_;

        data_ = static_cast<T*>(operator new(capacity_ * sizeof(T)));
        for (size_t i = 0; i < other.size(); i++) {
            new (&data_[i]) T(other.data_[i]);
        }
    }

    /**
     * Destructor
     */
    ~vector() {
        for (size_t i = 0; i < size_; i++) {
            data_[i].~T();
        }
        operator delete(data_);
    }
    /**
     * Assignment operator
     */
    vector& operator=(const vector& other) {
        if (this == &other) {
            return *this;
        }
        for (int i = 0; i < size_; i++) {
            data_[i].~T();
        }
        operator delete(data_);
        if (other.size_ == 0) {
            capacity_ = size_ = 0;
            data_ = nullptr;
            return *this;
        }
        capacity_ = other.capacity_;
        size_ = other.size_;

        data_ = static_cast<T*>(operator new(capacity_ * sizeof(T)));
        for (int i = 0; i < other.size(); i++) {
            new (&data_[i]) T(other.data_[i]);
        }
        return *this;
    }

    friend bool operator==(const vector& lhs, const vector& rhs) {
        if (lhs.size_ != rhs.size_) {
            return false;
        }
        for (size_t i = 0; i < lhs.size_; ++i) {
            if (!(lhs.data_[i] == rhs.data_[i])) {
                return false;
            }
        }
        return true;
    }

    friend bool operator!=(const vector& lhs, const vector& rhs) {
        return !(lhs == rhs);
    }

    /**
     * assigns specified element with bounds checking
     * throw index_out_of_bound if pos is not in [0, size)
     */
    T& at(const size_t& pos) {
        if (pos >= size_) {
            throw index_out_of_bound{};
        }
        return data_[pos];
    }
    const T& at(const size_t& pos) const {
        if (pos >= size_) {
            throw index_out_of_bound{};
        }
        return data_[pos];
    }
    /**
     * assigns specified element with bounds checking
     * throw index_out_of_bound if pos is not in [0, size)
     * !!! Pay attentions
     *   In STL this operator does not check the boundary but I want you to do.
     */
    T& operator[](const size_t& pos) {
        if (pos >= size_) {
            throw index_out_of_bound{};
        }
        return data_[pos];
    }
    const T& operator[](const size_t& pos) const {
        if (pos >= size_) {
            throw index_out_of_bound{};
        }
        return data_[pos];
    }
    /**
     * access the first element.
     * throw container_is_empty if size == 0
     */
    const T& front() const {
        if (size_ == 0) {
            throw container_is_empty{};
        }
        return data_[0];
    }
    /**
     * access the last element.
     * throw container_is_empty if size == 0
     */
    const T& back() const {
        if (size_ == 0) {
            throw container_is_empty{};
        }
        return data_[size_ - 1];
    }
    /**
     * returns an iterator to the beginning.
     */
    iterator begin() { return iterator{this, data_}; }
    const_iterator begin() const { return const_iterator{this, data_}; }

    const_iterator cbegin() const { return const_iterator{this, data_}; }
    /**
     * returns an iterator to the end.
     */
    iterator end() { return iterator{this, data_ + size_}; }
    const_iterator end() const { return const_iterator{this, data_ + size_}; }
    const_iterator cend() const { return const_iterator{this, data_ + size_}; }
    /**
     * checks whether the container is empty
     */
    bool empty() const { return size_ == 0; }
    /**
     * returns the number of elements
     */
    size_t size() const { return size_; }
    /**
     * clears the contents
     */
    void clear() {
        for (int i = 0; i < size_; i++) {
            data_[i].~T();
        }
        size_ = 0;
    }
    /**
     * inserts value before pos
     * returns an iterator pointing to the inserted value.
     */
    iterator insert(iterator pos, const T& value) {
        int ind = pos - begin();
        return insert(ind, value);
    }
    /**
     * inserts value at index ind.
     * after inserting, this->at(ind) == value
     * returns an iterator pointing to the inserted value.
     * throw index_out_of_bound if ind > size (in this situation ind can be size
     * because after inserting the size will increase 1.)
     */
    iterator insert(const size_t& ind, const T& value) {
        if (ind > size_) {
            throw index_out_of_bound{};
        }
        if (size_ == capacity_) {
            reallocate();
        }

        if (ind == size_) {
            new (&data_[size_]) T(value);
        } else {
            new (&data_[size_]) T(value);
            for (int i = size_ - 1;;
                 i--) {  // here size_ > 0 because ind != size_.
                data_[i + 1] = static_cast<T&&>(data_[i]);
                if (i == ind) {
                    break;
                }
            }
        }

        data_[ind] = value;
        size_++;
        return begin() + ind;
    }
    /**
     * removes the element at pos.
     * return an iterator pointing to the following element.
     * If the iterator pos refers the last element, the end() iterator is
     * returned.
     */
    iterator erase(iterator pos) {
        int ind = pos - begin();
        return erase(ind);
    }
    /**
     * removes the element with index ind.
     * return an iterator pointing to the following element.
     * throw index_out_of_bound if ind >= size
     */
    iterator erase(const size_t& ind) {
        if (ind >= size_) {
            throw index_out_of_bound{};
        }
        for (int i = ind + 1; i < size_; i++) {
            data_[i - 1] = static_cast<T&&>(data_[i]);
        }
        size_--;
        data_[size_].~T();
        return begin() + ind;
    }
    /**
     * adds an element to the end.
     */
    void push_back(const T& value) {
        if (size_ == capacity_) {
            reallocate();
        }

        new (&data_[size_]) T(value);
        ++size_;
    }
    /**
     * remove the last element from the end.
     * throw container_is_empty if size() == 0
     */
    void pop_back() {
        if (size_ == 0) {
            throw container_is_empty{};
        }
        erase(--end());
    }
};

}  // namespace sjtu

#endif
