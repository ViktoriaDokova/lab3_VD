#include <chrono>
#include <iostream>
#include <latch>
#include <syncstream>
#include <thread>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

constexpr int nt = 4;

struct Action {
    char set;
    int index;
};

// Sets a..k are contiguous letters, so a set is indexed by (set - 'a').
// Each latch counts the actions of its set that are not finished yet.
std::latch done[] = {
    std::latch{4}, std::latch{8}, std::latch{6}, std::latch{6}, std::latch{7}, std::latch{7},
    std::latch{6}, std::latch{7}, std::latch{5}, std::latch{9}, std::latch{3},
};

// Sets that must be fully finished before an action of the given set can start.
const char* const deps[] = {"", "", "", "bc", "a", "hj", "a", "dg", "e", "e", "i"};

// With equal action durations this schedule takes 17 time units (68 actions / 4 threads).
const std::vector<Action> plan[nt] = {
    {{'b', 1}, {'b', 5}, {'c', 1}, {'c', 4}, {'d', 1}, {'d', 4}, {'g', 1}, {'g', 4}, {'h', 1},
     {'h', 4}, {'h', 7}, {'i', 1}, {'i', 3}, {'i', 5}, {'k', 1}, {'k', 3}, {'f', 4}},
    {{'b', 2}, {'b', 6}, {'c', 2}, {'c', 5}, {'d', 2}, {'d', 5}, {'g', 2}, {'g', 5}, {'h', 2},
     {'h', 5}, {'e', 5}, {'i', 2}, {'i', 4}, {'j', 5}, {'j', 8}, {'f', 1}, {'f', 5}},
    {{'b', 3}, {'b', 7}, {'c', 3}, {'c', 6}, {'d', 3}, {'d', 6}, {'g', 3}, {'g', 6}, {'h', 3},
     {'h', 6}, {'e', 6}, {'j', 1}, {'j', 3}, {'j', 6}, {'j', 9}, {'f', 2}, {'f', 6}},
    {{'b', 4}, {'b', 8}, {'a', 1}, {'a', 2}, {'a', 3}, {'a', 4}, {'e', 1}, {'e', 2}, {'e', 3},
     {'e', 4}, {'e', 7}, {'j', 2}, {'j', 4}, {'j', 7}, {'k', 2}, {'f', 3}, {'f', 7}},
};

void f(char x, int i) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::osyncstream(std::cout) << "З набору " << x << " виконано дію " << i << ".\n";
}

void worker(int t) {
    for (const Action& action : plan[t]) {
        for (const char* p = deps[action.set - 'a']; *p != '\0'; ++p) {
            done[*p - 'a'].wait();
        }
        f(action.set, action.index);
        done[action.set - 'a'].count_down();
    }
}

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    std::osyncstream(std::cout) << "Обчислення розпочато.\n";
    auto start = std::chrono::steady_clock::now();

    {
        std::vector<std::jthread> threads;
        for (int t = 0; t < nt; ++t) {
            threads.emplace_back(worker, t);
        }
    }  // jthreads join here

    std::chrono::duration<double> elapsed = std::chrono::steady_clock::now() - start;
    std::osyncstream(std::cout) << "Обчислення завершено. Тривалість обчислення "
                                << elapsed.count() << " секунд.\n";
    return 0;
}
