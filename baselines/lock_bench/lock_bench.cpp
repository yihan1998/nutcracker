#include <benchmark/benchmark.h>
#include <pthread.h>
#include <vector>
#include <iostream>
#include <cstdlib>

pthread_mutex_t mutex_lock;  // Use pthread mutex
pthread_rwlock_t rwlock;      // Use pthread read-write lock

// Function to perform a write operation
void write_operation(int id) {
    pthread_mutex_lock(&mutex_lock);
    // Simulate some work
    benchmark::DoNotOptimize(id);
    pthread_mutex_unlock(&mutex_lock);
}

// Function to perform a read operation
void read_operation(int id) {
    pthread_rwlock_rdlock(&rwlock);
    // Simulate some work
    benchmark::DoNotOptimize(id);
    pthread_rwlock_unlock(&rwlock);
}

// Benchmark for pthread mutex
static void BM_PthreadMutex(benchmark::State& state) {
    int num_threads = state.range(0); // Get the number of threads from the state
    for (auto _ : state) {
        std::vector<pthread_t> threads(num_threads);
        for (int i = 0; i < num_threads; ++i) {
            pthread_create(&threads[i], nullptr, [](void* arg) -> void* {
                int id = *static_cast<int*>(arg);
                write_operation(id);
                return nullptr;
            }, &i);
        }
        for (auto& thread : threads) {
            pthread_join(thread, nullptr);
        }
    }
}
BENCHMARK(BM_PthreadMutex)->Arg(1)->Arg(2)->Arg(4)->Arg(6)->Arg(8)->Arg(10)->Arg(12)->Arg(14)->Arg(16);

// Benchmark for pthread read-write lock
static void BM_PthreadRWLock(benchmark::State& state) {
    int num_threads = state.range(0); // Get the number of threads from the state
    for (auto _ : state) {
        std::vector<pthread_t> threads(num_threads);
        for (int i = 0; i < num_threads; ++i) {
            pthread_create(&threads[i], nullptr, [](void* arg) -> void* {
                int id = *static_cast<int*>(arg);
                read_operation(id);
                return nullptr;
            }, &i);
        }
        for (auto& thread : threads) {
            pthread_join(thread, nullptr);
        }
    }
}
BENCHMARK(BM_PthreadRWLock)->Arg(1)->Arg(2)->Arg(4)->Arg(6)->Arg(8)->Arg(10)->Arg(12)->Arg(14)->Arg(16);

// Main function to set up locks and run benchmarks
int main(int argc, char** argv) {
    // Initialize the locks
    pthread_mutex_init(&mutex_lock, nullptr);
    pthread_rwlock_init(&rwlock, nullptr);
    
    ::benchmark::Initialize(&argc, argv);
    ::benchmark::RunSpecifiedBenchmarks();
    
    // Destroy the locks
    pthread_mutex_destroy(&mutex_lock);
    pthread_rwlock_destroy(&rwlock);
    return 0;
}
