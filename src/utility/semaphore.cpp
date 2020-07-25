#include <utility/semaphore.h>

#include <mutex>
#include <condition_variable>

namespace Utility
{
    // Constructor
    // Parameter count - the starting count
    Semaphore::Semaphore(int count) : m_count{count}
    {}

    // Destructor
    Semaphore::~Semaphore()
    {
        m_count = 0;
        m_condition_variable.notify_all();
    }

    // count getter //
    int Semaphore::count()
    {
        m_mutex.lock();
        int count{m_count};
        m_mutex.unlock();

        return count;
    }

    /* post function
     * Description: adds 1 to m_count, if count was 0 it notifies another potential waiting thread
     * Return: returns the previous count
     */
    int Semaphore::post()
    {
        m_mutex.lock();
        int old_count{m_count++};
        m_mutex.unlock();

        if(old_count == 0)
        {
            m_condition_variable.notify_one();
        }

        return old_count;
    }

    /* wait function
     * Description: subtracts 1 from count, if count is 0 before subtraction, wait on m_condition_variable
     * Return: returns the count bufore subtraction
     */
    int Semaphore::wait()
    {
        std::unique_lock<std::mutex> lock{m_mutex};
        while(m_count == 0)
        {
            m_condition_variable.wait(lock);
        }

        int old_count{m_count--};
        lock.unlock();

        return old_count;
    }
}
