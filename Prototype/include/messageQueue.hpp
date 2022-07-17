#ifndef FEATURESPY_MESSAGEQUEUE_HPP
#define FEATURESPY_MESSAGEQUEUE_HPP

#include "configure.hpp"
#include "dataStructure.hpp"
#include <boost/atomic.hpp>
#include <boost/lockfree/queue.hpp>
#include <boost/lockfree/spsc_queue.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>

template <class T>
class messageQueue {
    boost::lockfree::queue<T, boost::lockfree::capacity<5000>> lockFreeQueue_;

public:
    boost::atomic<bool> done_;
    messageQueue()
    {
        done_ = false;
    }
    ~messageQueue()
    {
    }
    bool push(T& data)
    {
        while (!lockFreeQueue_.push(data))
            ;
        return true;
    }
    bool pop(T& data)
    {
        bool status = lockFreeQueue_.pop(data);
        return status;
    }
    bool isEmpty()
    {
        return lockFreeQueue_.empty();
    }
};

#endif //FEATURESPY_MESSAGEQUEUE_HPP