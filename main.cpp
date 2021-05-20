#include <iostream>
#include <vector>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <functional>


std::atomic_bool isToSpawn(true);
std::atomic_bool isToServe(true);

int customerServed = 0;

std::mutex mutex;

std::atomic_int lobby_size(0);
std::atomic_int couch_size(0);
std::atomic_int outside_size(0);

std::vector<std::thread> v;

std::condition_variable cv;

std::function<void()> client_func = []()
{
    outside_size++;

    std::unique_lock<std::mutex> lock(mutex);
    cv.wait(lock, [] { return lobby_size != 20; });
    std::cout << "Client moves inside" << std::endl;
    lock.unlock();

    outside_size--;
    lobby_size++;

    lock.lock();
    cv.wait(lock, [] { return couch_size != 4; });
    std::cout << "Client moves onto the couch" << std::endl;
    lock.unlock();

    lobby_size--;
    couch_size++;
};

std::function<void()> thread_spawner = []()
{
    int id = 0;
    while(isToSpawn)
    {
        std::unique_lock<std::mutex> lock(mutex);
        std::cout << "Spawning a client" << std::endl;
        lock.unlock();
        v.emplace_back(client_func);
    }
};

std::function<void()> action_performed = []()
{
    while(isToServe)
    {
        std::unique_lock<std::mutex> lock(mutex);
        if(couch_size > 0)
        {
            couch_size--;
            std::cout << "Serving a customer " << customerServed << std::endl;
            cv.notify_one();
            customerServed++;
            if(customerServed == 50)
            {
                isToSpawn = false;
                isToServe = false;
            }
        }
        lock.unlock();
    }
};

int main()
{
    std::thread spawner(thread_spawner);

    std::thread barber1(action_performed);
    std::thread barber2(action_performed);
    std::thread barber3(action_performed);

    barber1.join();
    barber2.join();
    barber3.join();

    spawner.join();

    lobby_size = 0;
    couch_size = 0;

    for (auto & it : v)
    {
        it.join();
    }
}

