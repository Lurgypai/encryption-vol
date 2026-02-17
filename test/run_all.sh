#!/bin/bash

for file in configs/*; do
    echo "Running ${file}."
    ./run_benchmark.sh ${file}
done
