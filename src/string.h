#ifndef STRING_H
#define STRING_H

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
using namespace v1;
} // namespace String

#endif
