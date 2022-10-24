#!/bin/bash

function success() {
    echo "[ OK ]  ($1)"
    exit 0
}

function failure() {
    echo "[FAIL]: $1 ($2)"
    exit 1
}

MEMORY_LEAK_EXIT_CODE=255
CNF_FILE=$2
LOG_FILE="logs/$CNF_FILE.log"

test -e $1 || failure "Binary doesn't exist at $1" $2
test -e $CNF_FILE || failure "CNF file doesn't exist at $CNF_FILE" $2
mkdir -p logs && rm -f $LOG_FILE && touch $LOG_FILE
valgrind --leak-check=full --track-origins=yes --error-exitcode=$MEMORY_LEAK_EXIT_CODE --log-file=$LOG_FILE -s $1 $CNF_FILE &> /dev/null

if [[ $? -eq $MEMORY_LEAK_EXIT_CODE ]]; then
    failure "Memory leak found" $2
else
    success $2
fi

