#!/bin/bash
tmp=$0-$$.tmp
mkdir "$tmp" || exit
cp test-execution.sh "$tmp"
cd "$tmp" || exit

printf "woohoo! you made it to the test file\n" > tmp_test

n=1
for case in \
  'true' \
  'false' \
  'true && true' \
  'true && false' \
  'false && true' \
  'false && false' \
  'true || true' \
  'true || false' \
  'false || true' \
  'false || false' \
  'true ; true' \
  'true ; false' \
  'false ; true' \
  'false ; false' \
  '(true)' \
  '(false)' \
  'cat < invalid_file' \
  'cat < tmp_test' \
  'cat < tmp_test > hello.txt'
do
  echo "$case" >case$n.sh || exit
  echo "#$n" ; cat case$n.sh ; ../timetrash case$n.sh ; printf "Result: $?\n\n"
  n=$((n+1))
done

cd ..
rm -fr "$tmp"
