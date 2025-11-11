#!/bin/bash
for i in {0..5}; do
    echo "./dummy_task $1"
    ./dummy_task "$1"
done
