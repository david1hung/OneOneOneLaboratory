#!/bin/bash
tmp=$0-$$.tmp
mkdir "$tmp" || exit
cp test.sh "$tmp"
cd "$tmp" || exit

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
  'cat < test.sh' \
  'cat < test.sh > hello.txt'
do
  echo "$case" >case$n.sh || exit
  echo "#$n" ; cat case$n.sh ; ../timetrash case$n.sh ; printf "$?" >> case$n.sh
  n=$((n+1))
done
