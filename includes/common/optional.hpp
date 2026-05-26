#ifndef SJTU_OPTIONAL_HPP
#define SJTU_OPTIONAL_HPP

template <class T>
class Optional {
   private:
    bool has_value_;
    T value_;

   public:
    Optional() : has_value_(false), value_() {}

    Optional(const T& value) : has_value_(true), value_(value) {}

    Optional& operator=(const T& value) {
        has_value_ = true;
        value_ = value;
        return *this;
    }

    void reset() {
        has_value_ = false;
        value_ = T();
    }

    explicit operator bool() const { return has_value_; }
    bool has_value() const { return has_value_; }
    T& value() { return value_; }
    const T& value() const { return value_; }
    T value_or(const T& default_value) const {
        return has_value_ ? value_ : default_value;
    }
    T& operator*() { return value_; }
    const T& operator*() const { return value_; }
};

#endif  // SJTU_OPTIONAL_HPP
