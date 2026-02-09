#!/bin/bash

for file in config/*; do
    echo "Running ${file}."
    ./run_benchmark.sh ${file}
done
