#!/bin/bash

cd $(dirname $0)
for cnf_file in $(find . -name "*.cnf" -type f); do
    ./run-single-test.sh $1 $cnf_file;
done

