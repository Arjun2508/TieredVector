#include <iostream>
#include <iomanip>
#include <vector>
#include <deque>
#include <chrono>
#include <random>
#include <numeric>
#include <algorithm>

#include "../tiered_vector.hpp"
using namespace std;
using namespace cppx; 

const vector<size_t> SCALES = {
    1000, 
    10000, 
    100000, 
    1000000, 
    10000000, 
    73000000 // Target Scale
};

using Clock = std::chrono::high_resolution_clock;

// Prevents compiler from deleting loops that don't produce visible output
template <typename T>
void do_not_optimize(T const& value) {
    asm volatile("" : : "r,m"(value) : "memory");
}

// Helper to generate random indices (Outside the timer)
vector<size_t> generate_random_indices(size_t n) {
    vector<size_t> indices(n);
    // Fill with 0..n-1
    iota(indices.begin(), indices.end(), 0);
    
    // Shuffle to create random access pattern
    // Using a cheap LCG for speed on setup, standard mt19937 is slow for 73M
    mt19937 rng(42);
    shuffle(indices.begin(), indices.end(), rng);
    
    return indices;
}


// 1. UNRESERVED WRITE (Dynamic Growth)
template <typename Container>
double test_unreserved(size_t n) {
    Container c;
    auto start = Clock::now();
    for (size_t i = 0; i < n; ++i) {
        c.push_back((int)i);
    }
    auto end = Clock::now();
    do_not_optimize(c.size());
    return std::chrono::duration<double>(end - start).count();
}

// 2. RESERVED WRITE (Overwrite)
template <typename Container>
double test_reserved_write(size_t n) {
    Container c;
    // We resize first to ensure memory exists. 
    // This isolates the "Write Speed" from "Allocation Speed".
    c.resize(n); 
    
    auto start = Clock::now();
    for (size_t i = 0; i < n; ++i) {
        c[i] = (int)i;
    }
    auto end = Clock::now();
    do_not_optimize(c[n-1]);
    return std::chrono::duration<double>(end - start).count();
}

// 3. SEQUENTIAL READ (Summation)
template <typename Container>
double test_sequential_read(size_t n) {
    Container c;
    c.resize(n);
    // Fill with data first (untimed)
    for(size_t i=0; i<n; ++i) c[i] = (int)i;

    volatile long long sum = 0;
    
    auto start = Clock::now();
    for (size_t i = 0; i < n; ++i) {
        sum += c[i];
    }
    auto end = Clock::now();
    
    do_not_optimize(sum);
    return std::chrono::duration<double>(end - start).count();
}

// 4. RANDOM ACCESS (Latency Check)
template <typename Container>
double test_random_read(size_t n, const vector<size_t>& indices) {
    Container c;
    c.resize(n);
    // Fill with data first (untimed)
    for(size_t i=0; i<n; ++i) c[i] = (int)i;

    volatile long long sum = 0;

    auto start = Clock::now();
    // Use the pre-calculated random indices to test pure access time
    // without paying the cost of RNG generation inside the loop
    for (size_t idx : indices) {
        sum += c[idx];
    }
    auto end = Clock::now();

    do_not_optimize(sum);
    return std::chrono::duration<double>(end - start).count();
}


void print_header(string mode) {
    cout << "\n========================================================================================\n";
    cout << " BENCHMARK: " << mode << "\n";
    cout << "========================================================================================\n";
    cout << left << setw(15) << "N Elements" 
         << setw(18) << "std::vector" 
         << setw(18) << "std::deque" 
         << setw(18) << "TieredVec" 
         << setw(15) << "Winner" << endl;
    cout << "----------------------------------------------------------------------------------------\n";
}

void print_row(size_t n, double v, double d, double t) {
    string winner = "Tie";
    if (v <= d && v <= t) winner = "Vector";
    else if (d <= v && d <= t) winner = "Deque";
    else if (t <= v && t <= d) winner = "TieredVec";

    // Mark as "Close" if within 10% of Vector
    if (winner == "Vector" && t < v * 1.10) winner += " (Close)";

    cout << left << setw(15) << n 
         << setw(18) << fixed << setprecision(5) << v 
         << setw(18) << fixed << setprecision(5) << d 
         << setw(18) << fixed << setprecision(5) << t 
         << setw(15) << winner << endl;
}

int main() {
    cout << "Starting Comprehensive Benchmark...\n";
    cout << "Scaling to " << SCALES.back() << " items.\n";

    // 1. UNRESERVED GROWTH
    print_header("UNRESERVED GROWTH (push_back)");
    for (size_t n : SCALES) {
        print_row(n, 
            test_unreserved<vector<int>>(n),
            test_unreserved<deque<int>>(n),
            test_unreserved<tiered_vector<int>>(n));
    }

    // 2. RESERVED WRITE
    print_header("RESERVED WRITE (operator[])");
    for (size_t n : SCALES) {
        print_row(n, 
            test_reserved_write<vector<int>>(n),
            test_reserved_write<deque<int>>(n),
            test_reserved_write<tiered_vector<int>>(n));
    }

    // 3. SEQUENTIAL READ
    print_header("SEQUENTIAL READ (Linear Scan)");
    for (size_t n : SCALES) {
        print_row(n, 
            test_sequential_read<vector<int>>(n),
            test_sequential_read<deque<int>>(n),
            test_sequential_read<tiered_vector<int>>(n));
    }

    // 4. RANDOM ACCESS
    // Note: Generating random indices for 73M items takes memory/time, 
    // so we will cap the random test or be patient.
    print_header("RANDOM ACCESS (Index Hop)");
    for (size_t n : SCALES) {
        // Generate indices ONCE outside the timer
        // For the 73M test, this Setup Phase might take a few seconds.
        cout << "\r[Setup: Generating " << n << " indices...] " << flush; 
        vector<size_t> indices = generate_random_indices(n);
        cout << "\r" << string(40, ' ') << "\r"; // clear line

        print_row(n, 
            test_random_read<vector<int>>(n, indices),
            test_random_read<deque<int>>(n, indices),
            test_random_read<tiered_vector<int>>(n, indices));
    }

    cout << "\nBenchmark Complete.\n";
    return 0;
}