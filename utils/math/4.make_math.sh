!/bin/bash

if [ "y$1" = "y" ]; then
	echo "usage: $0 <date>"
	exit;
fi

date=$1;
prog=./fm1

if [ ! -d "$date/math" ]; then
	mkdir "$date/math"
fi

for i in `cat $date/fname.txt`; do
	rm /dev/shm/wiki/[0-7] -rf
	lang=`perl -e '$_=shift;s/wiki-.*//;print $_' $i`
	wiki_date=`perl -e '$_=shift;/wiki-(\d{8})/;print $1' $i`
	$prog 1 "$date/${lang}wiki-$wiki_date-math.tmp.txt" "$date/math/fastwiki.math.$lang.$wiki_date"
done
