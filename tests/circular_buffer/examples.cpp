// mcpplibs.mwrap.boost.circular_buffer — libs/circular_buffer/example/*.cpp as
// test cases. The bounded-buffer examples keep their boost::mutex /
// boost::condition / boost::bind producer-consumer, with smaller element
// counts and without boost::timer (they are timing comparisons upstream; the
// test keeps the FIFO behavior). boost::call_traits<T>::param_type is spelled
// `const T&` (the int / std::string case it resolves to).
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
import std;
import mcpplibs.mwrap.boost.circular_buffer;
import mcpplibs.mwrap.boost.thread;
import mcpplibs.mwrap.boost.bind;

namespace {
constexpr unsigned long queue_size = 100;
constexpr unsigned long total_elements = queue_size * 20;

template <class T>
class bounded_buffer {
public:
    using container_type = boost::circular_buffer<T>;
    using size_type = typename container_type::size_type;
    using value_type = typename container_type::value_type;
    explicit bounded_buffer(size_type capacity) : m_unread(0), m_container(capacity) {}
    bounded_buffer(const bounded_buffer&) = delete;
    bounded_buffer& operator=(const bounded_buffer&) = delete;
    void push_front(const value_type& item) {
        boost::mutex::scoped_lock lock(m_mutex);
        m_not_full.wait(lock, boost::bind(&bounded_buffer<value_type>::is_not_full, this));
        m_container.push_front(item);
        ++m_unread;
        lock.unlock();
        m_not_empty.notify_one();
    }
    void pop_back(value_type* pItem) {
        boost::mutex::scoped_lock lock(m_mutex);
        m_not_empty.wait(lock, boost::bind(&bounded_buffer<value_type>::is_not_empty, this));
        *pItem = m_container[--m_unread];
        lock.unlock();
        m_not_full.notify_one();
    }
private:
    bool is_not_empty() const { return m_unread > 0; }
    bool is_not_full() const { return m_unread < m_container.capacity(); }
    size_type m_unread;
    container_type m_container;
    boost::mutex m_mutex;
    boost::condition m_not_empty;
    boost::condition m_not_full;
};

template <class T>
class bounded_buffer_space_optimized {
public:
    using container_type = boost::circular_buffer_space_optimized<T>;
    using size_type = typename container_type::size_type;
    using value_type = typename container_type::value_type;
    explicit bounded_buffer_space_optimized(size_type capacity) : m_container(capacity) {}
    void push_front(const value_type& item) {
        boost::mutex::scoped_lock lock(m_mutex);
        m_not_full.wait(lock, boost::bind(&bounded_buffer_space_optimized<value_type>::is_not_full, this));
        m_container.push_front(item);
        lock.unlock();
        m_not_empty.notify_one();
    }
    void pop_back(value_type* pItem) {
        boost::mutex::scoped_lock lock(m_mutex);
        m_not_empty.wait(lock, boost::bind(&bounded_buffer_space_optimized<value_type>::is_not_empty, this));
        *pItem = m_container.back();
        m_container.pop_back();
        lock.unlock();
        m_not_full.notify_one();
    }
    std::size_t reserved() const { return m_container.capacity().capacity(); }
private:
    bool is_not_empty() const { return m_container.size() > 0; }
    bool is_not_full() const { return m_container.size() < m_container.capacity(); }
    container_type m_container;
    boost::mutex m_mutex;
    boost::condition m_not_empty;
    boost::condition m_not_full;
};

template <class T>
class bounded_buffer_deque_based {
public:
    using container_type = std::deque<T>;
    using size_type = typename container_type::size_type;
    using value_type = typename container_type::value_type;
    explicit bounded_buffer_deque_based(size_type capacity) : m_capacity(capacity) {}
    void push_front(const value_type& item) {
        boost::mutex::scoped_lock lock(m_mutex);
        m_not_full.wait(lock, boost::bind(&bounded_buffer_deque_based<value_type>::is_not_full, this));
        m_container.push_front(item);
        lock.unlock();
        m_not_empty.notify_one();
    }
    void pop_back(value_type* pItem) {
        boost::mutex::scoped_lock lock(m_mutex);
        m_not_empty.wait(lock, boost::bind(&bounded_buffer_deque_based<value_type>::is_not_empty, this));
        *pItem = m_container.back();
        m_container.pop_back();
        lock.unlock();
        m_not_full.notify_one();
    }
private:
    bool is_not_empty() const { return m_container.size() > 0; }
    bool is_not_full() const { return m_container.size() < m_capacity; }
    size_type m_capacity;
    container_type m_container;
    boost::mutex m_mutex;
    boost::condition m_not_empty;
    boost::condition m_not_full;
};

// Producer pushes 0..n-1, consumer checks they come out in the same order.
template <class Buffer>
std::vector<typename Buffer::value_type> fifo_test(Buffer* buffer,
                                                   std::function<typename Buffer::value_type(unsigned long)> make) {
    using value_type = typename Buffer::value_type;
    std::vector<value_type> received;
    received.reserve(total_elements);
    boost::thread consumer([&] {
        value_type item;
        for (unsigned long i = 0; i < total_elements; ++i) {
            buffer->pop_back(&item);
            received.push_back(item);
        }
    });
    boost::thread producer([&] {
        for (unsigned long i = 0; i < total_elements; ++i) buffer->push_front(make(i));
    });
    producer.join();
    consumer.join();
    return received;
}
}  // namespace

TEST_CASE("example: circular_buffer_example") {
    boost::circular_buffer<int> cb(3);
    cb.push_back(1);
    cb.push_back(2);
    cb.push_back(3);
    CHECK(cb[0] == 1);
    CHECK(cb[1] == 2);
    CHECK(cb[2] == 3);
    cb.push_back(4);  // overwrites 1
    cb.push_back(5);  // overwrites 2
    CHECK(cb[0] == 3);
    CHECK(cb[1] == 4);
    CHECK(cb[2] == 5);
    cb.pop_back();
    cb.pop_front();
    CHECK(cb[0] == 4);
}

TEST_CASE("example: circular_buffer_iter_example") {
    boost::circular_buffer<int> cb(3);
    cb.push_back(1);
    cb.push_back(2);
    cb.push_back(3);
    boost::circular_buffer<int>::iterator it = cb.begin();
    CHECK(*it == 1);
    cb.push_back(4);
    CHECK(*it == 4);  // the iterator still points to the initialized memory
}

TEST_CASE("example: circular_buffer_sum_example") {
    boost::circular_buffer<int> cb(3);
    CHECK(cb.capacity() == 3);
    CHECK(cb.size() == 0);
    CHECK(cb.empty());
    cb.push_back(1);
    cb.push_back(2);
    CHECK(cb[0] == 1);
    CHECK(cb[1] == 2);
    CHECK_FALSE(cb.full());
    CHECK(cb.size() == 2);
    cb.push_back(3);
    cb.push_back(4);
    CHECK(std::accumulate(cb.begin(), cb.end(), 0) == 9);
    CHECK(cb[0] == 2);
    CHECK(cb[1] == 3);
    CHECK(cb[2] == 4);
    CHECK(*cb.begin() == 2);
    CHECK(cb.front() == 2);
    CHECK(cb.back() == 4);
    CHECK(cb.full());
}

TEST_CASE("example: circular_buffer_bound_example") {
    bounded_buffer<int> bb_int(queue_size);
    auto got = fifo_test(&bb_int, std::function<int(unsigned long)>([](unsigned long i) {
        return static_cast<int>(i);
    }));
    REQUIRE(got.size() == total_elements);
    CHECK(std::is_sorted(got.begin(), got.end()));
    CHECK(got.back() == static_cast<int>(total_elements - 1));
}

TEST_CASE("example: bounded_buffer_comparison") {
    auto as_string = std::function<std::string(unsigned long)>([](unsigned long i) {
        return std::to_string(i);
    });
    bounded_buffer<std::string> a(queue_size);
    bounded_buffer_space_optimized<std::string> b(queue_size);
    bounded_buffer_deque_based<std::string> c(queue_size);
    auto ra = fifo_test(&a, as_string);
    auto rb = fifo_test(&b, as_string);
    auto rc = fifo_test(&c, as_string);
    CHECK(ra == rb);
    CHECK(rb == rc);
    CHECK(ra.front() == "0");
    CHECK(b.reserved() == queue_size);
}
