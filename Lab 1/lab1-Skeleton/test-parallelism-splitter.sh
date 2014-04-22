#!/bin/bash

echo 'echo hello > a.txt && echo world > b.txt
cat a.txt | sort > c.txt
cat c.txt > d.txt
cat d.txt' > parallelism_splitter

./timetrash -t parallelism_splitter
rm parallelism_splitter