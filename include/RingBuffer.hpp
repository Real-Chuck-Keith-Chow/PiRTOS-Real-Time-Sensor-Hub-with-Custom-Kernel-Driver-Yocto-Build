#pragma once
#include <vector>
#include <mutex>
#include <condition_variable>
#include <optional>
#include <chrono>

// Thread-safe fixed-size circular buffer.
// Overwrite policy is optional: if overwrite=true, newest item replaces oldest when full.
// Otherwise push() blocks until space is available (or timed variants return false).
template <typename T>
class RingBuffer {
public:
    explicit RingBuffer(size_t capacity, bool overwrite = false)
        : buf_(capacity), cap_(capacity), overwrite_(overwrite) {}

    // Non-copyable (to avoid accidental heavy copies); movable.
    RingBuffer(const RingBuffer&) = delete;
    RingBuffer& operator=(const RingBuffer&) = delete;
    RingBuffer(RingBuffer&&) = delete;
    RingBuffer& operator=(RingBuffer&&) = delete;

    size_t capacity() const { return cap_; }

    // Blocking push
    void push(const T& v) {
        std::unique_lock<std::mutex> lk(m_);
        space_cv_.wait(lk, [&]{ return overwrite_ || size_ < cap_; });
        do_push(v);
        lk.unlock();
        data_cv_.notify_one();
    }

    void push(T&& v) {
        std::unique_lock<std::mutex> lk(m_);
        space_cv_.wait(lk, [&]{ return overwrite_ || size_ < cap_; });
        do_push(std::move(v));
        lk.unlock();
        data_cv_.notify_one();
    }

    // Timed push: returns false on timeout if not overwriting and full.
    template <class Rep, class Period>
    bool push_for(const T& v, const std::chrono::duration<Rep,Period>& d) {
        std::unique_lock<std::mutex> lk(m_);
        if (!overwrite_ && !space_cv_.wait_for(lk, d, [&]{ return size_ < cap_; })) return false;
        do_push(v);
        lk.unlock();
        data_cv_.notify_one();
        return true;
    }

    // Blocking pop
    T pop() {
        std::unique_lock<std::mutex> lk(m_);
        data_cv_.wait(lk, [&]{ return size_ > 0; });
        T out = std::move(buf_[head_]);
        head_ = (head_ + 1) % cap_;
        --size_;
        lk.unlock();
        space_cv_.notify_one();
        return out;
    }

    // Non-blocking pop
    std::optional<T> try_pop() {
        std::lock_guard<std::mutex> lk(m_);
        if (size_ == 0) return std::nullopt;
        T out = std::move(buf_[head_]);
        head_ = (head_ + 1) % cap_;
        --size_;
        space_cv_.notify_one();
        return out;
    }

    // Timed pop
    template <class Rep, class Period>
    std::optional<T> pop_for(const std::chrono::duration<Rep,Period>& d) {
        std::unique_lock<std::mutex> lk(m_);
        if (!data_cv_.wait_for(lk, d, [&]{ return size_ > 0; })) return std::nullopt;
        T out = std::move(buf_[head_]);
        head_ = (head_ + 1) % cap_;
        --size_;
        lk.unlock();
        space_cv_.notify_one();
        return out;
    }

    size_t size() const {
        std::lock_guard<std::mutex> lk(m_);
        return size_;
    }

    bool empty() const { return size() == 0; }
    bool full()  const { return size() == cap_; }

private:
    template <typename U>
    void do_push(U&& v) {
        if (size_ == cap_) {
            // overwrite oldest
            buf_[tail_] = std::forward<U>(v);
            tail_ = (tail_ + 1) % cap_;
            head_ = tail_; // head follows tail when full and overwriting
        } else {
            buf_[tail_] = std::forward<U>(v);
            tail_ = (tail_ + 1) % cap_;
            ++size_;
        }
    }

    mutable std::mutex m_;
    std::condition_variable data_cv_, space_cv_;
    std::vector<T> buf_;
    size_t cap_;
    size_t head_ = 0;
    size_t tail_ = 0;
    size_t size_ = 0;
    bool overwrite_;
};
