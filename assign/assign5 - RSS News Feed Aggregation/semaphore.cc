#include "semaphore.h"
#include <mutex>
#include <condition_variable>
#include <memory>
using namespace std;

semaphore::semaphore(int value) : value(value) {}

void semaphore::wait()
{
    lock_guard<mutex> lg(m);
    cv.wait(m, [this]
            { return value > 0; });
    value--;
}

void semaphore::signal()
{
    lock_guard<mutex> lg(m);
    value++;
    if (value == 1)
        cv.notify_all();
}

struct pthread_value_t
{
    pthread_value_t(const pthread_key_t &key, semaphore &s) : key(key), s(s) {}
    pthread_key_t key;
    semaphore &s;
};

void semaphore::signal(on_thread_exit_t ote)
{
    // code that follows is based on code presented in a stackoverflow article,
    // the URL to which is presented in the header comment of this file.
    pthread_key_t key;
    pthread_key_create(&key, [](void *value)
                       {
        unique_ptr<pthread_value_t> data(static_cast<pthread_value_t *>(value));
        data->s.signal();
        pthread_key_delete(data->key); });
    pthread_setspecific(key, new pthread_value_t(key, *this));
}