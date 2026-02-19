//Author::Aman Negi
#ifndef MESSAGEBUS_H
#define MESSAGEBUS_H
#pragma once
#include <queue>
#include <mutex>
#include <optional>
// MessageQueue<T> is a minimal thread-safe FIFO queue designed for
// passing messages safely between multiple threads (e.g. network → main thread).
//
// std::queue is NOT thread-safe by itself. If one thread pushes while another
// pops, the program will exhibit undefined behavior. This class wraps a
// std::queue with a mutex to guarantee safe, serialized access.
//
// This queue is non-blocking:
//  - push() always succeeds immediately
//  - pop() returns std::nullopt if no data is available
//
// This design is ideal for real-time systems (games, networking, rendering)
// where blocking the main thread is unacceptable and polling is preferred.
template<typename T>
class MessageQueue {
public:
    // Pushes a new message into the queue.
    // The mutex guarantees that only one thread can modify the queue at a time,
    // preventing data races and corruption.
    void push(const T& v) {
        std::lock_guard<std::mutex> lock(m);
        q.push(v);
    }
    // Attempts to pop a message from the queue.
    // If the queue is empty, std::nullopt is returned instead of blocking.
    // This allows the caller to safely poll the queue every frame or tick.
    std::optional<T> pop() {
        std::lock_guard<std::mutex> lock(m);
        if (q.empty()) return std::nullopt;
        T v = q.front();
        q.pop();
        return v;
    }
    // Returns true if the queue is empty.
    // Even this read operation must lock the mutex, because another thread
    // could be pushing or popping at the same time.
    bool empty() const {
        std::lock_guard<std::mutex> lock(m);
        return q.empty();
    }

private:
    // Mutex protecting access to the internal queue.
    // 'mutable' allows const functions (like empty()) to still lock it.
    mutable std::mutex m;
    // The underlying FIFO container holding messages.
    std::queue<T> q;
};

#endif // MESSAGEBUS_H
