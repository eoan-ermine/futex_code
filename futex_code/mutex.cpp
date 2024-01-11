#include "mutex.hpp"

#include <iostream>
#include <thread>

// Simple function that increments the value pointed to by n, 10 million times.
// If m is not nullptr, it's a Mutex that will be used to protect the increment
// operation.
void threadfunc(int64_t *n, Mutex *m = nullptr) {
    for (int i = 0; i < 10000000; ++i) {
        if (m != nullptr) {
            m->lock();
        }
        *n += 1;
        if (m != nullptr) {
            m->unlock();
        }
    }
}

int main(int argc, char **argv) {
    {
        int64_t vnoprotect = 0;
        std::thread t1(threadfunc, &vnoprotect, nullptr);
        std::thread t2(threadfunc, &vnoprotect, nullptr);
        std::thread t3(threadfunc, &vnoprotect, nullptr);

        t1.join();
        t2.join();
        t3.join();

        std::cout << "vnoprotect = " << vnoprotect << "\n";
    }

    {
        int64_t v = 0;
        Mutex m;

        std::thread t1(threadfunc, &v, &m);
        std::thread t2(threadfunc, &v, &m);
        std::thread t3(threadfunc, &v, &m);

        t1.join();
        t2.join();
        t3.join();

        std::cout << "v = " << v << "\n";
    }

    return 0;
}