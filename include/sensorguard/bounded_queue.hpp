#pragma once
// Blocking bounded queue used between the producer (sampling) and consumer (analysis) threads.
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <mutex>
#include <utility>

namespace sg {

template <class T>
class BoundedQueue {
public:
    explicit BoundedQueue(std::size_t capacity) : cap_(capacity) {}

    // Blocks while full. Returns false if the queue was closed.
    bool push(T v) {
        std::unique_lock<std::mutex> l(m_);
        not_full_.wait(l, [&] { return closed_ || q_.size() < cap_; });
        if (closed_) return false;
        q_.push_back(std::move(v));
        not_empty_.notify_one();
        return true;
    }

    // Blocks while empty. Returns false once closed AND drained.
    bool pop(T& out) {
        std::unique_lock<std::mutex> l(m_);
        not_empty_.wait(l, [&] { return closed_ || !q_.empty(); });
        if (q_.empty()) return false;
        out = std::move(q_.front());
        q_.pop_front();
        not_full_.notify_one();
        return true;
    }

    void close() {
        { std::lock_guard<std::mutex> l(m_); closed_ = true; }
        not_empty_.notify_all();
        not_full_.notify_all();
    }

    std::size_t size() const { std::lock_guard<std::mutex> l(m_); return q_.size(); }

private:
    mutable std::mutex m_;
    std::condition_variable not_empty_, not_full_;
    std::deque<T> q_;
    std::size_t cap_;
    bool closed_ = false;
};

}  // namespace sg
