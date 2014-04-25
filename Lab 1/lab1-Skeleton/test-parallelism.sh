#!/bin/bash

result="test-parallelism-$USER.txt"

echo 'od -tf -N 1600000 /dev/urandom > a.txt
od -tf -N 800000 /dev/urandom > b.txt
od -tf -N 1600000 /dev/urandom > a.txt
od -tf -N 1600000 /dev/urandom > b.txt
od -tf -N 400000 /dev/urandom > a.txt
od -tf -N 1600000 /dev/urandom > b.txt' > parallelism_splitter

printf '#1: Sans parallelism\n' # > $result
{ time -p ./timetrash parallelism_splitter ; } # 2>> $result
printf '\n#2: Parallelism with dependencies\n' # >> $result
{ time -p ./timetrash -t parallelism_splitter ; } # 2>> $result

echo '(echo a && echo b ; echo c ;)
echo d
echo e

echo f
echo g
echo h || echo i && echo j ; echo l' > parallelism_splitter

printf '\n#3: Parallelism testing splitting\n' # >> $result
{ time -p ./timetrash -t parallelism_splitter ; } # 2>> $result

rm parallelism_splitter a.txt b.txt