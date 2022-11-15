#ifndef _THREAD_POOL_H_
#define _THREAD_POOL_H_

#include <thread>
#include <iostream>
#include <functional>
#include <mutex>
#include <queue>
#include <condition_variable>
using namespace std;

class thread_pool {
private:
    queue<function<void()>> tasks;  // tasks store the object of function<void()>
                                    // tasks.front()() can execute a function
    mutex lock;
    condition_variable not_empty;   // conditional variable
private:    
    int size;
    static const int default_size = 4;
private:
    bool is_shutdown;
public:
    thread_pool();
    thread_pool(int);
    ~thread_pool();
    void add_job(function<void(void*)>, void*);
    void add_job(function<void()>);
private:
    void run();
};


thread_pool::thread_pool(int size) : size(size) {
    is_shutdown = false; 
    for (int i = 0; i < this->size; i++)
        thread(&thread_pool::run, this).detach();   //create threads which call thread_poll::run, then detach them
}

thread_pool::thread_pool() : thread_pool(default_size) {}

void thread_pool::add_job(function<void(void*)> func, void* arg) {
    lock.lock();
    // bind(func, arg)();
    // function<void(void)> f = bind(func, arg);
    // tasks.push( move(f) );
    tasks.push([=]() { return func(arg); });    // use lambda to return an object which will afterly converted to function<void(void)>
    not_empty.notify_all();
    lock.unlock();
}

void thread_pool::add_job(function<void()> func) {
    lock.lock();
    // bind(func, arg)();
    // function<void(void)> f = bind(func, arg);
    // tasks.push( move(f) );
    tasks.push(func);// use lambda to return an object which will afterly converted to function<void(void)>
    not_empty.notify_all();
    lock.unlock();
}

thread_pool::~thread_pool() {
    is_shutdown = true; // shut down the thread_pool
    for (int i = 0; i < size; i++)
        not_empty.notify_all();
}

void thread_pool::run() {
    unique_lock<mutex> ul(lock);
    cout << "in thread:" << this_thread::get_id() << endl;
    while (1) {
        // while (tasks.size() == 0 && is_shutdown == false) {
        //     not_empty.wait(ul);
        // }
        not_empty.wait(ul, [&]() {  // use conditiolal variable. it will autonomously lock the mutex
            return tasks.size() != 0 || is_shutdown == true;
        });
        if (is_shutdown) {
            break;
        }
        cout << "thread:" << this_thread::get_id() << "runs.\n";
        // lock.lock();
        function<void()> f = tasks.front();
        tasks.pop();
        lock.unlock();      // unlock the mutex that conditional_variable locked
        f();
    }
    cout << "thread:" << this_thread::get_id() << "terminates.\n";  // may not be shown in the ternimal because threads are detach
}

#endif