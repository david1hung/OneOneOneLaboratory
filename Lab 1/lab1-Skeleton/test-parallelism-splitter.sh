#!/bin/bash

echo 'od -tf -N 16000000 /dev/urandom > /dev/null
od -tf -N 8000000 /dev/urandom > c.txt
od -tf -N 4000000 /dev/urandom > /dev/null' > parallelism_splitter

printf '#1: Sans parallelism\n'
time -p ./timetrash parallelism_splitter
printf '\n#2: Parallelism with dependencies\n'
time -p ./timetrash -t parallelism_splitter

echo 'od -tf -N 16000000 /dev/urandom > a.txt
od -tf -N 8000000 /dev/urandom > b.txt
od -tf -N 4000000 /dev/urandom > c.txt' > parallelism_splitter

printf '\n#3: Parallelism sans dependencies\n'
time -p ./timetrash -t parallelism_splitter

rm parallelism_splitter a.txt b.txt c.txt