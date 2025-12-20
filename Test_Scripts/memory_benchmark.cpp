#include <iostream>
#include <vector>
#include <deque>
#include <iomanip>
#include <cmath>
#include <string>

#include "../tiered_vector.hpp"
using namespace std;
using namespace cppx;

// 1. std::vector Calculator
// Precise because vector spec guarantees contiguous memory
size_t get_memory_vector(const vector<int>& v) {
    return v.capacity() * sizeof(int);
}

// 2. std::deque Calculator (Estimation)
// Deque allocates fixed-size chunks (usually 512 bytes or 4KB) plus a map of pointers.
// We approximate based on GCC/Clang standard implementations.
size_t get_memory_deque(const deque<int>& d) {
    if (d.empty()) return sizeof(deque<int>);
    
    const size_t chunk_size_bytes = 512; // Common chunk size
    const size_t ints_per_chunk = chunk_size_bytes / sizeof(int); // ~128 ints
    
    size_t chunks = (d.size() + ints_per_chunk - 1) / ints_per_chunk;
    size_t map_overhead = chunks * sizeof(int*); // The spine
    
    return (chunks * chunk_size_bytes) + map_overhead;
}

// 3. Tiered Vector Calculator (Your Logic)
// Based on the code: 
// - Spine = array of pointers (block_cap)
// - Blocks = 1024 ints each
// - Actual allocated blocks = ceil(size / 1024)
size_t get_memory_tiered(const tiered_vector<int>& tv) {
    // We have to infer internal state since members are private.
    // Logic: capacity() returns (block_cap * 1024).
    size_t block_cap = tv.capacity() / 1024;
    
    // Logic: blocks allocated. 
    // In your push_back logic, you allocate a new block every 1024 items.
    // So allocated blocks is roughly (size + 1023) / 1024.
    size_t block_sz = (tv.size() + 1023) >> 10;
    
    size_t spine_bytes = block_cap * sizeof(int*);
    size_t data_bytes  = block_sz * 1024 * sizeof(int);
    
    return spine_bytes + data_bytes;
}


void print_header() {
    cout << "\n" << string(90, '=') << "\n";
    cout << left << setw(12) << "Elements" 
         << setw(25) << "std::vector (Waste %)" 
         << setw(25) << "std::deque (Waste %)" 
         << setw(25) << "TieredVec (Waste %)" << endl;
    cout << string(90, '=') << "\n";
}

string format_mem(size_t used_bytes, size_t ideal_bytes) {
    double mb = used_bytes / (1024.0 * 1024.0);
    
    double waste_ratio = 0.0;
    if (ideal_bytes > 0 && used_bytes > ideal_bytes) {
        waste_ratio = ((double)(used_bytes - ideal_bytes) / used_bytes) * 100.0;
    }
    
    stringstream ss;
    ss << fixed << setprecision(1) << mb << "MB";
    
    // Add waste percentage if significant
    if (waste_ratio > 1.0) {
        ss << " (" << setprecision(0) << waste_ratio << "%)";
    } else {
        ss << " (0%)";
    }
    return ss.str();
}

void print_row(size_t n, const vector<int>& v, const deque<int>& d, const tiered_vector<int>& tv) {
    size_t ideal_bytes = n * sizeof(int);
    
    cout << left << setw(12) << n 
         << setw(25) << format_mem(get_memory_vector(v), ideal_bytes)
         << setw(25) << format_mem(get_memory_deque(d), ideal_bytes)
         << setw(25) << format_mem(get_memory_tiered(tv), ideal_bytes)
         << endl;
}


int main() {
    // SCALES to test. We want to catch the "Doubling" points of vector.
    // Vector usually doubles at 1, 2, 4, 8... 
    // Let's pick points specifically designed to expose Vector's weakness.
    // 2^20 = ~1M. 2^20 + 1 forces a resize to 2M capacity.
    vector<size_t> checkpoints = {
        1000,
        100000, 
        1000000, 
        1048577,
        10000000,
        33554433,
        73000000
    };

    vector<int> v;
    deque<int> d;
    tiered_vector<int> tv;

    cout << "Starting Robust Memory Efficiency Benchmark...\n";
    cout << "Ideal Memory for 73M ints is ~278 MB.\n";

    // --- TEST 1: DYNAMIC GROWTH (The Hoarding Test) ---
    // We add elements and measure how much RAM is held vs used.
    print_header();
    
    size_t current_idx = 0;
    for (size_t n : checkpoints) {
        // Grow to N
        for (size_t i = current_idx; i < n; ++i) {
            v.push_back(i);
            d.push_back(i);
            tv.push_back(i);
        }
        current_idx = n;
        
        print_row(n, v, d, tv);
    }

    cout << "\n" << string(90, '-') << "\n";
    cout << "TEST 2: THE SHRINK TEST (Memory Reclamation)\n";
    cout << "Popping 50% of elements (36.5M items)...\n";
    cout << string(90, '-') << "\n";

    size_t target = 36500000;
    while (v.size() > target) {
        v.pop_back();
        d.pop_back();
        tv.pop_back();
    }

    print_row(target, v, d, tv);

    cout << "\n[ANALYSIS]\n";
    size_t v_mem = get_memory_vector(v);
    size_t tv_mem = get_memory_tiered(tv);
    

    return 0;
}