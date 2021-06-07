#ifndef STRING_H
#define STRING_H

#include <atomic>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>

namespace String {
namespace v1 {

class String {
public:
  String(const char *c_str = "") {
    shared_string_ = std::make_shared<SharedString>(c_str);
  }

  String(String const &rhs) {
    if (rhs.shared_string_->sharedable_) {
      shared_string_ = rhs.shared_string_;
    } else {
      shared_string_ =
          std::make_shared<SharedString>(rhs.shared_string_->c_str_);
    }
  }

  String &operator=(String const &rhs) {
    if (rhs.shared_string_->sharedable_) {
      shared_string_ = rhs.shared_string_;
    } else {
      shared_string_ =
          std::make_shared<SharedString>(rhs.shared_string_->c_str_);
    }
    return *this;
  }

  String(String &&rhs) {
    shared_string_ = move(rhs.shared_string_);
    rhs.shared_string_ = std::make_shared<SharedString>("");
  }

  String &operator=(String &&rhs) {
    shared_string_ = move(rhs.shared_string_);
    rhs.shared_string_ = std::make_shared<SharedString>("");
    return *this;
  }

  const char *c_str() const {
    assert(shared_string_->c_str_ != nullptr);
    return shared_string_->c_str_;
  }

  char *c_str() {
    assert(shared_string_->c_str_ != nullptr);
    shared_string_->sharedable_ = false;
    shared_string_ = std::make_shared<SharedString>(shared_string_->c_str_);
    return shared_string_->c_str_;
  }

  char &operator[](size_t i) { return c_str()[i]; }

  const char &operator[](size_t i) const { return c_str()[i]; }

  size_t size() const { return shared_string_->size; }

  bool empty() const { return size() == 0; }

private:
  struct SharedString {
    SharedString(const char *c_str) {
      size_t len = strlen(c_str);
      size = len;
      c_str_ = new char[len + 1];
      strcpy(c_str_, c_str);
      c_str_[len] = '\0';
      sharedable_ = true;
    }

    ~SharedString() {
      assert(c_str_ != nullptr);
      free(c_str_);
    }
    char *c_str_{nullptr};
    size_t size;
    bool sharedable_{true};
  };
  std::shared_ptr<SharedString> shared_string_;
};

std::ostream &operator<<(std::ostream &os, String const &str) {
  os << str.c_str();
  return os;
}

bool operator==(String const &lhs, String const &rhs) {
  return strcmp(lhs.c_str(), rhs.c_str()) == 0;
}

bool operator!=(String const &lhs, String const &rhs) {
  return strcmp(lhs.c_str(), rhs.c_str()) != 0;
}

bool share_c_str(String const &lhs, String const &rhs) {
  return lhs.c_str() == rhs.c_str();
}

} // namespace v1

namespace v2 {

template <typename T> struct SharedPtr {
  explicit SharedPtr(T *data = nullptr) : state_{new SharedState(data)} {}
  SharedPtr(SharedPtr<T> const &rhs) {
    state_ = rhs.state_;
    if (state_ != nullptr) {
      state_->addRef();
    }
  }
  SharedPtr &operator=(SharedPtr<T> const &rhs) {
    if (state_ != nullptr) {
      state_->removeRef();
    }
    state_ = rhs.state_;
    if (state_ != nullptr) {
      state_->addRef();
    }
    return *this;
  }
  SharedPtr(SharedPtr<T> &&rhs) {
    state_ = rhs.state_;
    rhs.state_ = nullptr;
  }
  SharedPtr &operator=(SharedPtr<T> &&rhs) {
    if (state_ != nullptr) {
      state_->removeRef();
    }
    state_ = rhs.state_;
    rhs.state_ = nullptr;
    return *this;
  }
  ~SharedPtr() {
    if (state_ != nullptr) {
      state_->removeRef();
    }
  }

  T *operator->() const {
    if (state_ == nullptr)
      return nullptr;
    return state_->data_;
  }

private:
  struct SharedState {
    SharedState(T *data) : data_{data}, ref_cnt_{1} {}
    void removeRef() {
      ref_cnt_--;
      if (ref_cnt_.load() == 0) {
        delete this;
      }
    }
    void addRef() { ref_cnt_++; }
    T *data_;
    std::atomic<int> ref_cnt_;
  };
  SharedState *state_{nullptr};
};

struct String {
public:
  String(const char *c_str = "") {
    shared_string_ = SharedPtr<SharedString>(new SharedString(c_str));
  }

  String(String const &rhs) {
    if (rhs.shared_string_->sharedable_) {
      shared_string_ = rhs.shared_string_;
    } else {
      shared_string_ =
          SharedPtr<SharedString>(new SharedString(rhs.shared_string_->c_str_));
    }
  }

  String &operator=(String const &rhs) {
    if (rhs.shared_string_->sharedable_) {
      shared_string_ = rhs.shared_string_;
    } else {
      shared_string_ =
          SharedPtr<SharedString>(new SharedString(rhs.shared_string_->c_str_));
    }
    return *this;
  }

  String(String &&rhs) {
    shared_string_ = std::move(rhs.shared_string_);
    rhs.shared_string_ = SharedPtr<SharedString>(new SharedString(""));
  }

  String &operator=(String &&rhs) {
    shared_string_ = std::move(rhs.shared_string_);
    rhs.shared_string_ = SharedPtr<SharedString>(new SharedString(""));
    return *this;
  }

  const char *c_str() const {
    assert(shared_string_->c_str_ != nullptr);
    return shared_string_->c_str_;
  }

  char *c_str() {
    assert(shared_string_->c_str_ != nullptr);
    shared_string_->sharedable_ = false;
    shared_string_ =
        SharedPtr<SharedString>(new SharedString(shared_string_->c_str_));
    return shared_string_->c_str_;
  }

  char &operator[](size_t i) { return c_str()[i]; }

  const char &operator[](size_t i) const { return c_str()[i]; }

  size_t size() const { return shared_string_->size; }

  bool empty() const { return size() == 0; }

private:
  struct SharedString {
    SharedString(const char *c_str) {
      size_t len = strlen(c_str);
      size = len;
      c_str_ = new char[len + 1];
      strcpy(c_str_, c_str);
      c_str_[len] = '\0';
      sharedable_ = true;
    }

    ~SharedString() {
      assert(c_str_ != nullptr);
      free(c_str_);
    }
    char *c_str_{nullptr};
    size_t size;
    bool sharedable_{true};
  };
  SharedPtr<SharedString> shared_string_;
};

std::ostream &operator<<(std::ostream &os, String const &str) {
  os << str.c_str();
  return os;
}

bool operator==(String const &lhs, String const &rhs) {
  return strcmp(lhs.c_str(), rhs.c_str()) == 0;
}

bool operator!=(String const &lhs, String const &rhs) {
  return strcmp(lhs.c_str(), rhs.c_str()) != 0;
}

bool share_c_str(String const &lhs, String const &rhs) {
  return lhs.c_str() == rhs.c_str();
}

} // namespace v2

using namespace v2;
} // namespace String

#endif
