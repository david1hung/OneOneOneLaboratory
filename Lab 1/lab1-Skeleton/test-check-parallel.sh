#!/bin/bash

#test write read
echo 	'cat doc1.txt > a.txt  
		cat a.txt > b.txt
		cat b.txt > f1.txt' > tmp_script
printf '#1: Testing Write then Read\n'
./timetrash tmp_script
if ! diff -q doc1.txt f1.txt > /dev/null
then
	printf "doc1 & f1 are different\n"
else
	printf "Passed!\n"
fi
	

printf '#2: Testing Write then Write\n'
echo 	'cat doc1.txt > c.txt  
		cat doc2.txt > c.txt
		cat doc3.txt > c.txt' > tmp_script
./timetrash tmp_script
if ! diff -q c.txt doc3.txt > /dev/null
	then
	printf "c & doc3 are different"
	else
	printf "Passed!\n"
fi

cat doc2.txt > d.txt
printf '#3: Testing Read then Write\n'
echo 	'cat d.txt > e.txt  
		cat doc1.txt > d.txt' > tmp_script
./timetrash tmp_script
if ! diff -q e.txt doc2.txt > /dev/null
	then
	printf "e & doc2.txt are different\n"
	else
	printf "Passed!\n"
fi

rm tmp_script a.txt b.txt c.txt d.txt e.txt f1.txt
