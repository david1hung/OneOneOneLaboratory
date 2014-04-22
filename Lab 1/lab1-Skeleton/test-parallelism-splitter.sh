#!/bin/bash

echo 'od -tf -N 3200000 /dev/urandom > c.txt
od -tf -N 1600000 /dev/urandom > a.txt
od -tf -N 800000 /dev/urandom > a.txt
od -tf -N 400000 /dev/urandom > c.txt
od -tf -N 1600000 /dev/urandom > e.txt
od -tf -N 400000 /dev/urandom > a.txt
od -tf -N 1600000 /dev/urandom > f.txt' > parallelism_splitter

printf '#1: Sans parallelism\n'
time -p ./timetrash parallelism_splitter
printf '\n#2: Parallelism with dependencies\n'
time -p ./timetrash -t parallelism_splitter

echo 'od -tf -N 3200000 /dev/urandom > c.txt
od -tf -N 1600000 /dev/urandom > a.txt
od -tf -N 800000 /dev/urandom > b.txt
od -tf -N 400000 /dev/urandom > d.txt
od -tf -N 1600000 /dev/urandom > e.txt
od -tf -N 400000 /dev/urandom > f.txt
od -tf -N 1600000 /dev/urandom > g.txt' > parallelism_splitter

printf '\n#3: Parallelism sans dependencies\n'
time -p ./timetrash -t parallelism_splitter

rm parallelism_splitter a.txt b.txt c.txt d.txt e.txt f.txt g.txt