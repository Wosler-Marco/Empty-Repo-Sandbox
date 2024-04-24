#!/usr/bin/env bash

ALL=OFF

while [[ $# -gt 0 ]]; do
    case "$1" in
        -a|--all)
            ALL=ON
            shift
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

cd ./build
# Call make with specified options
if [ "$ALL" == "ON" ]; then
    # Get the number of CPU cores/threads
    num_cores=$(nproc)
    echo "Number of CPU cores/threads: $num_cores"

    # Set the desired resource limit (e.g., 80% CPU usage)
    desired_cpu_usage=60

    # Calculate the optimal -j value based on CPU cores and resource limit
    j_value=$((num_cores * desired_cpu_usage / 100))
    echo "Calculated -j value: $j_value"

    make -j "$j_value"
else
    make
fi