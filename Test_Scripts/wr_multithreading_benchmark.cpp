#include <iostream>
#include <vector>
#include <deque>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <vector>
#include <atomic>
#include <random>
#include <iomanip>
#include <omp.h>
#include "../tiered_vector.hpp"

using namespace std;
using namespace cppx;

/*

How to run:
g++ -O3 -fopenmp wr_multithreading_benchmark.cpp -o lock_test
./lock_test

*/


// Since vector reallocates, we MUST lock the whole thing for safety.
template <typename T>
class GlobalLockWrapper {
    std::vector<T> data;
    std::shared_mutex mtx; // Reader-Writer Lock

public:
    void resize(size_t n) {
        std::unique_lock lock(mtx);
        data.resize(n);
    }

    // Thread A writing index 0 BLOCKS Thread B writing index 1000
    void write(size_t idx, T val) {
        std::unique_lock lock(mtx); 
        data[idx] = val;
    }
};

// Since blocks never move, we only lock the specific block we touch.
template <typename T>
class SegmentedLockWrapper {
    tiered_vector<T> data;
    // We create a "Stripe" of mutexes. 
    // Ideally 1 mutex per block, or a fixed pool (e.g., 64 mutexes) to save RAM.
    // For this demo, let's map blocks to 128 mutexes.
    static const size_t NUM_LOCKS = 128; 
    std::mutex locks[NUM_LOCKS];

public:
    void resize(size_t n) {
        // Resize touches the spine, so we might need a global lock 
        // strictly for the resizing moment, but NOT for data access.
        // For this test, we assume size is pre-allocated to focus on ACCESS speed.
        data.resize(n);
    }

    // Thread A writing Block 0 runs PARALLEL to Thread B writing Block 1
    void write(size_t idx, T val) {
        // 1. Find which block this index belongs to
        size_t block_idx = idx >> 10;
        
        // 2. Map block to a mutex (Stripe)
        size_t lock_idx = block_idx % NUM_LOCKS;

        // 3. Lock ONLY that segment
        std::lock_guard<std::mutex> lock(locks[lock_idx]);
        data[idx] = val;
    }
};


const size_t N = 10'000'000;
const int NUM_OPS = 5'000'000'0; // 50 Million random writes

template <typename ContainerWrapper>
void run_concurrency_test(string name) {
    ContainerWrapper container;
    container.resize(N); // Pre-fill

    // Pre-calculate random indices to isolate Locking overhead from RNG overhead
    vector<size_t> indices(NUM_OPS);
    mt19937 rng(42);
    uniform_int_distribution<size_t> dist(0, N - 1);
    for(auto &x : indices) x = dist(rng);

    auto start = chrono::high_resolution_clock::now();

    // The Parallel Workload
    #pragma omp parallel for
    for (int i = 0; i < NUM_OPS; ++i) {
        container.write(indices[i], i);
    }

    auto end = chrono::high_resolution_clock::now();
    double duration = chrono::duration<double>(end - start).count();
    double ops_sec = NUM_OPS / duration;

    cout << left << setw(25) << name 
         << " | Time: " << fixed << setprecision(3) << duration << "s"
         << " | " << setprecision(1) << (ops_sec / 1e6) << " M ops/sec" << endl;
}

int main() {
    size_t threads = 16;
    omp_set_num_threads(threads); // Force high contention

    cout << "\n=============================================================\n";
    cout << "  LOCK GRANULARITY SHOWDOWN (16 Threads, 50M Random Writes)\n";
    cout << "=============================================================\n";
    cout << "Scenario: 16 threads trying to write to random indices.\n";
    cout << "Expectation: Vector locks globally. Tiered locks locally.\n\n";

    run_concurrency_test<GlobalLockWrapper<int>>("Global Lock (Vector)");
    run_concurrency_test<SegmentedLockWrapper<int>>("Segmented Lock (Tiered)");

    cout << "\n=============================================================\n";
    return 0;
}