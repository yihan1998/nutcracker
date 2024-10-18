#!/bin/bash

# Output file for benchmark results
output_file="benchmark_results.txt"

# Clear previous results
echo "" > $output_file

# Iterate over the specified number of threads
for num_threads in 1 2 4 6 8 10 12 14 16; do
    echo "Running benchmarks with $num_threads threads..."
    
    # Run the benchmark and save output to the output file
    ./lock_bench --benchmark_filter=BM_PthreadMutex --benchmark_min_time=1s -- --num_threads=$num_threads >> $output_file
    ./lock_bench --benchmark_filter=BM_PthreadRWLock --benchmark_min_time=1s -- --num_threads=$num_threads >> $output_file
    
    echo "Results saved for $num_threads threads."
done

echo "Benchmarking complete. Results saved in $output_file."
