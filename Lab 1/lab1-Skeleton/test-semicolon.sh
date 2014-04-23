#!/bin/bash

for case in \
    'echo hello;' \
    'echo a ; echo b ; echo c' \
    'echo a && echo b ; echo c ; echo d'
do
    echo "$case" > semicolon_sample
    ./timetrash -p semicolon_sample
    ./timetrash -t semicolon_sample
    rm semicolon_sample
done