#include <benchmark/benchmark.h>
#include <pthread.h>
#include <vector>
#include <iostream>
#include <atomic>
#include <chrono>

pthread_mutex_t mutex_lock;  // Use pthread mutex
pthread_rwlock_t rwlock;      // Use pthread read-write lock
std::atomic<bool> keep_running; // Atomic variable to control thread execution

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

// Thread worker function for mutex benchmark
void* mutex_worker(void* arg) {
    int id = *static_cast<int*>(arg);
    while (keep_running.load()) {
        write_operation(id);
    }
    return nullptr;
}

// Thread worker function for read operation benchmark
void* rwlock_worker(void* arg) {
    int id = *static_cast<int*>(arg);
    while (keep_running.load()) {
        read_operation(id);
    }
    return nullptr;
}

// Benchmark for pthread mutex
static void BM_PthreadMutex(benchmark::State& state) {
    int num_threads = state.range(0); // Get the number of threads from the state

    // Create threads
    std::vector<pthread_t> threads(num_threads);
    std::vector<int> ids(num_threads);
    for (int i = 0; i < num_threads; ++i) {
        ids[i] = i; // Assign IDs
        pthread_create(&threads[i], nullptr, mutex_worker, &ids[i]);
    }

    for (auto _ : state) {
        keep_running = true; // Start the benchmark
        benchmark::DoNotOptimize(keep_running); // Prevent optimization of the loop
    }

    keep_running = false; // Stop the threads

    // Wait for threads to finish
    for (auto& thread : threads) {
        pthread_join(thread, nullptr);
    }
}
BENCHMARK(BM_PthreadMutex)->Arg(1)->Arg(2)->Arg(4)->Arg(6)->Arg(8)->Arg(10)->Arg(12)->Arg(14)->Arg(16);

// Benchmark for pthread read-write lock
static void BM_PthreadRWLock(benchmark::State& state) {
    int num_threads = state.range(0); // Get the number of threads from the state

    // Create threads
    std::vector<pthread_t> threads(num_threads);
    std::vector<int> ids(num_threads);
    for (int i = 0; i < num_threads; ++i) {
        ids[i] = i; // Assign IDs
        pthread_create(&threads[i], nullptr, rwlock_worker, &ids[i]);
    }

    for (auto _ : state) {
        keep_running = true; // Start the benchmark
        benchmark::DoNotOptimize(keep_running); // Prevent optimization of the loop
    }

    keep_running = false; // Stop the threads

    // Wait for threads to finish
    for (auto& thread : threads) {
        pthread_join(thread, nullptr);
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
