#ifndef MESSAGEBUS_H
#define MESSAGEBUS_H
#pragma once
#include <queue>
#include <mutex>
#include <optional>

template<typename T>
class MessageQueue {
public:
    void push(const T& v) {
        std::lock_guard<std::mutex> lock(m);
        q.push(v);
    }

    std::optional<T> pop() {
        std::lock_guard<std::mutex> lock(m);
        if (q.empty()) return std::nullopt;
        T v = q.front();
        q.pop();
        return v;
    }

    bool empty() const {
        std::lock_guard<std::mutex> lock(m);
        return q.empty();
    }

private:
    mutable std::mutex m;
    std::queue<T> q;
};

#endif // MESSAGEBUS_H
