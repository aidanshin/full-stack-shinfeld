#ifndef THREADSAFEQUEUE_HPP
#define THREADSAFEQUEUE_HPP

#include <mutex>
#include <queue>
#include <condition_variable>
#include <optional>

template<typename T>
class ThreadSafeQueue {
    private:
        mutable std::mutex mtx;
        std::condition_variable cv;
        std::queue<T> q;        
        bool closed = false;

    public:

        ThreadSafeQueue() = default;
        
        template <typename U>
        void push(U&& value) {
            {
                std::lock_guard<std::mutex> lock(mtx);
                if (closed) throw std::runtime_error("Queue is closed");
                q.push(std::forward<U>(value));
            }
            cv.notify_one();
        }

        std::optional<T> pop() {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, [this]() {return closed || !q.empty(); });

            if (q.empty()) return std::nullopt;
            T item = std::move(q.front());
            q.pop();
            return item;
        }

        bool tryPop(T& item) {
            std::lock_guard<std::mutex> lock(mtx);
            if (q.empty()) return false;
            item = std::move(q.front());
            q.pop();
            return true;
        }

        void close() {
            {
                std::lock_guard<std::mutex> lock(mtx);
                closed = true;
            }
            cv.notify_all();
        }

        bool isClosed() const {
            std::lock_guard<std::mutex> lock(mtx);
            return closed;
        }

        bool empty() const {
            std::lock_guard<std::mutex> lock(mtx);
            return q.empty();
        }

        size_t size() const {
            std::lock_guard<std::mutex> lock(mtx);
            return q.size();
        }

        

};

#endif 
