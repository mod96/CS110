/**
 * File: thread-pool.cc
 * --------------------
 * Presents the implementation of the ThreadPool class.
 */

#include "thread-pool.h"
using namespace std;

ThreadPool::ThreadPool(size_t numThreads)
    : numThreads(numThreads), wts(numThreads), qSize(0),
      availableWorkers(0), workerAvailable(numThreads), workerLock(numThreads)
{
    dt = thread([this]()
                { dispatcher(); });
    for (size_t workerID = 0; workerID < numThreads; workerID++)
    {
        wts[workerID] = thread([this](size_t workerID)
                               { worker(workerID); },
                               workerID);
        workerAvailable[workerID].first = true;
    }
}
void ThreadPool::schedule(const function<void(void)> &thunk)
{
    /**
     * Push thunk into the q
     */
    qLock.lock();
    q.push(thunk);
    qLock.unlock();

    /**
     * increase qSize, signal all
     */
    lock_guard<mutex> lg(qSizeLock);
    qSize++;
    qSizeCV.notify_all();
}
void ThreadPool::wait()
{
    /**
     * There will be no calling of `schedule` until this function finishes.
     * Wait until qSize becomes 0
     */
    lock_guard<mutex> lg(qSizeLock);
    qSizeCV.wait(qSizeLock, [this]
                 { return qSize > 0; });

    /**
     * Wait until all workers finish thunk.
     */
    for (size_t workerID = 0; workerID < numThreads; workerID++)
    {
        availableWorkers.wait();
    }
    /**
     * restore semaphore
     */
    for (size_t workerID = 0; workerID < numThreads; workerID++)
    {
        availableWorkers.signal();
    }
}

ThreadPool::~ThreadPool()
{
    wait();
    /**
     * Now qSize is 0, no workers are running(all threads stuck at .wait())
     */
    getOut = true;

    /**
     * let qSize++ and signal that dispatcher can go and die.
     */
    qSizeLock.lock();
    qSize++;
    qSizeCV.notify_all();
    qSizeLock.unlock();

    /**
     * Since wait() has passed, no workers are running now.
     * signal workers so that they can go and die.
     */
    for (size_t workerID = 0; workerID < numThreads; workerID++)
    {
        workerLock[workerID]->signal();
    }

    /**
     * Join
     */
    dt.join();
    for (size_t workerID = 0; workerID < numThreads; workerID++)
    {
        wts[workerID].join();
        delete workerLock[workerID];
    }
}

void ThreadPool::dispatcher()
{
    while (true)
    {
        /**
         * Wait until qSize > 0,
         * decrease qSize.
         */
        qSizeLock.lock();
        qSizeCV.wait(qSizeLock, [this]
                     { return qSize == 0; });
        qSize--;
        qSizeLock.unlock();

        if (getOut)
            break;

        /**
         * Wait until availableWorkers > 0
         */
        availableWorkers.wait();
        for (size_t workerID = 0; workerID < numThreads; workerID++)
        {
            workerAvailable[workerID].second.lock();
            if (workerAvailable[workerID].first)
            {
                workerAvailable[workerID].first = false;
                qLock.lock();
                thunkShare[workerID] = q.front();
                q.pop();
                qLock.unlock();
                workerLock[workerID]->signal();
                workerAvailable[workerID].second.unlock();
                break;
            }
            workerAvailable[workerID].second.unlock();
        }
    }
}

void ThreadPool::worker(int id)
{
    workerLock[id] = new semaphore(0);
    availableWorkers.signal();
    while (true)
    {
        workerLock[id]->wait();
        if (getOut)
            break;
        const function<void(void)> &thunk = thunkShare[id];
        thunk();
        workerAvailable[id].second.lock();
        workerAvailable[id].first = true;
        workerAvailable[id].second.unlock();
        availableWorkers.signal();
    }
}