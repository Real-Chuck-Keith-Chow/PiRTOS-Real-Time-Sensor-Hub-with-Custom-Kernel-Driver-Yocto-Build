#pragma once
#include <cstddef>
#include <mutex>
#include <optional>
#include <vector>

template <typename T>
class RingBuffer {
public:
  explicit RingBuffer(size_t cap) : buf_(cap) {}

  bool push(const T& v) {
    std::lock_guard<std::mutex> lk(m_);
    if (count_ == buf_.size()) return false;
    buf_[(head_++) % buf_.size()] = v;
    ++count_;
    return true;
  }
  std::optional<T> pop() {
    std::lock_guard<std::mutex> lk(m_);
    if (count_ == 0) return std::nullopt;
    T v = *buf_[(tail_++) % buf_.size()];
    --count_;
    return v;
  }
  size_t size() const { return count_; }
  size_t capacity() const { return buf_.size(); }

private:
  mutable std::mutex m_;
  std::vector<std::optional<T>> buf_;
  size_t head_ = 0, tail_ = 0, count_ = 0;
};
