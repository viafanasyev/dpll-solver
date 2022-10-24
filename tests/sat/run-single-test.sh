#!/bin/bash

function success() {
    echo "[ OK ]  ($1)"
    exit 0
}

function failure() {
    echo "[FAIL]: $1 ($2)"
    exit 1
}

CNF_FILE=$2
OUT_FILE="$2.out"

test -e $1 || failure "Binary doesn't exist at $1" $2
test -e $CNF_FILE || failure "CNF file doesn't exist at $CNF_FILE" $2
test -e $OUT_FILE || failure "OUT file doesn't exist at $OUT_FILE" $2

ACT_OUTPUT="$($1 $CNF_FILE)"
if [[ $? -ne 0 ]]; then
    failure "Program terminated with non-zero exit code" $2
fi

EXP_OUTPUT="$(cat $OUT_FILE)"

if [[ $ACT_OUTPUT != $EXP_OUTPUT ]]; then
    failure "Expected '$EXP_OUTPUT', but got '$ACT_OUTPUT'" $2
else
    success $2
fi

