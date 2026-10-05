#include <thread>

#include "sensorguard/bounded_queue.hpp"
#include "test_framework.hpp"

using namespace sg;

TEST(queue_fifo_order) {
    BoundedQueue<int> q(8);
    for (int i = 0; i < 5; ++i) q.push(i);
    int v = -1;
    for (int i = 0; i < 5; ++i) { CHECK(q.pop(v)); CHECK(v == i); }
}

TEST(queue_close_unblocks_consumer_after_drain) {
    BoundedQueue<int> q(4);
    q.push(1);
    q.close();
    int v = 0;
    CHECK(q.pop(v));          // still drains
    CHECK(v == 1);
    CHECK(!q.pop(v));         // then reports closed
    CHECK(!q.push(2));        // push after close fails
}

TEST(queue_producer_consumer_threads_no_loss) {
    BoundedQueue<int> q(4);   // small capacity forces blocking
    const int N = 10000;
    long long sum = 0;
    std::thread cons([&] { int v; while (q.pop(v)) sum += v; });
    std::thread prod([&] { for (int i = 1; i <= N; ++i) q.push(i); q.close(); });
    prod.join();
    cons.join();
    CHECK(sum == static_cast<long long>(N) * (N + 1) / 2);
}

TEST(queue_blocked_producer_released_by_close) {
    BoundedQueue<int> q(1);
    q.push(1);
    bool result = true;
    std::thread prod([&] { result = q.push(2); });   // blocks: queue is full
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    q.close();
    prod.join();
    CHECK(!result);
}
