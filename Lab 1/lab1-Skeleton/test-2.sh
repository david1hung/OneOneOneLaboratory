echo 'cat doc1.txt > a.txt  
cat a.txt > b.txt
od -tf -N 1000000 /dev/urandom > f.txt
cat b.txt > f1.txt


cat doc1.txt > c.txt  
cat doc2.txt > c.txt
od -tf -N 100000 /dev/urandom > g.txt
cat doc3.txt > c.txt


cat doc2.txt > d.txt
od -tf -N 10000 /dev/urandom > h.txt
cat d.txt > e.txt  
cat doc1.txt > d.txt' > tmp_script


./timetrash tmp_script
if ! diff -q doc1.txt f1.txt > /dev/null
then
	printf "doc1 & f1 are different\n"
else
	printf "Passed WAR!\n"
fi

if ! diff -q c.txt doc3.txt > /dev/null
	then
	printf "c & doc3 are different"
	else
	printf "Passed WAW!\n"
fi

if ! diff -q e.txt doc2.txt > /dev/null
	then
	printf "e & doc2.txt are different\n"
	else
	printf "Passed RAW!\n"
fi

rm tmp_script a.txt b.txt c.txt d.txt e.txt f1.txt f.txt g.txt h.txt