----------------------------------- PERFORMANCE BENCHMARKS -----------------------------------

1) general_benchmark.cpp:

Context: 
    A high level overview comparing the three major operations: push_back, reading sequentially, random access and bytes/element

The Mechanism: 
    std::vector uses a single contiguous block of memory. When it is full, it allocates a new block 2x the size, copies everything and deletes the old block

    std::deque uses a map of pointers to small fixed-size arrays. It never copies existing elements, but the arrays are often small, leading to frequent allocations and cache misses.

    tiered_vector uses an array of pointers pointing to moderate-large sized data chunks, size in the power of two. When full, it adds new chunks, never moves existing data.

Expected Observation and Reason:
    push_back (no reserving memory initially) - tiered_vector dominates, and the reason is because no reallocation/copy of large amounts of data happens. std::deque loses here due to high allocator pressure

    seq_scan - std::vector wins, but tiered_vector is competitive. vector is a single block, allowing the CPU prefetcher to predict 100% of the memory accesses. tiered_vector introduces a hop every 1024 elements, which is a negligible penalty compared to std:deque's penalty

    random_access - std::vector wins, but tiered_vector is very competitive. The reason vector wins is simply because there is lesser math to do, compared to tiered_vector, which has two dereferences. The reason deque loses is because of more cache misses.

        -------------------------------------------------------------------------------------------------------------------
        Count       Type                Push(ms)    SeqScan(ms) RndAcc(ms)  Total(MB)     Bytes/Elem     
        -------------------------------------------------------------------------------------------------------------------
        1000000     tiered_vector       17.5        6.6         11.9        3.91          4.10           
        1000000     deque               67.2        79.2        81.7        2.76          2.89           
        1000000     vector              74.2        5.6         11.8        4.69          4.92           

        -------------------------------------------------------------------------------------------------------------------
        Count       Type                Push(ms)    SeqScan(ms) RndAcc(ms)  Total(MB)     Bytes/Elem     
        -------------------------------------------------------------------------------------------------------------------
        10000000    tiered_vector       171.2       66.6        59.5        36.33         3.81           
        10000000    deque               681.8       777.3       404.4       38.70         4.06           
        10000000    vector              759.6       46.0        47.9        45.02         4.72           

        -------------------------------------------------------------------------------------------------------------------
        Count       Type                Push(ms)    SeqScan(ms) RndAcc(ms)  Total(MB)     Bytes/Elem     
        -------------------------------------------------------------------------------------------------------------------
        33000000    tiered_vector       412.8       140.5       38.4        118.54        3.77           
        33000000    deque               1065.5      841.8       132.9       130.55        4.15           
        33000000    vector              803.9       57.2        18.3        156.76        4.98           

        -------------------------------------------------------------------------------------------------------------------
        Count       Type                Push(ms)    SeqScan(ms) RndAcc(ms)  Total(MB)     Bytes/Elem     
        -------------------------------------------------------------------------------------------------------------------
        60000000    tiered_vector       323.6       124.1       18.5        198.19        3.46           
        60000000    deque               1286.8      1473.6      131.6       238.38        4.17           
        60000000    vector              1480.9      103.9       18.3        259.76        4.54           

        -------------------------------------------------------------------------------------------------------------------
        Count       Type                Push(ms)    SeqScan(ms) RndAcc(ms)  Total(MB)     Bytes/Elem     
        -------------------------------------------------------------------------------------------------------------------
        65000000    tiered_vector       352.7       134.1       18.4        217.34        3.51           
        65000000    deque               1395.2      1592.0      127.6       258.34        4.17           
        65000000    vector              1590.1      112.7       18.3        278.83        4.50

2) speed_benchmark.cpp:

Context: 
    Aims to calculate micro-level bench marks, in two modes.. initially unreserved vs initially reserved.

The Mechanism:
    Simulates both unreserved and reserved growth. If the memory is initially reserved, no reallocation happens in a vector, which is the ideal case we assume.

Expected Observation and Reason:
    Unreserved Growth - tiered_vector wins for push_back operation, for reasons discussed earlier

    Reserved Growth - vector is king in all aspects, because no reallocation of elements happen at any time. In this case, it just pretty much behaves as fast as an array, approaching the RAM limit.. 

        ========================================================================================
        BENCHMARK: UNRESERVED GROWTH (push_back)
        ========================================================================================
        N Elements     std::vector       std::deque        TieredVec         Winner         
        ----------------------------------------------------------------------------------------
        1000           0.00004           0.00003           0.00001           TieredVec      
        10000          0.00040           0.00031           0.00010           TieredVec      
        100000         0.00384           0.00333           0.00089           TieredVec      
        1000000        0.04927           0.05785           0.01115           TieredVec      
        10000000       0.69572           0.57949           0.13706           TieredVec      
        73000000       1.75732           3.35112           1.21763           TieredVec      

        ========================================================================================
        BENCHMARK: RESERVED WRITE (operator[])
        ========================================================================================
        N Elements     std::vector       std::deque        TieredVec         Winner         
        ----------------------------------------------------------------------------------------
        1000           0.00000           0.00002           0.00000           Vector         
        10000          0.00002           0.00023           0.00002           Vector         
        100000         0.00016           0.00236           0.00019           Vector         
        1000000        0.00161           0.02399           0.00191           Vector         
        10000000       0.01589           0.23690           0.01936           Vector         
        73000000       0.11689           1.73158           0.14085           Vector         

        ========================================================================================
        BENCHMARK: SEQUENTIAL READ (Linear Scan)
        ========================================================================================
        N Elements     std::vector       std::deque        TieredVec         Winner         
        ----------------------------------------------------------------------------------------
        1000           0.00000           0.00002           0.00000           Vector         
        10000          0.00002           0.00023           0.00002           Vector         
        100000         0.00027           0.00234           0.00020           TieredVec      
        1000000        0.00174           0.02370           0.00230           Vector         
        10000000       0.01659           0.23787           0.02046           Vector         
        73000000       0.12037           1.72742           0.15057           Vector         

        ========================================================================================
        BENCHMARK: RANDOM ACCESS (Index Hop)
        ========================================================================================
        N Elements     std::vector       std::deque        TieredVec         Winner         
        ----------------------------------------------------------------------------------------
        1000           0.00001           0.00003           0.00001           Vector (Close) 
        10000          0.00007           0.00028           0.00007           TieredVec      
        100000         0.00067           0.00304           0.00083           Vector         
        1000000        0.01025           0.03681           0.01387           Vector         
        10000000       0.50245           1.19925           0.52536           Vector (Close) 
        73000000       4.60984           11.60753          5.10716           Vector

3) memory_benchmark.cpp

Context: 
    Measures wasted memory, i.e. memory allocated, but not containing user data

Mechanism: 
    std::vector as discussed earlier, reallocates n to 2*n blocks, wasting memory and creating an overhead, for a major boost in speed.

    tiered_vector Allocates fixed_size chunks. If a chunk holds 1024 items, waste is never more than 1023 items, regardless of the total size, if assumed to push_back sequentially.

    Waste becomes 1023 elements per block at worst case scenario, if not assumed to push_back sequentially by reserving the pointers initially.

Expected Observation and Reason:
    pretty trivial... tiered_vector and deque has minimal waste, while vector has a huge overhead as n scales.

        ==========================================================================================
        Elements    std::vector (Waste %)    std::deque (Waste %)     TieredVec (Waste %)      
        ==========================================================================================
        1000        0.0MB (2%)               0.0MB (4%)               0.0MB (4%)               
        100000      0.5MB (24%)              0.4MB (2%)               0.4MB (0%)               
        1000000     4.0MB (5%)               3.9MB (2%)               3.8MB (0%)               
        1048577     8.0MB (50%)              4.1MB (2%)               4.0MB (0%)               
        10000000    64.0MB (40%)             38.7MB (2%)              38.3MB (0%)              
        33554433    256.0MB (50%)            130.0MB (2%)             128.5MB (0%)             
        73000000    512.0MB (46%)            282.8MB (2%)             279.5MB (0%)             

        ------------------------------------------------------------------------------------------
        TEST 2: THE SHRINK TEST (Memory Reclamation)
        Popping 50% of elements (36.5M items)...
        ------------------------------------------------------------------------------------------
        36500000    512.0MB (73%)            141.4MB (2%)             140.2MB (0%)             


4)wr_multithreaded.cpp

Context:
    The "killer feature" of this data structure. 16 threads attempting to write/read random indices simultaneously.
    Have not implemented thread safety in tiered_vector yet, so we will be dealing with w/r operations for now

Mechanism: 
    Global Locking (Vector): Because std::vector might resize and move memory, pointer reference is lost during realloc, and hence it enforces serial execution (global lock).

    Segmented Locking (Tiered_Vector): Because tiered_vector guarantess reference stability, we can use mutex per chunk. Thread A writing to Chunk 1 does not block Thread B writing to Chunk 2.

Expected Observation and Reason:
    Tiered vector is effectively running in parallel in the average case (16 threads working at once in the best case). This makes tiered_vector terribly more efficient for this nieche case scenario.

        =============================================================
        LOCK GRANULARITY SHOWDOWN (16 Threads, 50M Random Writes)
        =============================================================
        Scenario: 16 threads trying to write to random indices.
        Expectation: Vector locks globally. Tiered locks locally.

        Global Lock (Vector)      | Time: 19.938s | 2.5 M ops/sec
        Segmented Lock (Tiered)   | Time: 0.523s | 95.6 M ops/sec

        =============================================================