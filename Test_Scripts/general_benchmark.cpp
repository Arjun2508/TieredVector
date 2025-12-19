//source: gemini
#include <iostream>
#include <vector>
#include <deque>
#include <chrono>
#include <iomanip>
#include <fstream>
#include <unistd.h>
#include <numeric>
#include <cmath>
#include <string>
#include "../tiered_vector.hpp"

using namespace std;
using namespace cppx;
#define endl "\n"
// =========================================================
// 2. SYSTEM METRICS (Linux Specific)
// =========================================================

// Reads Current Resident Set Size (RSS) in Bytes
// Accurate to the page level (4KB)
size_t get_ram_usage() {
    long rss = 0;
    ifstream statm("/proc/self/statm");
    if (statm) {
        long program, resident;
        statm >> program >> resident;
        rss = resident * sysconf(_SC_PAGESIZE);
    }
    return rss;
}

class Timer {
    using Clock = chrono::high_resolution_clock;
    chrono::time_point<Clock> start;
public:
    Timer() { reset(); }
    void reset() { start = Clock::now(); }
    double ms() {
        return chrono::duration<double, milli>(Clock::now() - start).count();
    }
};

// =========================================================
// 3. BENCHMARK SUITE
// =========================================================

struct Result {
    double push_ms;
    double seq_ms;  // Sequential Read (Prefetching test)
    double rnd_ms;  // Random Read (Latency test)
    size_t mem_bytes;
};

template <typename Container>
Result run_test(size_t N) {
    // 1. Reset Heap State (Best Effort)
    // Allocate and free a chunk to scrub previous allocator artifacts
    { std::vector<char> scrub(1024 * 1024, 1); }
    
    size_t mem_start = get_ram_usage();
    Timer t;

    // --- TEST 1: PUSH BACK (Growth Speed) ---
    Container* c = new Container();
    t.reset();
    for(size_t i = 0; i < N; ++i) {
        c->push_back(static_cast<int>(i));
    }
    double t_push = t.ms();

    // --- TEST 2: MEMORY DELTA (Efficiency) ---
    size_t mem_peak = get_ram_usage();
    size_t mem_used = (mem_peak > mem_start) ? (mem_peak - mem_start) : 0;

    // --- TEST 3: SEQUENTIAL SCAN (Hardware Prefetching) ---
    // Simulates: for(size_t i=0; i<N; ++i) sum += c[i];
    volatile long long sum = 0;
    t.reset();
    for(size_t i = 0; i < N; ++i) {
        sum += (*c)[i];
    }
    double t_seq = t.ms();

    // --- TEST 4: RANDOM ACCESS (Cache/TLB Latency) ---
    // Access 10% of elements (capped at 5M ops) randomly
    size_t ops = std::min(N, (size_t)5000000);
    // Linear Congruential Generator for fast, deterministic randomness
    // (std::mt19937 is too slow and becomes the bottleneck itself)
    size_t idx = 0;
    t.reset();
    for(size_t i = 0; i < ops; ++i) {
        idx = (idx * 1664525 + 1013904223) % N;
        sum += (*c)[idx];
    }
    double t_rnd = t.ms();

    delete c;
    return {t_push, t_seq, t_rnd, mem_used};
}

// =========================================================
// 4. REPORTING
// =========================================================

void print_header() {
    cout << string(115, '-') << endl;
    cout << left << setw(12) << "Count" 
         << setw(20) << "Type" 
         << setw(12) << "Push(ms)" 
         << setw(12) << "SeqScan(ms)" 
         << setw(12) << "RndAcc(ms)" 
         << setw(14) << "Total(MB)" 
         << setw(15) << "Bytes/Elem" << endl;
    cout << string(115, '-') << endl;
}

void print_row(size_t N, string name, Result r) {
    double mb = r.mem_bytes / (1024.0 * 1024.0);
    double bpe = (double)r.mem_bytes / N;
    
    cout << left << setw(12) << N 
         << setw(20) << name 
         << setw(12) << fixed << setprecision(1) << r.push_ms 
         << setw(12) << r.seq_ms 
         << setw(12) << r.rnd_ms 
         << setw(14) << setprecision(2) << mb 
         << setw(15) << setprecision(2) << bpe 
         << endl;
}

int main() {
    // SCALING STRATEGY:
    // 1M:   Warmup / L3 Cache Fits
    // 10M:  Exceeds Cache (RAM Latency kicks in)
    // 33M:  Vector Tipping Point 1 (Just crossed 32M -> Resized to 64M)
    // 60M:  Vector Stable Zone (Vector fits tightly in 64M capacity)
    // 65M:  Vector Tipping Point 2 (Just crossed 64M -> Resized to 128M)
    vector<size_t> sizes = {1000000, 10000000, 33000000, 60000000, 65000000};

    cout << "===================================================================================================================\n";
    cout << " FINAL BENCHMARK: tiered_vector vs Vector vs Deque (Linux x64)\n";
    cout << " Purpose: Demonstrate Memory Stability & Access Pattern Bottlenecks\n";
    cout << "===================================================================================================================\n";

    for(size_t N : sizes) {
        print_header();
        
        // 1. tiered_vector (The Contender)
        print_row(N, "tiered_vector", run_test<tiered_vector<int>>(N));
        
        // 2. Deque (The Standard Alternative)
        print_row(N, "deque", run_test<deque<int>>(N));

        // 3. Vector (The Speed King / Memory Hog)
        print_row(N, "vector", run_test<vector<int>>(N));
        
        cout << endl;
    }

    return 0;
}
