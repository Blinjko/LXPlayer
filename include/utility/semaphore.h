#pragma once

#include <mutex>
#include <condition_variable>

namespace Utility
{
    /* Semaphore class
     * Description: This is a counting semaphore class
     * Why not use the one in the STL / Standard Library?
     * The semaphore in the STL is part of C++20 and I rather avoid using very very new features, when I can just implement them myself.
     */
    class Semaphore
    {
        public:
            Semaphore(int);
            Semaphore(const Semaphore&) = delete;

            ~Semaphore();

            int count();

            int post();
            int wait();

        private:

            int m_count;
            std::mutex m_mutex;
            std::condition_variable m_condition_variable;
    };
}
